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
#include "flame_calc.h"
#include "block_info.h"
#include "ims_to_string_ex.h"
#include "affine_calc.h"
#include "oper_block.h"
#include "palette.h"
#include "matrix_helper.h"
#include "edge_ball.h"
#include "edge_map.h"

#include "block_graph.h"
#include "eval_info.h"

size_t calc_flame(
	std::ostream& of,
	eval_info& ei,
	eval_context& ec,
	ast_maps& am,
	affine_calc& bc,
	const save_type st,
	const variator_params& vp,
	const palette& pal,
	const background& backg,
	const screen_disk<double>& sd,
	std::span<const oper_block*> vis)
{

	using Real = double;

	block_info bi;
	std::vector<Real> mes;
	DynMat<Real> mat;
	auto bound(sd);

	of << "<flames name = \"IFStile\">\r\n";

	//Gives a flame number based on the vertex number in the graph
	//or ims_max if not used in flame
	std::vector<size_t> ver2flame;
	std::vector<size_t> flame2ver;//reverse
	std::vector<size_t> com2flame;
	std::vector<size_t> flame2com;//components used
	std::vector<uint8_t> fc2fc;//is there a path from one component to another?
	//additional edges fv1->fv2
	std::vector<flame_edge> fedges;

	std::unique_ptr<oper_block> block_sq;

	size_t num_saved = 0;
	for(auto* sr: vis){
	
		if (ims_need_stop())break;

		bound.clear2();
			
		if (vis.size() > 1 && st == save_type::checked && !sr->m_flags.checked) {
			continue;
		}

		check_block(sr);

		if (!sr->can_exists()) {
			continue;
		}

		bi.recalc_graph();

		block_sq = std::make_unique<oper_block>();
		block_sq->inherit_from(*sr, vp, ei.m_opinfo2, false);

		bi.init4(
			*block_sq,
			ec,
			am,
			true,
			ei.m_idata4.get(),
			bc);

		if (!bi.exists()) {
			continue;
		}

		//TODO:
		//m_special.eval_builtins(get_block(), dim_proj(), ec);

		if (!bi.compute_metrics(bc.m_dim_calc)) {
			continue;
		}

		let dim = bi.common_dim_proj();

		if (dim != 2 && dim != 3) {
			continue;
		}

		let& g = bi.get_fg();
		size_t root = sr->get_froot();

		ver2flame.resize(g.num_ver());

		size_t fidx;//number of flame vertices
		if (st == save_type::single && root != ims_max) {
			for (auto& q : ver2flame)q = ims_max;
			std::vector<size_t> stack;//vertices to be processed
			stack.push_back(root);
			fidx = 0;
			ver2flame[root] = fidx++;
			while (!stack.empty()) {
				let v = stack.back();
				stack.pop_back();
				assert(ver2flame[v] != ims_max);

				let ne = g.num_edges(v);
				for (size_t j = 0; j < ne; ++j) {
					let vt = g.get_edge(v, j).second;
					if (ver2flame[vt] != ims_max) {
						continue;
					}
					stack.push_back(vt);
					ver2flame[vt] = fidx++;
				}
			}
		} else {//all vertices
			fidx = ver2flame.size();
			for (size_t k = 0; k < fidx; ++k) {
				ver2flame[k] = k;
			};

			if (root == ims_max) {
				root = sr->get_graph()->ref2fg(sr->find_default_ref());
				if (root == ims_max) {
					assert(false);
					root = 0;//if nothing helps
				}
			}
		}

		if (fidx == 0) {
			continue;
		}

		flame2ver.resize(fidx);
		for (size_t k = 0; k < ver2flame.size(); ++k) {
			let vf = ver2flame[k];
			if (vf != ims_max)flame2ver[vf] = k;
		}
		////////////////////////////////////////////////////////
		//if there are multiple components and component a leads to component b
		//then we add one backward edge from b->a with a compression factor
		//0.5 and a non-existent map
		com2flame.resize(g.m_comp.size());
		for (auto& q : com2flame)q = ims_max;
		size_t cidx = 0;
		for (let v : flame2ver) {
			let c = g.m_ver2com[v];
			if (com2flame[c] != ims_max)continue;
			com2flame[c] = cidx++;
		}
		flame2com.resize(cidx);
		for (size_t k = 0; k < com2flame.size(); ++k) {
			let cf = com2flame[k];
			if (cf != ims_max)flame2com[cf] = k;
		}
		//////////////////////////////////////////////////////////
		fedges.clear();

		fc2fc.resize(cidx * cidx);
		for (auto& q : fc2fc)q = 0;
		for (size_t m = 0; m < flame2com.size(); ++m) {
			let c = flame2com[m];
			let& comp = g.m_comp[c];
			for (size_t k = 0; k < comp.num_ver; ++k) {//over all vertices of the component
				let v = g.m_ver_sorted[k + comp.idx_sorted];

				let ne = g.num_edges(v);
				for (size_t j = 0; j < ne; ++j) {//by all edges of the vertex
					let vt = g.get_edge(v, j).second;
					let ct = g.m_ver2com[vt];
					if (ct == c)continue;
					let mt = com2flame[ct];
					//need an additional backward edge mt->m
					auto& ff = fc2fc[mt * cidx + m];
					if (ff == 1) {
						continue;//already added
					};
					ff = 1;

					flame_edge fe;

					fe.vs = ver2flame[vt];
					fe.vt = ver2flame[v];
					fe.m = ims_max;
					fe.clr_idx = 0;
					fedges.emplace_back(fe);
				}
			}
		}


		let& bb = bi.m_vb[root];

		if (dim == 2) {
			if (bound.empty2()) {
				if (bb.defined2()) {
					auto* cd = bb.center_data();
					bound.c[0] = cd[0];
					bound.c[1] = cd[1];
					bound.r = bb.radius();
				} else {
					bound.c[0] = 0;
					bound.c[1] = 0;
					bound.r = 1;
				}

			}

			assert(!bound.empty2());//since inside the linked component
		} else {

		}

		Real dim_pow = 0;
		for (let& v : flame2ver) {
			let h = bi.m_im.di[g.m_ver2com[v]].H;
			dim_pow = std::max(dim_pow, h);
		}

		/////////////////////////////////////////////////////////

		
		let& ri = bi.m_em;
		mes.resize(ri.size());
		for (size_t k = 0; k < ri.size(); ++k) {
			using std::pow;
			using boost::multiprecision::pow;
			mes[k] = pow(std::abs(ri[k].det_rootn), dim_pow);
		}

		/////////////////////////////////////////////////////////

		let nv = fidx;


		for (size_t j = 0; j < nv; ++j) {//over all vertices of the component
			let v = flame2ver[j];
			let ne = g.num_edges(v);

			for (size_t k = 0; k < ne; ++k) {
				let& e = g.get_edge(v, k);
				flame_edge fe;
				fe.vs = j;
				fe.vt = ver2flame[e.second];
				fe.m = e.m;
				fe.clr_idx = k;
				fedges.emplace_back(fe);
			}
		}


		//The dimensionality module finds the measures for a direct matrix
		//Here we also need to find the measures for a transposed matrix
		mat.setZero(nv, nv);
		const Real aw = 0.5;
		for (let& fe : fedges) {
			let w = (fe.m == ims_max) ? aw : mes[fe.m];
			mat(fe.vt, fe.vs) += w;
		}

		mat -= DynMat<Real>::Identity(nv, nv);

		auto& lu = matrix_helper::get_matrix_helper().m_FullPivLU_generic;
		lu.setThreshold(ims_num_traits<Real>::almost_zero());
		DynMat<Real> ker = lu.compute(mat).kernel();

		for (auto& fe : fedges) {
			let w = (fe.m == ims_max) ? aw : mes[fe.m];
			fe.w = static_cast<double>(ker(fe.vs, 0) * w);
		}

		//zero weights cannot be allowed
		for (auto& fe : fedges) {
			if (fe.w < 1e-5)fe.w = 1.0 / static_cast<double>(fedges.size());
		};

		//flame does not allow duplicate names
		let name = sr->m_name + "_" + std::to_string(sr->m_block_id);

		ims_to_flame(
			of,
			dim,
			name,
			fedges,
			ver2flame[root],
			bi.m_em,
			bound,
			pal,
			backg.data);

		++num_saved;


		of << "\r\n\r\n";
	}

	of << "</flames>";

	return num_saved;
}
