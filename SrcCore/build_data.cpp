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
#include "build_data.h"
#include "oper_block.h"
#include "block_class.h"
#include "clock_print.h"
#include "eval_context.h"
#include "block_graph.h"
#include "variable.h"
#include "ast_stack.h"
#include "ims_info.h"
#include "custom_block.h"
#include "inter_type.h"

ims_info& ims_info_get();


oper_block& build_data::get_block()
{
	return *m_block_sq;
}

const oper_block& build_data::get_block() const
{
	return *m_block_sq;
}

bool build_data::empty() const
{
	return !m_block_sq || m_block_sq->empty4();
}

void build_data::clear()
{
	m_bb = nullptr;
	m_normal_parent.reset();
	m_special.clear8();
	m_bi.clear_proj_data();
	m_bi.set_to_recalc_graph();
	if (m_block_sq)m_block_sq->clear();
}

void build_data::pre_init(
	const oper_block* db,
	const variator_params& vp)
{
	if (!m_block_sq) {
		m_block_sq = std::make_unique<oper_block>();
	} else {
		m_block_sq->clear();
	}
	m_bb = db;
	m_vp = vp;
	m_normal_parent.reset();
	//do not clear cached proj data!
	m_bi.set_to_recalc_graph();
	m_special.clear8();//find them before building
}


oper_block* build_data::get_direct(ifs_object_type m)
{
	let cond = (m == ifs_object_type::normal) && m_normal_parent;
	return cond ? m_normal_parent.get() : m_block_sq.get();
}

void build_data::adjust_roots(ifs_object_type tsrc)
{
	if (!m_bi.exists()) {
		return;
	}

	auto* bx = get_direct(tsrc);

	let ref = bx->get_active_ref();
	if (ref == ims_max) {
		assert(false);
		return;
	}
	
	auto bstr = bx->get_class()->get_var_name(ref);

	let bnrm = (tsrc == ifs_object_type::normal);

	auto bstr_normal = bstr;
	if (!bnrm) {
		//cut off the last underscore
		let pos = bstr.rfind("_");
		if (pos != std::string::npos) {
			bstr_normal = bstr.substr(0, pos);
		}
	}

	let tn = tsrc == ifs_object_type::normal ?
		ifs_object_type::custom :
		ifs_object_type::normal;

	auto* b = get_direct(tn);

	if (!b)return;
	let qnrm = (tn == ifs_object_type::normal);
	
	let sz = b->num_vars();
	
	let& rf = b->ctx()->m_refs5;

	//current
	let qidx = b->get_active_ref();

	let* g = b->get_graph();

	size_t find_idx = ims_max;
	for (size_t i = 0; i < sz; ++i) {

		if (!g->closed2(i) || rf[i].is_subs) {
			continue;
		}

		auto qstr = b->get_class()->get_var_name(i);

		bool accepted = false;

		if (qnrm) {
			if (qstr == bstr_normal) {
				accepted = true;
			}
		} else {
			//cut off the last underscore
			let pos = qstr.rfind("_");
			if (pos != std::string::npos) {
				if (qstr.substr(0, pos) == bstr_normal) {
					accepted = true;
				}
			}
		}

		if (accepted) {
			if (qidx == i) {
				find_idx = i;//turned out to be current
				break;
			}
			if (find_idx == ims_max) {// looking for the first one
				find_idx = i;
				//continue, maybe the current qidx is suitable
			}
		}
	}

	if (find_idx != ims_max) {
		b->set_active_ref(find_idx);
	}
	
}

background* build_data::get_background()
{
	if (m_special.chas_builtin(builtin_ids::background)) {
		return &m_special.m_bac;
	}
	return nullptr;
}


void build_data::set_block(std::unique_ptr<oper_block>& other) 
{
	m_block_sq = std::move(other);
}


void build_data::on_change_mode() 
{
	if (!m_normal_parent)return;

	set_block(m_normal_parent);
	m_bi.set_to_recalc_graph();
}

bool build_data::init_normal_block()
{
	if (m_bi.m_id8 > 0) {//cached
		return m_bi.exists();
	}
	if (!m_bb) {//build image, delete all blocks, try to rotate
		return false;
	}

	test_clock_print clock("init_normal_block");

	auto* b = m_block_sq.get();

	if (b->empty4()) {
		check_block(m_bb);

		if (!m_bb->m_graph) {
			m_bi.gen_next_id();
			return false;
		}

		m_changed = b->inherit_from(*m_bb, m_vp, m_bi.m_cv, true);
		b->m_name = m_bb->m_name;
	}

	let res = m_bi.init4(*b);

	if (!res) {
		return false;
	}


	if (!m_bi.exists()) {
		return false;
	}

	
	//root is needed before construction,
	//because below we change the graph (set_active_ref)
	//which is prohibited during construction
	auto root = m_special.eval_root(*b);

	if (root != ims_max) {
		b->set_active_ref(root);
	} else if(b->get_active_ref() == ims_max) {
		root = b->find_default_ref();
		if (root == ims_max) {
			return false;
		}
		b->set_active_ref(root);
	}

	m_special.eval_builtins(get_block(), m_bi.m_ctx);

	////////////////////////////////////////////////////////////////////

	if (ims_need_stop()) {
		return false;
	}

	//even if the moment or dimension could not be calculated correctly
	//important values are filled with acceptable values
	if (!m_bi.compute_metrics()) {
		return false;
	}

	if (ims_need_stop()) {
		return false;
	}


	return true;
}


size_t build_data::get_froot() const
{
	return get_block().get_froot();
}


bool build_data::can_create_view() const
{
	return !m_normal_parent && !m_changed && !empty() && m_bi.exists() && m_bb;
}

bool build_data::init_custom_block(
	ims_identifiers& idf,
	ifs_object_type mode,
	const report_params& rp)
{

	if (m_normal_parent) {
		return m_bi.exists();
	}

	std::unique_ptr<oper_block> block_custom;

	inter_result ires;

	if (!create_custom_block(
		block_custom,
		ires,
		m_bi,
		*get_block().elevate_empty(),
		idf,
		mode,
		rp))
	{
		return false;
	};


	ast_stack ai;
	ims_info::link_refs_for_block(ims_info_get(), block_custom.get(), ai);


	m_normal_parent = std::move(m_block_sq);//he is now a parent.
	set_block(block_custom);

	auto& b = get_block();

	m_bi.set_to_recalc_graph();

	let res = m_bi.init4(b);


	if (!res) {
		return false;//fatal error
	}

	if (!m_bi.exists()) {
		return false;
	}


	if (ims_need_stop()) {
		return false;
	}


	if (!m_bi.compute_metrics()) {
		return false;
	}
	if (ims_need_stop()) {
		return false;
	}

	//find a root based on the usual set
	adjust_roots(ifs_object_type::normal);

	return true;
}


