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
#include "builder3d.h"
#include "builder.h"
#include "projector.h"
#include "lqsort.h"
#include "gbuffer3d.h"
#include "clock_print.h"

////////////////////////////////////////////////////////////////////////////////
#ifdef DEVELOPER_VERSION
void builder3d::print_statistics()
{
	size_t actual_pixels = 0;
	m_img.for_each([&](let& px) {
		if (!px.empty()) {
		++actual_pixels;
		}
	});


	let dnpix = 1.0/double(actual_pixels);

	ims_print("num_pixels = {}\n", m_img.w() * m_img.h());
	ims_print("actual_pixels = {}\n", actual_pixels);
	ims_print("z_updates = {}\n", m_z_updates * dnpix);
	ims_print("n_updates = {}\n", m_n_updates * dnpix);
	ims_print("z_states = {}\n", m_z_states * dnpix);
	ims_print("n_states = {}\n", m_n_states * dnpix);
	ims_print("z_time = {}\n", m_z_time_s);
	ims_print("n_time = {}\n", m_n_time_s);

}
#endif//DEVELOPER_VERSION

void builder3d::reserve_memory(size_t num_pix)
{
	m_img.reserve(num_pix);
}

void builder3d::init_draw(
	gbuffer3d& dst, 
	const subspace_info<Real>& si, 
	const camera<Real>& cam) const
{
	dst.m_ssao_ready = false;
	
	let iw = m_img.w();
	let ih = m_img.h();
	dst.m_img.recreate(iw, ih);
	

	dst.m_cam = cam;
	dst.proj.init(cam, iw, ih);

	///////////////////////////////////////

	let dim = si.get_dim_space();
	DynVec<Real> tn(dim);

	projector P;
	P.R = si.basis;
	P.calc_L_ortho();

	DynVec<Real> tn3(3);

	let sz = iw * ih;

	for (size_t i = 0; i < sz; ++i) {
	
		let& s = m_img(i);
		auto& d = dst.m_img(i);

		if (s.empty()) {
			d.z = 0;//infinity
			continue;
		}

		d.z = s.z;
		d.n = s.pn;

		let m = sqrt(d.n[0] * d.n[0] + d.n[1] * d.n[1] + d.n[2] * d.n[2]);
		d.n[0] /= m;
		d.n[1] /= m;
		d.n[2] /= m;

		/////////////////////////////////////////////////////////
		let ma = std::max(s.s, s.ma);
		let ms = s.ms + s.s;

		//average color
		d.c[0] = s.c[0] / ms;
		d.c[1] = s.c[1] / ms;
		d.c[2] = s.c[2] / ms;

		d.border = ma / ms;

		d.ssao = 0;
	}
}

bool builder3d::calc_buffer(
	state_stack& ss, 
	size_t w, 
	size_t h, 
	const camera<Real>& cam, 
	const float quality, 
	const float thickness, 
	const size_t root,
	const light_params<Real>& ls)
{
	m_img.recreate(w, h);

	cam_proj<Real> proj;
	proj.init(cam, w, h);

	////////////////////////////////////////////////////////////////////////

	pixel* head = nullptr;

	for (size_t iy = 0; iy < h; ++iy) {
		for (size_t ix = 0; ix < w; ++ix) {
			let ofs = iy * w + ix;

			auto& px = m_img(ofs);
			px.clear();

			//three-dimensional direction
			auto pv = proj.back_proj_dir(cam, ix, iy);

			let dn = pv.norm();
			pv.normalize();

			px.pv[0] = pv(0);
			px.pv[1] = pv(1);
			px.pv[2] = pv(2);

			px.pn[0] = px.pn[1] = px.pn[2] = 0;
			px.c[0] = px.c[1] = px.c[2] = 0;

			px.inc = float(thickness / dn);

			px.next = head;
			head = &px;
		}
	};

#ifdef DEVELOPER_VERSION
	m_z_updates = 0;
	m_n_updates = 0;

	m_z_time_s = 0;
	m_n_time_s = 0;

#endif

	//auto* icm = ss.icm;
	//ss.icm = nullptr;
	
#ifdef DEVELOPER_VERSION
	let time_point1 = ims_chrono::now();
#endif
	if (!calc(head, ss, calc_type::surface, quality, thickness, root, cam, ls)) {
		return false;
	};
#ifdef DEVELOPER_VERSION
	m_z_time_s =
		ims_chrono::dif_micro(time_point1, ims_chrono::now()) * 1e-6;
#endif

	
	//ss.icm = icm;

#ifdef DEVELOPER_VERSION
	m_z_states = ss.m_states_explored;
#endif

	if (ims_need_stop()) {
		return true;
	}

	////////////////////////////////////////////////////////////////////////
	head = nullptr;

	//preparation for calculating normals
	m_img.for_each([&head, &cam](auto& px) {
		if (px.empty())return;

		//move the position and reset the normal
		px.pv[0] = cam.m_loc(0) + px.pv[0] * px.z;
		px.pv[1] = cam.m_loc(1) + px.pv[1] * px.z;
		px.pv[2] = cam.m_loc(2) + px.pv[2] * px.z;

		px.pn[0] = px.pn[1] = px.pn[2] = 0;

		//there was nothing yet
		px.clear_color();

		px.next = head;
		head = &px;
	});

	
#ifdef DEVELOPER_VERSION
	let time_point2 = ims_chrono::now();
#endif

	if (!calc(head, ss, calc_type::normals, quality, thickness, root, cam, ls)) {
		return false;
	};

#ifdef DEVELOPER_VERSION
	m_n_time_s =
		ims_chrono::dif_micro(time_point2, ims_chrono::now()) * 1e-6;
#endif

	m_img.for_each([](auto& px) {px.on_change_id(); });

#ifdef DEVELOPER_VERSION
	m_n_states = ss.m_states_explored;
#endif

	return true;
}

bool builder3d::calc(
	pixel* intB, 
	state_stack& ss, 
	calc_type type, 
	const double quality, 
	const double thickness,
	const size_t root,
	const camera<Real> cam,
	const light_params<Real>&)
{

	IMS_SCOPE([&ss] {ss.release_elems(); });

	assert(quality >= 1);

	let& si = *ss.m_psi;


	
	
	projector P;
	P.R = si.basis;
	P.calc_L_ortho();

	DynVec<Real> tv;//temporary

	//adding a root element
	auto* ce = ss.create_root(root);

	//project the centers of the balls in 3D, cutting off along the way
	using ELEM = typename state_stack::elem;
	auto proj = [&P, &si, &cam, &tv, &type](ELEM* q)->bool {
		tv = q->b.center();
		tv -= si.origin;
		
		q->bc = P.L * tv;

		if (P.L.rows() != P.L.cols()) {
			//orthogonal projection of the center of the ball onto the subspace
			tv *= -1;
			tv.noalias() += P.R * q->bc;
			if (tv.norm() >= q->b.radius()) {
				return false;//empty set
			}
		}

		if (type == calc_type::surface) {
			q->bc -= cam.m_loc;
			q->ds = q->bc.norm();
		}
		return true;
	};

	if (ce->b.defined2()) {
		proj(ce);
	}


	auto& cth = *ims_worker::get();

	m_ext.clear();



	size_t id = 0;

	while (ce) {

		if (cth.is_need_stop2())break;

		let cd = ce->depth4;

		//merge
		assert(m_ext.size() >= cd);
		for (size_t i = cd; i < m_ext.size(); ++i) {
			auto* lst = m_ext[i];

			while (lst) {
				auto* nxt = lst->next;

				lst->next = intB;
				intB = lst;

				lst = nxt;
			}
		}
		m_ext.resize(cd);

		////////////////////////////////////////////////////
		//check the ball

		let& bd = ce->b;

		auto* next_ce = ce->next();

		if (!bd.defined2()) {
			m_ext.push_back(nullptr);
		} else {

			let bdr = bd.radius();

			let r_prec = bdr * quality;

			let& weight = ce->mes;

			//cut the intB list
			auto* lst = intB;
			pixel* extB = nullptr;
			intB = nullptr;


			while (lst) {
				auto* nxt = lst->next;

				auto& px = *lst;

				bool need_divide = false;

				Vec3 pv(px.pv[0], px.pv[1], px.pv[2]);

				if (type == calc_type::surface) {

					let& ds = ce->ds;


					////////////////////////////
					//neighborhood of a set
					let eps = ds * lst->inc * m_surf_thick;

					auto eps_th = eps;
					auto th = ce->get_thickness(thickness); 
					if (th > 1)eps_th *= th;

					//no more dividing for this beam
					let ready = r_prec < eps_th;
					////////////////////////////

					let a = ce->bc.dot(pv);

					for (;;) {//will be executed once

						Real r;

						//take the surrounding area
						if (ready) {
							r = eps_th;
						} else {
							r = std::min(bdr + eps_th, r_prec);
						}
						//r+=eps_th;//alternative option


						auto d2 = r * r + a * a - ds * ds;

						if (d2 < 0) {
							//the ray did not intersect with the ball
							break;
						}

						d2 = sqrt(d2);

						if (a + d2 < 0) {
							//the ball is behind the camera - we throw it away
							break;
						}

						let z = a - d2;
						//	ASSUME(h > 0);

						if (!px.empty() && z >= px.z) {
							//too far - discard
							break;
						}

						if (ready) {
							//deep enough, we update the state, but we don't break it down any further
							px.z = z;

#ifdef DEVELOPER_VERSION
							++m_z_updates;
#endif
						} else {
							need_divide = true;
						}

						break;
					}

				} else if (type == calc_type::normals) {

					assert(!px.empty());

					//neighborhood of a set
					let eps = px.z * lst->inc * m_surf_thick;


					auto eps_th = eps;
					let th = ce->get_thickness(thickness);
					if (th > 1)eps_th *= th;


					//vector from the center of the ball to a point on the surface
					Vec3 tn = pv - ce->bc;

					let qn2 = tn.squaredNorm();
					let qn = sqrt(qn2);

					for (;;) {//will be executed once


						if (qn > eps_th * 2 + bdr) {
							break;//too far, we don't take it into account at all
						}


						if (r_prec >= eps) {
							need_divide = true;
							break;//not fine enough, divide further
						}

						//divided enough, take into account

#ifdef DEVELOPER_VERSION
						++m_n_updates;
#endif

						tn *= (weight / qn2);//or qn?
						px.pn[0] += float(tn(0));
						px.pn[1] += float(tn(1));
						px.pn[2] += float(tn(2));


						if (px.id != ce->id3) {
							px.id = (uint32_t)ce->id3;
							px.on_change_id();
						}

						px.s += (float)weight;

						let& c = ce->c.t;

						px.c[0] += float(c[0] * weight);
						px.c[1] += float(c[1] * weight);
						px.c[2] += float(c[2] * weight);

						break;
					}
				} else {//shadows


				}

				/////////////////////////////////
				if (need_divide) {
					lst->next = intB;
					intB = lst;
				} else {
					lst->next = extB;
					extB = lst;
				}

				lst = nxt;
			}

			m_ext.emplace_back(extB);


			if (!intB) {//rolling back
				cth.work_add(ce->mes / 2);
				ss.to_heap(ce);
				ce = next_ce;
				continue;
			}
		}

		//m_ext increased by 1

		////////////////////////////////////////////////////////////
		//go deeper
		auto* nce = ce->m_next;
		ce->m_next = nullptr;

		//divide until the balls contain the camera
		ELEM* ready = nullptr;
		while (ce)
		{
			if (ss.m_heap_size > 10000) {
				return false;
			}
			auto* lce = ce->m_next;

			ce = ss.divide(ce, &id);

			auto* q = ce;
			ce = nullptr;
			for (; q; ) {

				auto* nxt = q->m_next;

				if (!proj(q)) {
					cth.work_add(q->mes/2);
					ss.to_heap(q);
					q = nxt;
					continue;
				}


				if (type == calc_type::surface && q->ds < q->b.radius()) {
					q->m_next = ce;
					ce = q;
				} else {
					q->m_next = ready;
					ready = q;
				}

				q = nxt;
			}

			if (ce)ce->append(lce);
			else ce = lce;
		}
		ce = ready;

		for (auto* q = ce; q; q = q->m_next) {
			q->depth4 = cd + 1;
		}

		if (ce) {
			ce->append(nce);
			//sort res_list, the top of the list should be closest to the camera
			if (type == calc_type::surface) {
				ce = qsort_list(ce, nce, [](let& e1, let& e2) {
					return e1.ds == e2.ds ? &e1 < &e2 : e1.ds < e2.ds;
				});
			}
		} else {
			ce = nce;
		}

	}//end while

	

	return true;
}

/////////////////////////////////////////////////////

void builder3d::pixel::clear()
{
	z = 0;
}

bool builder3d::pixel::empty() const
{
	return z == 0;
}

void builder3d::pixel::clear_color()
{
	c[0] = c[1] = c[2] = s = ma = ms = 0;
	id = std::numeric_limits<decltype(id)>::max();
}

void builder3d::pixel::on_change_id()
{
	ma = std::max(s, ma);
	ms += s;
	s = 0;
}
