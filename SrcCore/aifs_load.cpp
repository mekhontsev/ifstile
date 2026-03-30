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
#include "aifs_load.h"
#include "ims_info.h"
#include "ifs_data_text.h"
#include "ims_stage.h"
#include "aifs_printer.h"
#include "oper_block.h"

//load from a text stream in internal format
//returns false if a critical error occurs
//true if the file was interrupted by the user or loaded successfully
bool aifs_from_stream(
	ifs_list& lst,
	size_t& cur_line,
	read_state& rs,
	std::istreambuf_iterator<char>& it_beg,
	const std::istreambuf_iterator<char>& it_end)
{
	auto& rt = ims_stage::get();

	for (;;) {//loop through blocks

		if (ims_need_stop()) {
			break;
		}
		rt.work_add(1);

		if (!aifs_from_stream_ex(lst, cur_line, rs, it_beg, it_end)) {
			return false;
		}

		if (rs.m_source_num_lines == 0) {
			break;//ready
		};
	};

	return true;
};

bool ims_load7(
	ims_info& nfo,
	std::string_view filename,
	std::istreambuf_iterator<char>& nbeg,
	const std::istreambuf_iterator<char>& nend,
	bool bappend
)
{
	let from = bappend ? nfo.m_list.m_blocks.size() : 0;

	size_t cur_line = 1;

	read_state rs;

	std::string js_src;
	js_from_stream(js_src, cur_line, nbeg, nend);

	bool do_porcess = true;

	if (bappend) {
		if (nfo.m_js_src.empty()) {
			nfo.m_js_src = std::move(js_src);
		} else if (js_src.empty() || js_src == nfo.m_js_src) {
			do_porcess = false;
		} else {
			ims_error("Incompatible JS");
			return false;
		}
	} else {
		nfo.m_js_src = std::move(js_src);
	}

	if (!nfo.m_js_src.empty() && do_porcess) {
		nfo.m_js_filename = filename;
		if (!nfo.process_js(rs)) {
			return false;
		}
	}

	if (!aifs_from_stream(nfo.m_list, cur_line, rs, nbeg, nend)) {
		return false;
	}


	if (nfo.m_list.empty() && nfo.m_js_src.empty()) {
		ims_warning("Empty file");
		return true;//then the user can manually add blocks
	}

	if (!nfo.link_refs(from)) {
		return false;
	}

	return true;
}


bool ims_apply_source(
	const ims_info& nfo,
	std::unique_ptr<ims_info>& dst,
	const oper_block** new_b,
	std::string_view src,
	const oper_block* b,
	bool js_mode)
{
	*new_b = nullptr;

	//repeat the process of loading the entire file
	//because many things may depend on the block
	//even if js_mode == true, we need to return what block b has become

	if (!dst) {
		dst.reset(new ims_info);
	}
	auto& imp = *dst;


	read_state rs;


	////////////////////////////////////////////////////////////////////////////

	if (js_mode) {
		imp.m_js_src = src;
	} else {
		imp.m_js_src = nfo.m_js_src;//does not change
	}

	//reuse ims_info::m_js_filename
	if (!imp.process_js(rs)) {
		return false;
	};

	////////////////////////////////////////////////////////////////////////////

	aifs_printer de;
	std::vector<const oper_block*> dummy;
	de.prepare(nfo.m_list, dummy, false, true);

	std::stringstream str;

	if (!b && !js_mode) {//at the end we'll try to insert a new block
		de.arr.emplace_back(nullptr);
	}

	for (size_t i = 0; i < de.arr.size(); ++i) {
		let* cb = de.arr[i];

		if (cb == b && !js_mode) {
			//including the case of inserting a new block (cb == b == nullptr)
			str << src;
		} else {
			assert(cb);
			de.write_block(str, cb, cb->m_flags, true);
		}

		using IIC = std::istreambuf_iterator<char>;
		auto nbeg = IIC(str);
		let nend = IIC();

		size_t cur_line = cb ? cb->m_line8 : 0;

		if (!aifs_from_stream(imp.m_list, cur_line, rs, nbeg, nend)) {
			return false;
		}

		if (rs.m_source_num_lines == 0) {
			break;
		}
		//one iteration gives exactly 1 block
		assert(imp.m_list.m_blocks.size() == i + 1);

		auto* new_cb = imp.m_list.get_block_by_idx(i);
		if (cb == b) {
			*new_b = new_cb;
		}

		std::stringstream().swap(str);//clear
	}

	return imp.link_refs(0);
}