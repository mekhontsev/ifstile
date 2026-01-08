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
#include "full_search.h"
#include "oper_block.h"
#include "block_class.h"
#include "ims_info.h"
#include "eval_context.h"
#include "variable.h"

full_search::status full_search::next(oper_block& dst)
{
	if (m_next_idx == 0) {
		for (auto& q : m_state) {
			q.v = q.b;
		}
	} else {
		if (!next_vec()) {//completed
			return status::complete;
		};
	}

	++m_next_idx;

	let& src = *m_tm;
	
	let* g = src.get_parent()->ctx();

	let sz = src.num_vars();


	dst.set_parent(src.get_parent());
	dst.m_class.reset();
	dst.m_ops = src.m_ops;//copy all operators
	dst.m_first_var = src.m_first_var;

	size_t vidx = 0;//global index

	for (size_t vi = 0; vi < sz; ++vi) {
		if (!g->m_refs5[vi].is_var())continue;
		for (let& q : m_opinfo4.a[vi].data) {
			let& qs = q.src;
			let& op = qs.h;
			let dst_idx = q.dst_idx9;
			assert(dst_idx < dst.m_ops.size());

			distrib_info di;
			src.get_distrib(di, qs);

			switch (op.tt) {
			case ETYPE::set_vector:
			{
				size_t dim = op.get_u24();
				if (dim == 0)dim = src.get_dim();

				let nidx = dst.add(dim);//allocate memory first

				auto& h = dst.m_ops[dst_idx].hdr;
				h.tt = ETYPE::vector_imm;
				h.ts = ESUBTYPE::integer;
				h.u32 = static_cast<uint32_t>(nidx);
				h.set_u24(dim);

				
				for (size_t k = 0; k < dim; ++k) {
					dst.m_ops[nidx + k].i64 = m_state[vidx++].v;
				}
				break;
			}
			case ETYPE::set_binary:
			{
				size_t dim = op.get_u24();
				if (dim == 0)dim = src.get_dim();
				
				let d0 = (size_t)di.d[0];
				let d1 = (size_t)di.d[1];

			
				size_t num_emp = 0;
				for (size_t k = 0; k < dim; ++k) {
					if (m_state[vidx+k].v == 0) {
						++num_emp;
					}
				}

				if (num_emp < d0 || num_emp > d1) {
					return status::ignore;
				}
			

				let nx=dst.add(dim);//allocate memory first

				auto& h = dst.m_ops[dst_idx].hdr;

				h.tt = ETYPE::vector;
				h.u32 = static_cast<uint32_t>(nx);
				h.set_u24(dim);

				for (size_t k = 0; k < dim; ++k) {
					if (m_state[vidx++].v) {
						dst.m_ops[nx + k].hdr.set_id();
					} else {
						dst.m_ops[nx + k].hdr.set_xempty();
					}
				}
				break;
			}
			case ETYPE::set_interval:
			{
				let nidx = dst.add(1);//allocate memory first
				
				auto& h = dst.m_ops[dst_idx].hdr;
				h.tt = ETYPE::number;
				h.ts = ESUBTYPE::integer;
				h.u32 = static_cast<uint32_t>(nidx);

				dst.m_ops[nidx].i64 = m_state[vidx++].v;

				break;
			}
			default:
				return status::ignore;
			}
		}
	}

	dst.m_block_id = block_id_max;
	dst.m_name = std::to_string(m_next_idx-1);
	dst.remove_search();
	dst.m_dim2 = src.m_dim2;
	dst.m_subspace = src.m_subspace;

	auto& f = dst.m_flags;
	f = src.m_flags;
	f.free_var = false;
	f.only_var = true;
	f.has_dim = false;
	f.checked = false;
	f.hidden = false;
	f.ready = false;
	

	return status::ok;

	//check_block(&dst);
	//return dst.is_invalid() ? status::ignore : status::ok;
}

bool full_search::reset(const oper_block& src, int64_t fall_back_rad)
{
	m_next_idx = 0;
	m_state.clear();

	if (!m_tm) {
		m_tm = std::make_unique<oper_block>();
	} else {
		m_tm->clear();
	}

	auto& dst = *m_tm;

	////////////////////////////////////////////////////////////////////////
	let* g = src.ctx();


	let sz = src.num_vars();

	let& ecr = g->m_refs5;

	m_opinfo4.a.resize(sz);

	dst.m_class.reset();
	dst.set_parent(&src);
	dst.m_flags = src.m_flags;
	dst.m_flags.ready = false;
	dst.m_dim2 = src.m_dim2;
	dst.m_flags.from_js = false;

	dst.clear_ops();
	uint32_t pos = 0;

	state_elem e;

	

	for (size_t vi = 0; vi < sz; ++vi) {
		let& rf = g->m_refs5[vi];
		if (!rf.is_var())continue;

		let ds = dst.add_var(pos, vi, false);

		auto& cv = m_opinfo4.a[vi];
		cv.data.clear();
		dst.insert_op_ex(ds, ecr[vi].c, *g, &cv);

		for (let& q : cv.data) {
			let& qs = q.src;
			let& op = qs.h;
			

			distrib_info di;
			src.get_distrib(di, qs);


			switch (op.tt) {
			
			case ETYPE::set_interval:
			case ETYPE::set_vector:
			{
			
				//convert normal distribution to uniform
				if (di.s == ESUBTYPE::dist_normal_def ||
					di.s == ESUBTYPE::dist_normal)
				{
					e.b = -fall_back_rad;
					e.e =  fall_back_rad + 1;

				} else if (	di.s == ESUBTYPE::dist_uniform){
					e.b = (int64_t)(di.d[0]);
					e.e = (int64_t)(di.d[1]) + 1;
				} else {
					return false;
				}

				if (e.e <= e.b) {
					return false;
				}


				size_t dim;
				if (op.tt == ETYPE::set_interval) {
					dim = 1;
				} else {
					dim = op.get_u24();
					if (dim == 0)dim = src.get_dim();
				}

				for (size_t k = 0; k < dim; ++k) {
					m_state.emplace_back(e);
				}

				break;
			}case ETYPE::set_binary:
			{
				e.b = 0;
				e.e = 2;
				auto dim = op.get_u24();
				if (dim == 0)dim = src.get_dim();

				for (size_t k = 0; k < dim; ++k) {
					m_state.emplace_back(e);
				}

				break;
			}
			default:
				return false;
			}
		}


	}

	return true;
}



bool full_search::next_vec()
{
	for (auto& q : m_state) {
		++q.v;
		if (q.v < q.e) {
			return true;
		}
		q.v = q.b;
	}
	return false;
}

