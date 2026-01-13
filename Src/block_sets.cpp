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
#include "block_sets.h"
#include "build_data.h"
#include "oper_block.h"
#include "eval_context.h"
#include "block_graph.h"
#include "variable.h"

bool block_sets::init8(const build_data& bd)
{	
	let bid = bd.m_bi.m_id8;

	if (m_cur_bid == bid)return false;

	m_cur_bid = bid;

	m_all_refs.clear();

	let& sr = bd.get_block();

	let& ec = *sr.ctx();
	let& g = *sr.get_graph();

	let sz = sr.num_vars();
	for (size_t i = 0; i < sz; ++i) {
		if (!g.closed2(i))continue;
		if(ec.m_refs5[i].is_subs)continue;

		let ver = g.ref2fg(i);
		if (bd.m_bi.get_fg().num_edges(ver) == 0)continue;

		m_all_refs.emplace_back(i);
	}
	
	return true;
}



size_t block_sets::change_set2(const build_data& bi, int step, bool cycle)
{
	
	auto& uc = *this;
	uc.init8(bi);

	let sz = uc.m_all_refs.size();

	if (sz == 0) {
		return ims_max;
	}
	size_t root = bi.get_block().get_active_ref();
	if (root == ims_max) {
		root = step > 0 ? 0 : sz - 1;
	}

	size_t pos = ims_max;
	for (size_t i = 0; i < sz; ++i) {
		if (root == uc.m_all_refs[i]) {
			pos = i;
			break;
		}
	}
	if (pos == ims_max) {
		return ims_max;
	}

	//it's a bit complicated
	size_t astep = size_t(std::abs(step)) % sz;
	if (cycle) {
		if (step > 0) {
			pos = (pos + astep) % sz;
		} else {
			pos = (pos + sz - astep) % sz;
		}
	} else {
		if (step > 0) {
			if (pos + astep < sz) {
				pos += astep;
			} else {
				pos = sz - 1;
			}
		} else {
			if (pos >= astep) {
				pos -= astep;
			} else {
				pos = 0;
			}
		}
	}
	return uc.m_all_refs[pos];
}
