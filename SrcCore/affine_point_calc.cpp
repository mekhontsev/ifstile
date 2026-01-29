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
#include "affine_point_calc.h"
#include "eval_pool.h"
#include "mat_operations.h"
#include "pool_ptr.h"
#include "eval_helpers.h"
#include "edge_ball.h"
#include "edge_map.h"
#include "ims_num_traits.h"
#include "ims_graph.h"

using Real = affine_point_calc::Real;

cardinality affine_point_calc::get_status() const
{
	cardinality ret = cardinality::empty;
	for (let& q : m_points_in_comp) {
		if (q >= cardinality::nzero)continue;
		ret = std::max(ret, q);
	}
	return ret;
}

void affine_point_calc::process(
	std::vector<edge_ball>& vb, 
	std::span<const edge_map> ri,
	const ims_graph& g)
{
	m_points_in_comp.resize(g.m_comp.size());

	ims_resize(vb, g.num_ver());

	for (size_t comp_idx = 0; comp_idx < g.m_comp.size(); ++comp_idx) {
		auto& c = g.m_comp[comp_idx];

		auto& status = m_points_in_comp[comp_idx];//reference
		status = cardinality::empty;//if empty, it doesn't affect anything

		////////////////////////////////////////////////////////////////////
		//finding one point from each set using fixed points of cycles
		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = g.m_ver_sorted[j + c.idx_sorted];

			
			//turned out to be non-empty
			status = cardinality::point;//contains at least one point

			if (vb[v].defined2()) {
				continue;//processed before or incorrectly
			}

			m_edg_cycle.clear();
			size_t vt = v;
			size_t cycle_from = ims_max;//where the cycle begins

			//search for a cycle
			while (cycle_from == ims_max) {
				//looking for the first suitable edge
				let ne = g.num_edges(vt);

				for (size_t e = 0; e < ne; ++e) {
					let& qe = g.get_edge(vt, e);

					//if the topology information is accurate,
					//then we work only within the component
					if (c.has_self && g.m_ver2com[qe.second] != comp_idx) {
						continue;
					}

					//a valid vertex has at least one valid edge
					//therefore, we will definitely get here and exit the loop
					m_edg_cycle.push_back(qe);
					break;
				}

				vt = m_edg_cycle.back().second;

				if (vb[vt].defined2()) {//previously processed
					cycle_from = m_edg_cycle.size();//outside
				} else {
					//check, maybe we've seen this before
					for (size_t k = 0; k < m_edg_cycle.size(); ++k) {
						if (m_edg_cycle[k].first == vt) {//found a cycle from k to the end ver_cycle
							cycle_from = k;
							break;
						}
					};
				}
			};

			assert(!m_edg_cycle.empty());

			if (cycle_from < m_edg_cycle.size()) {//cycle found
				assert(m_edg_cycle[cycle_from].first == vt);

				//multiply the cycle maps
				pool_ptr CycleMap(eval_pool::ep.get_id_val());
				
				for (size_t k = cycle_from; k < m_edg_cycle.size(); ++k) {
					CycleMap = eval_helpers::edge_mul(
							CycleMap.get(), ri[m_edg_cycle[k].m].m.get(), true);
				}

				vb[vt] = eval_helpers::fixed_point(CycleMap.get());

				if(!vb[vt]){
					status = cardinality::error;
				}
			};

			if (status == cardinality::error) {
				break;
			}

			//spread the point along the chain
			let* pt = vb[vt].get();

			for (let& q : boost::adaptors::reverse(m_edg_cycle)) {

				auto& L = vb[q.first];
				
				if (m_points_in_comp[g.m_ver2com[q.second]] == cardinality::error){
					status = cardinality::error;
					break;
				}
	
				if (!L.defined2()) {
					let& m = ri[q.m].m;

					edge_ball mp(eval_helpers::mul_ball(m.get(), pt));

					//ball is found, create a copy, set radius to 0
					L = eval_helpers::create_ball(mp.center_data(), 0, mp.dim());
				}

				pt = L.get();
			}
		}//for by component vertices

		if (status == cardinality::empty || status == cardinality::error) {
			continue;
		}

		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = g.m_ver_sorted[j + c.idx_sorted];

			let ne = g.num_edges(v);
			let* p = vb[v].center_data();//known component point

			for (size_t e = 0; e < ne; ++e) {
				let& qe = g.get_edge(v, e);

				let tcomp = g.m_ver2com[qe.second];

				if (comp_idx != tcomp) {//from another component, status is known
					status = std::max(status, m_points_in_comp[tcomp]);
				}

				if (status >= cardinality::nzero) {
					break;
				}

				let& m = ri[qe.m].m;
				let& bt = vb[qe.second];

				edge_ball mb = eval_helpers::mul_ball(m.get(), bt.get());
				auto* q = mb.center_data();
				let dim = mb.dim();
				if (vb[v].dim() < dim) {
					vb[v].reset(eval_helpers::extend_real_vector_size(vb[v].get(), dim + 1));
					p = vb[v].center_data();
				}
				add_vec_mul(q, p, -1.0, dim);
				let d = vec_norm(q, dim);

				if (d < ims_num_traits<Real>::almost_zero()) {
					continue;
				}

				if (comp_idx == tcomp) {
					status = cardinality::nzero;
					break;
				}

				status = std::max(status,
					c.has_self ? cardinality::countable : cardinality::finite);
			}
			if (status >= cardinality::nzero)break;
		}
	}
}



