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
#include "affine_subspace.h"
#include "matrix_helper.h"
#include "ims_graph.h"
#include "ims_val.h"
#include "edge_ball.h"
#include "edge_map.h"
#include "eval_pool.h"
#include "eval_helpers.h"

using Real = affine_builder::Real;

//slightly ineffective
static bool mul_generic(Real* dst, const Real* src, const ims_val* m, size_t dim) 
{
	edge_ball b(eval_pool::ep.get_vector_real(dim + 1));
	auto* s = b->p_r();
	s[0] = 0;
	std::copy(src, src + dim, s + 1);
	b = eval_helpers::mul_ball(m, b.get(), true);
	if (!b)return false;//cut off
	let* d = b->p_r();
	std::copy(d + 1, d + 1 + dim, dst);
	return true;
}

size_t affine_builder::compute(
	const ims_graph& g, 
	size_t dim,
	std::span<const edge_map> ri,
	std::span<const edge_ball> vb,
	const Real eps,
	size_t max_subspaces)
{

	m_data.clear();

	let rows = dim + 1;

	m_points.resize(rows * 2 * rows);//two full spaces

	auto& lu = matrix_helper::get_matrix_helper().m_FullPivLU_generic;

	lu.setThreshold(eps);

	m_data.resize(g.num_ver());

	for (size_t v = 0; v < g.num_ver(); ++v) {
		if (g.is_ver_empty(v))continue;

		auto& vd = m_data[v];
		vd.next_check = 0;
		auto& sb = vd.own;
		let comp_idx = g.m_ver2com[v];
		let& c = g.m_comp[comp_idx];

		if (!c.has_self) {
			sb.idx = 0;
			sb.num = 0;
			vd.lim_check = 0;
			continue;
		}

		if (!vb[v].defined2()) {
			return 0;
		}
		let* src = vb[v].center_data();
		let idx = m_points.size();
		m_points.resize(idx + rows);
		auto* dst = &m_points[idx];
		
		let d = vb[v].dim();
		assert(d <= dim);
		std::copy(src, src + d, dst);
		for (size_t i = d; i < dim; ++i) {
			dst[i] = 0;
		}
		dst[dim] = 1;//expanding the space


		sb.idx = idx;
		sb.num = 1;
		vd.lim_check = 1;
	}

	//at this stage, for each vertex of the self-referencing components,
	//one proper subspace consisting of exactly one point is created
	//expand each subspace using only strong connectivity

	for (size_t comp_idx = 0; comp_idx < g.m_comp.size(); ++comp_idx) {
		let& c = g.m_comp[comp_idx];
		if (!c.has_self)continue;

		
		for (;;) {
			if (ims_need_stop()) {
				return 0;
			}

			//expand the subspace once, acting on each iteration
			//by exactly one successive point from each edge

			for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
				let v = g.m_ver_sorted[j + c.idx_sorted];

				auto& vd = m_data[v];
				auto& sb = vd.own;

				//copy the existing subspace for expansion
				let* src = &m_points[sb.idx];
				let sz = sb.num * rows;

				std::copy(src, src + sz, get_temp());


				let ne = g.num_edges(v);
				for (size_t e = 0; e < ne; ++e) {
					let& qe = g.get_edge(v, e);
					let vt = qe.second;

					if (g.m_ver2com[qe.second] != comp_idx) {
						continue;//only for the component itself
					}

					let& vdt = m_data[vt];
					let& sbt = vdt.own;

					let& m = ri[qe.m].m;
					
					//act on points that have not yet been acted on...
					if (vdt.next_check >= vdt.lim_check)continue;

					auto* pt_src = &m_points[sbt.idx + vdt.next_check * rows];

					let csz = sb.num * rows;
					size_t nsz = csz + rows;

					

					auto* t = get_temp();
					if (!mul_generic(t + csz, pt_src, m.get(), dim)) {
						return 0;
					}
					t[nsz - 1] = 1;

					let rank = (size_t)lu.compute(
						EMat(get_temp(), rows, sb.num + 1)).rank();

					if (rank > sb.num) {//managed to expand

						assert(rank == sb.num + 1);

						sb.idx = m_points.size();
						sb.num = rank;

						assert(rank * rows == nsz);

						m_points.resize(sb.idx + nsz);
						std::copy(get_temp(), get_temp() + nsz, &m_points[sb.idx]);
					}
				}

			}


			bool complete = true;
			for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
				let v = g.m_ver_sorted[j + c.idx_sorted];

				auto& vd = m_data[v];

				if (vd.next_check < vd.lim_check) {//really checked vd.next_check
					++vd.next_check;
				}
				if (vd.lim_check < vd.own.num) {
					++vd.lim_check;
				}
				if (vd.next_check < vd.lim_check) {
					complete = false;
				}
			}
			if(complete)break;


		}//while (!complete) 


#ifndef NDEBUG
		//all component sets must have the same proper dimension
		size_t common_num = ims_max;
		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = g.m_ver_sorted[j + c.idx_sorted];
			let& sb = m_data[v].own;
			if (common_num == ims_max) {
				common_num = sb.num;
			}
			else {
				assert(sb.num == common_num);
			}
		}
#endif // !NDEBUG

	}//comp_idx
	


	//here, each vertex of a self-referencing component has a unique
	//subspace maximally expanded by iterating over its own components

	//now, in topological sorting order, we "wind" the subspaces
	//of other components, constantly removing duplicates
	for (size_t comp_idx = 0; comp_idx < g.m_comp.size(); ++comp_idx) {
		auto& c = g.m_comp[comp_idx];
		
		
		//add subspaces from external components, combining each of them
		//with our own
		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = g.m_ver_sorted[j + c.idx_sorted];

			auto& vd = m_data[v];

			let& sb = vd.own;

			if (sb.num>0) {
				//copy our own subspace for expansion
				let* src = &m_points[sb.idx];
				let sz = sb.num * rows;			
				std::copy(src, src + sz, get_temp());
			}

			let ne = g.num_edges(v);
			for (size_t e = 0; e < ne; ++e) {
				let& qe = g.get_edge(v, e);
				let vt = qe.second;

				if (g.m_ver2com[qe.second] == comp_idx) {
					continue;//only for other components
				}

				//combine each external with its own
				let& vdt = m_data[vt];
				
				let& m = ri[qe.m].m;

				//across all subspaces of the one we depend on



				for (let& sbt : vdt.m_sbs) {
					size_t tsz = sb.num * rows;

				
					size_t rank = sb.num;

					//over all reference points of the subspace
					for (size_t k = 0; k < sbt.num; ++k) {
						let dst_idx = tsz;
						tsz = dst_idx + rows;
						

						auto* pt_src = &m_points[sbt.idx +k* rows];

						if (!mul_generic(get_temp() + dst_idx, pt_src, m.get(), dim)) {
							return 0;
						}
						get_temp()[tsz - 1] = 1;
				
						rank = (size_t)lu.compute(
							EMat(get_temp(), rows, rank +1)).rank();
						tsz = rank * rows;
					}

					//m_points_temp contains a new subspace, insert it
					if (tsz > sb.num * rows) {

						append_subspace(rows, rank, v);

					}
				}
			}
		}


		//provide each set with a non-empty set of subspaces
		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = g.m_ver_sorted[j + c.idx_sorted];

			auto& vd = m_data[v];

			auto& sb = vd.m_sbs;
			
			if (sb.empty()) {
				sb.emplace_back(vd.own);
				vd.next_check = 1;//we've already checked our own
			}else {
				vd.next_check = 0;
			}
			vd.lim_check = sb.size();
		}


		for (;;) {
			if (ims_need_stop()) {
				return 0;
			}
			for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
				let v = g.m_ver_sorted[j + c.idx_sorted];

				let ne = g.num_edges(v);
				for (size_t e = 0; e < ne; ++e) {
					let& qe = g.get_edge(v, e);
					let vt = qe.second;

					if (g.m_ver2com[qe.second] != comp_idx) {
						continue;//only for the component itself
					}

					let& vdt = m_data[vt];
				

					let& m = ri[qe.m].m;


					//across all subspaces of the one we depend on
					for (size_t k = vdt.next_check; k < vdt.lim_check; ++k) {
						let& sbt = vdt.m_sbs[k];
						if (sbt.num==0)continue;

						//put the subspace image here
						//map all the points in turn
						for (size_t i = 0; i < sbt.num; ++i) {

							let idx = i*rows;
							
							if (!mul_generic(
								get_temp() + idx,
								&m_points[sbt.idx + idx],
								m.get(),
								dim))
							{
								return 0;
							}

							get_temp()[idx+rows-1] = 1;
						}

						append_subspace(rows, sbt.num, v);

					}
				}
			}

			bool complete2 = true;
			
			for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
				let v = g.m_ver_sorted[j + c.idx_sorted];
				auto& vd = m_data[v];
				vd.next_check = vd.lim_check;
				vd.lim_check = vd.m_sbs.size();
				if (vd.next_check < vd.lim_check) {
					complete2 = false;
				}
				if (vd.lim_check > max_subspaces) {
					return rows;//maximum rank
				}
				
			}
			if (complete2) {
				break;
			}

		}//infinite loop for the current component


#ifndef NDEBUG
		//all component sets must have the same dimension
		size_t common_num = ims_max;
		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = g.m_ver_sorted[j + c.idx_sorted];

			let& vd = m_data[v];

			size_t max_num=0;
			for (let& q : vd.m_sbs) {
				max_num = std::max(max_num, q.num);
			}
			if (common_num == ims_max) {
				common_num = max_num;
			}else {
				assert(common_num == max_num);
			}
		}
#endif // !NDEBUG

		
	
	}//comp_idx

	size_t ret = 0;
	for (let& q : m_data) {
		for (let& s : q.m_sbs) {
			ret = std::max(ret, s.num);
		}
	}
	return ret;
}

//Now we need to check that
//1) the new one is not contained in any previously created ones
//2) delete all old ones that contain the new one

bool affine_builder::append_subspace(
	size_t rows,
	size_t cols, 
	size_t dst_ver)
{
	let sbt_sz = cols * rows;

	auto& sb = m_data[dst_ver].m_sbs;

	for (auto& sbc : sb) {
		if (sbc.num == 0) {
			continue;//previously deleted
		}

		auto* it = &m_points[sbc.idx];

		std::copy(it, it + sbc.num * rows, get_temp() + sbt_sz);

		auto& lu = matrix_helper::get_matrix_helper().m_FullPivLU_generic;

		let rank = (size_t)lu.compute(
			EMat(get_temp(), rows, cols + sbc.num)).rank();

		if (rank > std::max(cols, sbc.num)) {
			continue;//check further
		}

		if (rank <= sbc.num) {//the new is contained in the existing
			return false;
		}

		//the existing is contained in the new
		assert(rank <= cols);

		sbc.num = 0;//delete the existing one
		//but we'll check further
	}

	//std::erase_if(sb, [](let& q) {return q.num == 0;});

	subspace ns;
	ns.idx = m_points.size();
	ns.num = cols;

	//it's better not to use std::back_inserter because we copy the beginning of the array to the end
	m_points.resize(ns.idx + sbt_sz);
	std::copy(get_temp(), get_temp() + sbt_sz, m_points.data() + ns.idx);

	sb.emplace_back(ns);

	return true;
}