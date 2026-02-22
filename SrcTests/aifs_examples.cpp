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
#include "oper_block.h"
#include "aifs_tester.h"
#include "block_class.h"
#include "block_graph.h"
#include "variable.h"
#include "edge_ball.h"
#include "edge_map.h"
#include "samples.h"

TEST(testExamples, test_all)
{
	samples smp;
	aifs_tester t;

	//load no more than one without ID for each parent
	ankerl::unordered_dense::set<size_t> used_pars;
	
	std::cout << "Test examples " << std::endl;
	std::string berr;
	for (let& c : smp.m_samples) {
		for (let& e : c.smp) {
			std::cout << c.name << ": " << e.name << std::endl;

			berr.clear();

			IMS_SCOPE([&] {
				if (!berr.empty()) {
					FAIL() << "Example: " << e.name << "[" << berr << "]" << 
						" Error: " << t.err_msg;
				}
			});

			
			t.aifs = { reinterpret_cast<const char*>(e.data), e.size };
			if (!t.init_ex()) { berr = "*"; return; };

			used_pars.clear();

			size_t num_tested = 0;
			for (let id : t.nfo->m_list.m_blocks) {
				let& d = t.nfo->m_list.m_id2data[id];

				if (d.m_str_id == ims_max && d.b->get_parent()) {
					if (!used_pars.emplace(d.b->get_parent()->m_block_id).second) {
						continue;
					}
				}

				auto* b = const_cast<oper_block*>(d.b.get());

				++num_tested;
				
				t.bi.recalc_graph();
				if (!t.bi.init4(*b, t.ec, t.am, t.gid.get(), t.ac)) {
					berr = b->str_id4();
					if (berr.empty())berr = std::to_string(b->m_block_id);
					if (b->m_parent) {
						berr += ":";
						berr += b->m_parent->str_id4();
					}
				};
			}

			std::cout << "Tested "<< num_tested << " blocks" << std::endl;;
		}
	}
};
