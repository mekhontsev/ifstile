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
#include "ifs_renderer.h"
#include "ifs_data_text.h"
#include "eval_context.h"
#include "ast_maps.h"
#include "graph_init_data_ptr.h"
#include "affine_calc.h"

void check_block(const oper_block*);

bool ifs_renderer::init(const std::string& aifs)
{
	std::istringstream istr(aifs);
	auto nbeg = std::istreambuf_iterator<char>(istr);
	let nend = std::istreambuf_iterator<char>();

	/////////////////////////////////////////////////
	
	size_t cur_line = 1;

	read_state rs;

	std::string js_src;
	js_from_stream(js_src, cur_line, nbeg, nend);

	
	m_nfo.m_js_src = std::move(js_src);
	

	if (!m_nfo.m_js_src.empty()) {
		m_nfo.m_js_filename = "";
		if (!m_nfo.process_js(rs)) {
			return false;
		}
	}


	for (;;) {//loop through blocks
		if (!aifs_from_stream_ex(m_nfo.m_list, cur_line, rs, nbeg, nend)) {
			return false;
		}

		if (rs.m_source_num_lines == 0) {
			break;//completed
		};
	};


	if (m_nfo.m_list.empty() && m_nfo.m_js_src.empty()) {
		return false;
	}

	if (!m_nfo.link_refs(0)) {
		return false;
	}

	//find first visible
	oper_block* b = nullptr;
	for (let bid : m_nfo.m_list.m_blocks) {
		auto* pb = m_nfo.m_list.m_id2data[bid].b.get();
		if (!pb->m_flags.hidden) {
			b = pb;
			break;
		}
	}
	
	if (!b) {
		return false;
	}

	eval_context ec;
	ast_maps am;
	graph_init_data_ptr gd;
	affine_calc ac;
	if (!m_bi.init4(*b, ec, am, true, gd.get(), ac)) {
		return false;
	}


	//root is needed before building,
	//because below we change the graph (set_active_ref)
	//which is prohibited during building
	auto root = m_sv.eval_root(*b);
	if (root == ims_max) {
		root = b->find_default_ref();
		if (root == ims_max) {
			return false;
		}
	}
	b->set_active_ref(root);
	
	m_sv.eval_builtins(*b, ec);



	////////////////////////////////////////////////////////////////////

	//even if the moment or dimension could not be calculated correctly
	//important values are filled with acceptable values
	affine_dim_calc dc;
	if (!m_bi.compute_metrics(dc)) {
		return false;
	}

	m_bb = b;

	return true;
}

bool ifs_renderer::render(ims_bitmap& dst, float quality, float thickness)
{
	clear_color(dst);
	
	
	let root2 = m_bb->get_froot();
	let dim_set = m_bi.m_ver_dim[root2];
	if (dim_set == ims_max || dim_set == 0) {
		return false;
	};
	
	//initialize the subspace
	auto& sv = m_sv;
	auto& si = sv.m_si2;


	///////////////////////////////////////////////

	si.resize2(dim_set,  dim_set);
	si.reset();

	if (dim_set > si.get_section_dim()) {
		//we select the most elongated directions, example:
		//k1 <= k2 <= k3 are singular values, so if k1 != k2, we take (k2, k3)
		//otherwise, we take (k1, k2)
		let& me = m_bi.m_im.me[root2];
		si.origin = me.C;

		auto& b = si.basis_user;

		size_t start_idx = me.Q.cols() - b.cols();

		while (start_idx > 0 &&
			std::abs(me.I(start_idx) - me.I(start_idx - 1)) <
			ims_num_traits<double>::almost_zero())
		{
			--start_idx;
		}


		for (int c = 0; c < b.cols(); ++c) {
			b.col(b.cols() - 1 - c) = me.Q.col(start_idx + c);
		}
	}


	si.init_si();

	let sds = si.get_section_dim();

	if (sds < 1 || sds > 3) {
		return false;
	}

	if (m_bi.get_fg().is_ver_empty(root2)) {
		return false;
	}

	//builder type
	
	return true;
}
