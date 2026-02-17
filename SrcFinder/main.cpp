// This file is part of IFStile project
// Copyright (C)2026 Dmitry Mekhontsev <mekhontsev@gmail.com>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "pch.h"

#include "ims_file.h"
#include "error_helper.h"
#include "columns.h"
#include "finder.h"
#include "ims_info.h"
#include "oper_block.h"
#include "aifs_printer.h"
#include "ims_chrono.h"
#include "env_block.h"
#include "ims_keywords.h"
#include "aifs_load.h"

struct render_params;

void ims_num_traits_init_all();


ims_static std::mutex g_lock;

oper_block* get_cur_block() 
{ 
	//TODO: used only in one place in finder.cpp
	return nullptr; 
};

void ext_console_clear() {};

int main_utf8(int argc, char** argv)
{    
    if (argc < 2) {
        std::cout << "You must provide the path to AIFS file" << std::endl;
        return 0;
    }

	std::string this_file = argv[0];
	std::replace(this_file.begin(), this_file.end(), '\\', '/');


    ims_num_traits_init_all();

    let fp = ims_file::adjust(argv[1]);
	ims_info nfo;
	
    std::ifstream file;
	ims_file::open(file, fp);
	if (!file.is_open()) {
        std::cout << "Could not open file for read" << std::endl;
		return 0;
	}
	auto iter = std::istreambuf_iterator<char>(file);
	let end = std::istreambuf_iterator<char>();

	let is_ok = ims_load7(nfo, fp, iter, end, false);

	file.close();

    if (!is_ok){
        return 0;
    }
   
	auto& fnd = finder::get();
	env_block_data ebd;
	ebd.cols = &columns::get();
	ebd.fparams = &fnd;
	ebd.rparams = nullptr;

	auto* xb = nfo.m_list.find_block2(ims_keywords::search_params_block, "");
	if (xb)load_env_block(xb, ebd);

    fnd.init_search_domain(nfo.m_list);


	std::ofstream ofile;
	ims_file::open(ofile, fp, std::ios_base::binary | std::ios_base::app);
	if (!ofile.is_open()) {
		std::cout << "Could not open file for write" << std::endl;
		return 0;
	}

	ims_chrono init_started;

	size_t num_found = 0;

	auto cb = [&ofile, &init_started, &num_found]
	(finder::check_result r, oper_block* sr) mutable
	{
		auto& fnd = finder::get();
		if (r == finder::init_started) {
			init_started = fnd.m_start_time;
			return;
		}
		if (r == finder::search_started) {
			let df = ims_chrono::dif_micro(init_started, fnd.m_start_time);
			std::cout << "Search is initialized: " << df * 1e-6 << " s" << std::endl;
			return;
		}

		if (r == finder::new_found || r == finder::best_replaced) {
			//save to file
			ims_write_block(ofile, sr);

			ofile.flush();

			++num_found;

			std::cout << '\r' 
				<< "[" << ims_chrono::fmt_time_t_to_hmsdmY().buf << "] " 
				<< num_found;
			std::cout.flush();
		}
	};

	std::cout << "Search is started" << std::endl;

	finder::find_set(
		fnd,
        nfo.m_list,
		columns::get(),
        g_lock,
		cb);
   
    return 0;
}