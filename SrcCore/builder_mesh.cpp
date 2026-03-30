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
#include "builder_mesh.h"
#include "builder.h"
#include "voxel_volume.h"
#include "projector.h"

bool builder_mesh::calc_buffer(
	state_stack& ss,
	voxel_volume& sb,
	const float quality,
	const size_t root)
{
	IMS_SCOPE([&ss] {ss.release_elems(); });

	let& si = *ss.m_psi;

	let dim = si.get_dim_space();
	let dpr = si.get_section_dim();//0,1,2

	//adding a root element
	auto* ce = ss.create_root(root);

	size_t id = 0;

	projector proj;
	proj.R = si.basis;
	proj.calc_L_ortho();

	DynVec<Real> pc;//projection of the center onto the plane
	DynVec<Real> tv;//temporary


	auto& rnfo = ims_stage::get();


	let maxr = sb.get_radius();

	while (ce) {

		if (ims_need_stop())
		{
			return false;
		};

		let cd = ce->depth4;

		if (cd > max_depth) {
			return false;
		}


		auto& bd = ce->b;

		auto* next_ce = ce->next();

		if (bd.defined2()) {

			//project the center onto the subspace
			bd.center() -= si.origin;
			pc.noalias() = proj.L * bd.center();

			//does it intersect with the required plane?
			if (dpr != dim) {
				tv.noalias() = proj.R * pc;//orthogonal projection of the center of the ball onto the subspace
				tv -= bd.center();
				if (tv.norm() >= bd.radius()) {
					rnfo.work_add(ce->mes);
					ss.to_heap(ce);
					ce = next_ce;
					continue;
				}
			}

			if (bd.radius() * quality < maxr) {

				let& clr = ce->c.t;


				Eigen::Vector3d bc;
				for (size_t i = 0; i < 3; ++i) {
					bc(i) = i < dpr ? pc(i) : 0;
				}

				Eigen::Vector3d uvw;
				uvw << clr[0], clr[1], clr[2];

				try {
					sb.update(bc, maxr, uvw);
				}
				catch (const std::exception& e) {
					ims_error("Error: {}", e.what());
					return false;
				}

				rnfo.work_add(ce->mes);
				ss.to_heap(ce);
				ce = next_ce;
				continue;
			}
			//divide further
		}


		auto* nce = ce->m_next;
		ce = ss.divide(ce, &id);
		for (auto* q = ce; q; q = q->m_next) {
			q->depth4 = cd + 1;
		}
		if (ce)ce->append(nce);
		else ce = nce;
	}//end while

	return true;
}
