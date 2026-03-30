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

#pragma once

#include "ims_graph.h"
#include "matrix_funcs.h"
#include "dyn_mat_vec.h"


//pows - the degree of the base matrix in each map
template<typename Integer>
void compute_graph_poly(
	std::vector<Integer>& h,
	const ims_graph& ig,
	const size_t comp_idx,//component of the graph
	std::span<const intptr_t> pows)
{

	let& c = ig.m_comp[comp_idx];

	//virtual set numbers
	std::vector<std::vector<size_t>> pow_set(c.num_ver);

	size_t num_ver = 0;//next number
	for (size_t i = 0; i < c.num_ver; ++i) {//by component vertices
		let vs = ig.m_ver_sorted[c.idx_sorted + i];

		let ne = ig.num_edges(vs);

		for (size_t j = 0; j < ne; ++j) {
			let& e = ig.get_edge(vs, j);
			
			if (ig.m_ver2com[e.second] != comp_idx) {
				continue;//only for the current component
			};			

			//list of virtual vertices vt
			auto& pt = pow_set[ig.m_ver_in_comp[e.second]];

			let p = pows[e.m];
			//k^0*S, k^1*S, ... k^(p-1)
			//must fit [0,1,...p-1], 0 is required
			while ((intptr_t)pt.size() < p || pt.empty()) {
				pt.push_back(num_ver++);
			};
		}
	}

	DynMat<Integer> nmat;
	nmat.resize(num_ver, num_ver);
	nmat.setZero();

	//vertices to which the edge from vs goes through id
	std::vector<size_t> stack;
	for (size_t i = 0; i < c.num_ver; ++i) {//by component vertices
		let vs = ig.m_ver_sorted[c.idx_sorted + i];

		let& ps = pow_set[ig.m_ver_in_comp[vs]];

		assert(stack.empty());
		stack.push_back(vs);

		size_t weight = 0;

		while (!stack.empty()) {

			if (weight > c.num_ver * 100) {
				h.clear();
				return;
			}

			let v = stack.back();
			stack.pop_back();

			let ne = ig.num_edges(v);
			for (size_t j = 0; j < ne; ++j) {
				let& e = ig.get_edge(v, j);
			
				if (ig.m_ver2com[e.second] != comp_idx) {
					continue;//only for the current component
				};

				++weight;
				
				let p = pows[e.m];
				if (p <= 0) {
					stack.push_back(e.second);//continue to divide
				} else {
					let& pt = pow_set[ig.m_ver_in_comp[e.second]];
					nmat(ps[0], pt[p - 1]) += 1;
				}
			}
		}
		for (size_t j = 1; j < ps.size(); ++j) {
			nmat(ps[j], ps[j - 1]) += 1;
		}
	}

	h.resize(num_ver + 1);
	h.back() = 1;
	
	if (num_ver == 0)return;

	//here the calculations can take a very long time
	DynMat<Integer> T, nmatB;
	char_poly(nmat, num_ver, nmatB, T, h.data(), &ims_stage::get());
}
