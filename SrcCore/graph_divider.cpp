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
#include "graph_divider.h"
#include "projector.h"
#include "eval_pool.h"
#include "eval_helpers.h"
#include "ims_graph_base.h"
#include "ims_val.h"
#include "edge_map.h"



void graph_divider::init_gd(
	std::span<const edge_map> ri,
	std::span<const edge_ball> vb,
	const ims_graph_base& dig,
	const size_t root)
{
	stack.reserve(10000);


	release_maps();
	stack.resize(1);
	auto& q = stack.front();

	q.m = eval_pool::ep.get_id_val();
	q.ver = root;

	auto& rb = vb[root];
	if (rb.defined2()) {
		q.b = rb;
		q.b.get()->add_ref();
	} else {
		assert(!q.b.get());
	}

	size_t idx = 0;//up to what index are we ready
	elem qdiv;

	while (idx < stack.size()) {

		if (vb[stack[idx].ver].defined2()) {
			++idx;
			continue;
		}

		//move to the end
		if (idx + 1 < stack.size()) {
			std::swap(stack[idx], stack.back());
		}

		//divede
		qdiv = std::move(stack.back());//copy - stores a reference to the map
		stack.pop_back();
		

		let& qv = dig.m_vers[qdiv.ver];

		for (size_t i = 0; i < qv.sz; ++i) {
			let& e = dig.m_edges[qv.idx + i];

			auto& dst = stack.emplace_back();
	
			dst.ver = e.second;
			dst.m = eval_helpers::edge_mul(qdiv.m.get(), ri[e.m].m.get(), true);

			let& b = vb[dst.ver];
			if (b.defined2()) {
				dst.b = eval_helpers::mul_ball(dst.m.get(), b.get());
			}
		}
	}
}

void graph_divider::get_box(box<Real>& dst) const
{
	dst.clear();
	for (let& e : stack) {		
		dst.add_point(e.b.center(), e.b.radius());
	}
}

void graph_divider::divide_prec(
	std::span<const edge_map> ri,
	std::span<const edge_ball> vb,
	const ims_graph_base& dig,
	const subspace_info<Real>& si,
	const std::function<bool(const ims_val* ball)>& func)
{
	
	let dim = si.get_dim_space();
	let dpr = si.get_section_dim();

	projector proj;
	proj.R = si.basis;
	proj.calc_L_ortho();

	edge_ball bd(eval_pool::ep.get_vector_real(dim + 1));
	edge_ball pc(eval_pool::ep.get_vector_real(dpr + 1));

	
	DynVec<Real> tv;//temporary

	IMS_SCOPE([&] {release_maps();});

	while (!stack.empty() && !ims_need_stop()) {

		//check the ball
		auto cur_elem = std::move(stack.back());
		stack.pop_back();
		
		if (cur_elem.b.dim() < dim) {
			cur_elem.b.reset(eval_helpers::extend_real_vector_size(cur_elem.b.get(), dim + 1));
		}

		bd.center() = cur_elem.b.center();
		bd.center().noalias() -= si.origin;
		bd.set_radius(cur_elem.b.radius());
		
		//projection of the center onto the subspace
		pc.center().noalias() = proj.L * bd.center();
		pc.set_radius(bd.radius());

		//does it intersect with the required plane?
		if (dpr != dim) {
			//orthogonal projection of the center of the ball onto the subspace
			tv.noalias() = proj.R * pc.center();
			tv.noalias() -= bd.center();
			if (tv.norm() >= bd.radius()) {
				continue;
			}
		}

		if (func(pc.get())) {
			continue;//no need to divide it further
		}

		//go deeper
		let& qv = dig.m_vers[cur_elem.ver];

		for (size_t i = 0; i < qv.sz; ++i) {
			let& e = dig.m_edges[qv.idx + i];

			auto& ne = stack.emplace_back();
		
			ne.ver = e.second;
			ne.m = eval_helpers::edge_mul(cur_elem.m.get(), ri[e.m].m.get(), true);
			
			let& src = vb[ne.ver];
			assert(src.defined2());
			ne.b = eval_helpers::mul_ball(ne.m.get(), src.get());
		}
	}
}

void graph_divider::release_maps()
{
	for (auto& s : stack) {
		s.m.reset();
		s.b.reset();
	}
	stack.clear();
}
