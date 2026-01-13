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
#include "diam_solver.h"
#include "eval_helpers.h"
#include "ims_graph_base.h"
#include "ims_val.h"
#include "edge_map.h"


void diam_solver::compute(const geom_input_data& d)
{

	IMS_SCOPE([&] {release_maps(); });


	reset();

	let& ri = d.ri;
	let& vb = d.vb;

	auto vbr = vb[d.root];
	if (vbr.radius() == 0) {
		m_result.push_back({ vbr.center(), vbr.center(),});
		return;
	}

	
	m_gd.init_gd(ri, vb, *d.gm, d.root);

	let init_size = m_gd.stack.size();

	//fill in the pairs
	for (size_t i = 0; i < init_size; ++i) {
		for (size_t j = i; j < init_size; ++j) {

			state root_state;

			auto& e0 = root_state.m_elem[0];
			auto& e1 = root_state.m_elem[1];

			let& si = m_gd.stack[i];
			e0.ver = si.ver;
			e0.m = si.m;
			e0.m.get()->add_ref();
			e0.b = si.b;
			e0.b.get()->add_ref();
			
			let& sj = m_gd.stack[i];
			e1.ver = sj.ver;
			e1.m = sj.m;
			e1.m.get()->add_ref();
			e1.b = sj.b;
			e1.b.get()->add_ref();

			let& b0 = e0.b;
			let& b1 = e1.b;

			Real b01 = (b0.center() - b1.center()).norm();
			Real sr = b0.radius() + b1.radius();

			root_state.m_h = b01 + sr;
			root_state.m_next_to_check = 0;

			m_q.push(root_state);
		}
	}

	m_gd.release_maps();

	const Real sqp = sqrt(d.eps);

	//those who less than min_diam are not interested for us
	Real min_diam = 0;

	while (!m_q.empty()) {

		if (ims_need_stop()) {
			break;
		}

		if (m_q.size() > d.max_queue_size) {
			return;
		}

		//pair with the largest diameter
		auto cur_elem = m_q.top();//copy
		m_q.pop();
		IMS_SCOPE([&]{cur_elem.release();});

		//check with already inserted
		let mds = min_diam * sqp;

		if (cur_elem.m_h + mds < min_diam) {//it can definitely be cut off
			continue;
		}

		let& ce = cur_elem.m_elem;

		let sumr = ce[0].b.radius() + ce[1].b.radius();
		if (cur_elem.m_h > 2 * sumr) {


			let mr = std::max(ce[0].b.radius(), ce[1].b.radius());

			bool del = false;

			for (size_t i = cur_elem.m_next_to_check; i < m_result.size(); ++i) {
				let& v = m_result[i];

				//[v0,v1] - [ce0,ce1]
				//a pair of balls of radius mds around v0, v1 + a pair of balls e0, e1


				let d00 = (v[0] - ce[0].b.center()).norm();
				let d10 = (v[1] - ce[0].b.center()).norm();
				let dm0 = std::min(d00, d10);

				let d01 = (v[0] - ce[1].b.center()).norm();
				let d11 = (v[1] - ce[1].b.center()).norm();
				let dm1 = std::min(d01, d11);

				let dm = std::max(dm0, dm1);//distance between segments

				if (mr < mds && dm < mds) {//cur_elem is already precise enough and too close - can be cut off
					del = true;
					break;
				}

				if (cur_elem.m_h + dm * sqp < min_diam) {//cur_elem is too short - can be cut off
					del = true;
					break;
				}


				if (dm < mds + mr) {//it's not clear yet, let's divide it
					//TODO: it can't get here for some reason
					break;
				}

				//definitely far away, no need to compare with this one anymore, let's move on to the next one
				cur_elem.m_next_to_check = i + 1;
			}


			if (del) {
				continue;
			}

			if (2 * sumr < d.eps * min_diam) {//deep enough
				auto& r = m_result.emplace_back();
				r[0] = ce[0].b.center();
				r[1] = ce[1].b.center();

				if (m_result.size() >= d.max_result_size) {
					return;
				}
				continue;
			}

		}

		//divide the largest
		const size_t div_dix = ce[0].b.radius() < ce[1].b.radius() ? 1 : 0;

		let& qdiv = ce[div_dix];
		let& qv = d.gm->m_vers[qdiv.ver];

		for (size_t i = 0; i < qv.sz; ++i) {
			let& e = d.gm->m_edges[qv.idx + i];

			state ne;

			ne.m_elem[0] = ce[1 - div_dix];//copy - do not split
			ne.m_elem[0].m.get()->add_ref();
			ne.m_elem[0].b.get()->add_ref();

			//fill the second one
			auto& dst = ne.m_elem[1];

			ne.m_next_to_check = cur_elem.m_next_to_check;

			dst.ver = e.second;

			//matrix multiplication
			dst.m = eval_helpers::edge_mul(qdiv.m.get(), ri[e.m].m.get(), true);
			dst.b = eval_helpers::mul_ball(dst.m.get(), vb[dst.ver].get());

		
			let& b0 = ne.m_elem[0].b;
			let& b1 = ne.m_elem[1].b;

			let d01 = (b0.center() - b1.center()).norm();
			let sr = b0.radius() + b1.radius();

			ne.m_h = d01 + sr;

			min_diam = std::max(min_diam, d01 - sr);

			m_q.push(ne);
		}

	}
}
