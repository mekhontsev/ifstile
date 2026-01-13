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
#include "dist_solver.h"
#include "eval_helpers.h"
#include "ims_graph_base.h"
#include "ims_val.h"
#include "edge_map.h"

void dist_solver::compute(
	const geom_input_data& d, 
	const Vec& center)
{

	IMS_SCOPE([&] {release_maps(); });


	reset();

	let& ri = d.ri;
	let& vb = d.vb;

	assert(vb[d.root]);

	if (vb[d.root].radius() == 0) {
		m_result.push_back({ center });
		return;
	}

	m_gd.init_gd(ri, vb, *d.gm, d.root);

	let init_size = m_gd.stack.size();

	//fill the beginning
	for (size_t i = 0; i < init_size; ++i) {

		state root_state;

		auto& e0 = root_state.m_elem[0];

		let& si = m_gd.stack[i];

		e0.m = si.m;
		e0.m.get()->add_ref();
		e0.b = si.b;
		e0.b.get()->add_ref();
		e0.ver = si.ver;

		let& b0 = e0.b;

		Real b01 = (b0.center() - center).norm();
		Real sr = b0.radius();

		root_state.m_h = b01 + sr;
		root_state.m_next_to_check = 0;

		m_q.push(root_state);
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

		//with the largest radius
		auto cur_elem = m_q.top(); //copy
		m_q.pop();
		IMS_SCOPE([&]{cur_elem.release();});

		//check with those already inserted
		let mds = min_diam * sqp;

		if (cur_elem.m_h + mds < min_diam) {//it can definitely be cut off
			continue;
		}

		let& ce = cur_elem.m_elem[0];

		let sumr = ce.b.radius();
		if (cur_elem.m_h > 2 * sumr) {


			let mr = ce.b.radius();

			bool del = false;

			for (size_t i = cur_elem.m_next_to_check; i < m_result.size(); ++i) {
				let& v = m_result[i][0];

				//[v0,v1] - [ce0,ce1]
				//a pair of balls of radius mds around v0, v1 + a pair of balls e0, e1


				let dm = (v - ce.b.center()).norm();//distance between segments

				if (mr < mds && dm < mds) {//cur_elem is already precise enough and too close - can be cut off
					del = true;
					break;
				}

				if (cur_elem.m_h + dm * sqp < min_diam) {//cur_elem is too short - can be cut off
					del = true;
					break;
				}


				if (dm < mds + mr) {//it's not clear yet, divide it
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
				r[0] = ce.b.center();

				if (m_result.size() >= d.max_result_size) {
					return;
				}
				continue;
			}

		}

		//divide the largest

		let& qdiv = ce;
		let& qv = d.gm->m_vers[qdiv.ver];

		for (size_t i = 0; i < qv.sz; ++i) {
			let& e = d.gm->m_edges[qv.idx + i];

			state ne;
			
			//fill the second one
			auto& dst = ne.m_elem[0];

			ne.m_next_to_check = cur_elem.m_next_to_check;

			dst.ver = e.second;

			//matrix multiplication
			dst.m = eval_helpers::edge_mul(qdiv.m.get(), ri[e.m].m.get(), true);
			dst.b = eval_helpers::mul_ball(dst.m.get(), vb[dst.ver].get());

			let& b0 = dst.b;

			let d01 = (b0.center() - center).norm();
			let sr = b0.radius();

			ne.m_h = d01 + sr;

			min_diam = std::max(min_diam, d01 - sr);

			m_q.push(ne);

		}

	}
}

