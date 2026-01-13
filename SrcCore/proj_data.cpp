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
#include "proj_data.h"
#include "eval_context.h"
#include "ims_val.h"
#include "eval_helpers.h"
#include "pool_ptr.h"
#include "block_form.h"
#include "oper_block.h"

void proj_data::recheck()
{
	ims_erase(m_projs, [](let& e) {return !e.ready; });

	for (auto& q : m_projs) {
		q.ready = false;
	}
}


void proj_data::clear()
{
	m_projs.clear();
}

size_t proj_data::get_proj(const ast_context& p, eval_context& ec)
{
	assert(p.is_def());

	size_t ret = m_projs.size();

	for (size_t i = 0; i < m_projs.size(); ++i) {
		if (ast_context::lexic_compare(m_projs[i].m_ptr, p) == 0) {
			ret = i;
			break;
		}
	}
	
	proj_elem* e = nullptr;

	if (ret < m_projs.size()) {
		e = &m_projs[ret];
	} else {
		e = &m_projs.emplace_back();
		e->m_ptr = p;
	}

	if (e->ready) {
		return ret;
	}

	e->ready = true;

	////////////////////////////////////////////////////////////////////////////

	bool err = true;//until we are convinced otherwise

	IMS_SCOPE([&] {
		if (err) {//defined, but not evaluated
			e->m_projector.clear();
			ims_error("invalid subspace");
		}
	});


	pool_ptr pr(ec.eval7(p, true));

	if (!pr) {
		return ret;
	}
	if (!pr->is(ims_val::ETP::vector, ims_val::EST::other)) {
		return ret;
	}

	let sz = pr->get_size();

	if (sz < 2) {
		return ret;
	}

	let** va = pr->p_v();

	if (!va[0]) {
		return ret;
	}

	pool_ptr v(eval_helpers::to_affine3(va[0], p.a->get_dim()));

	if (!v || !v->is(ims_val::EST::rational)) {
		return ret;
	}

	eval_helpers::to_proj_integer(v.get(), m_A_temp);

	//calculate the subspace indices
	m_sbt_temp.resize(sz - 1);
	for (size_t i = 1; i < sz; ++i) {
		let* vi = va[i];
		if (!vi) {
			return ret;
		}
		double index;
		if (!vi->to_real(index)) {
			return ret;
		}
		m_sbt_temp[i - 1] = index;
	}

	bool eq =
		!e->m_projector.empty() &&
		m_A_temp.rows() == e->m_A.rows() &&
		m_A_temp == e->m_A &&
		m_sbt_temp == e->m_sbt;

	if (eq) {
		err = false;//all checks passed
		return ret;
	}

	e->m_sbt = m_sbt_temp;
	e->m_A = m_A_temp;

	if (!block_form::get_proj(
		e->m_A, e->m_sbt, e->m_projector.L, e->m_projector.R))
	{
		return ret;
	}

	assert(!e->m_projector.empty());

	err = false;//all checks passed

	return ret;
}
