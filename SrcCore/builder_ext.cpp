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
#include "builder_ext.h"
#include "builder.h"
#include "projector.h"

void draw_info_ext::init(const palette& pal, const colorize_params& cp)
{
	m_par = cp;
	m_pal = pal;
}

bool draw_info_ext::get_pixel_raw(Real& dst, const size_t ofs, double shift) const
{
	let& px = m_img(ofs);

	if (px.f <= 0) {
		return false;
	}


	let f = px.f;
	//let f = px.vv;


	if (m_par.type == colorize_params::e_equipotential) {
		dst = log(f) / m_par.params[1] + shift;
		return true;
	}

	assert(m_par.type == colorize_params::e_field_lines);
	
	Real v = atan2(px.vy, px.vx) / boost::math::constants::pi<Real>();//[-1;1]

	let c = shift - floor(shift);

	v = (v + 1) / 2;
	if (v < c)v = v + 1 - c;
	else v = v - c;

	dst = v;

	return true;
}

bool draw_info_ext::get_pixel(pixel_RGBA& rgba, const size_t ix, const size_t iy) const
{
	let ofs = m_img.get_index(ix, iy);

	Real x;
	if (!get_pixel_raw(x, ofs, m_par.shift)) {
		rgba(0) = 0;
		rgba(1) = 0;
		rgba(2) = 0;
		rgba(3) = 0;
		return false;
	}

	palette::color c;

	let sz = m_pal.data.size();
	if (sz == 1) {
		//transition between black
		x -= floor(x);
		if (x > 0.5)x = 1 - x;
		let v = float(2 * x);

		let& src = m_pal.data[0].c;
		c[0] = src[0] * v;
		c[1] = src[1] * v;
		c[2] = src[2] * v;
		c[3] = 1;
	} else {
		m_pal.interpolate(c, x * double(sz));
	}


	rgba(0) = c[0];
	rgba(1) = c[1];
	rgba(2) = c[2];
	rgba(3) = 1;

	return true;
}

void builder_ext::init_draw(draw_info_ext& dst, const DynMat<Real>& L) const
{
	dst.m_img.recreate(m_width, m_height);

	let sz = m_width * m_height;

	let dim = (size_t)L.cols();
	let data_num = dim * 2 + 1;

	DynVec<Real> pc;
	pc.resize(dim);

	for (size_t i = 0; i < sz; ++i) {
		auto& pd = dst.m_img(i);
		let* d = &m_data(i * data_num + dim);

		let f = d[dim];
		pd.f = (float)f;

		if (f <= 0)continue;

		for (size_t k = 0; k < dim; ++k) {
			pc(k) = d[k];
		}


		let v = L * pc;

		pd.vv = (float)pc.norm();

		pd.vx = (float)v(0);
		pd.vy = (float)v(1);
	}
}

void builder_ext::prepare(size_t w, size_t h, 
	const subspace_info<Real> si,
	const screen_params<Real> sp)
{


	m_list.resize(w * h);

	let dim = (size_t)si.basis.rows();
	let data_num = dim * 2 + 1;
	m_data.recreate(m_list.size() * data_num, 1);
	m_width = w;
	m_height = h;

	m_data.for_each([](auto& q) {q = 0; });

	////////////////////////////////////////////////////////////////////////

	list_entry* head = nullptr;

	let a = sp.a * boost::math::constants::pi<double>() / 180;
	let c = cos(a);
	let s = sin(a);
	Eigen::Matrix<Real, 2, 2> m;
	m << c, -s, s, c;

	Eigen::Matrix<Real, 2, 1> center;
	center << sp.x, sp.y;
	center = m * center;
	
	DynMat<Real> R;
	
	if (dim >= 2) {
		R = si.basis * m.transpose();
	} else {
		R = si.basis;
	}

	let scr_x1 = center(0) - sp.ps * w / 2;
	let scr_y1 = center(1) - sp.ps * h / 2;

	DynVec<Real> pc;
	pc.resize(2);
	DynVec<Real> tv;//temporary

	for (size_t iy = 0; iy < h; ++iy) {
		pc(1) = scr_y1 + iy * sp.ps;

		for (size_t ix = 0; ix < w; ++ix) {
			pc(0) = scr_x1 + ix * sp.ps;

			let ofs = iy * w + ix;

			auto* dst = &m_data(ofs * data_num);
			tv.noalias() = R * pc + si.origin;
			for (size_t k = 0; k < dim; ++k) {
				dst[k] = tv(k);
			}

			auto& lst = m_list[ofs];
			lst.data = dst;
			lst.next = head;
			head = &lst;
		}
	};
}

void builder_ext::reserve_memory(size_t num_pix, size_t dim)
{
	m_list.reserve(num_pix);
	let data_num = dim * 2 + 1;
	m_data.reserve(data_num * num_pix);
}

void builder_ext::calc_buffer(
	size_t dim,
	state_stack& ss, 
	const size_t root,
	const Real min_rad, 
	const float quality, 
	const double power)
{
	IMS_SCOPE([&ss] {ss.release_elems(); });
	
	if (m_list.empty()) {
		return;
	}

	//adding a root element
	auto* ce = ss.create_root(root);

	DynVec<Real> tv;
	tv.resize(dim);


	auto& rnfo = ims_stage::get();

	//list to process for the current element
	auto* intB = &m_list.back();//that's how prepare worked.

	m_ext.clear();

	while (ce) {
		if (ims_need_stop())break;

		let cd = ce->depth4;

		if (cd > max_depth) {
			return;
		}

		//join all
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


		auto& bd = ce->b;
		auto* next_ce = ce->next();


		if (!bd.defined2()) {
			m_ext.push_back(nullptr);
		} else {
			let bdr = bd.radius();

			auto rad = quality * bdr;

			//cut the intB list
			auto* lst = intB;
			list_entry* extB = nullptr;
			intB = nullptr;
			while (lst) {
				auto* nxt = lst->next;

				auto* data = lst->data;
				for (size_t i = 0; i < dim; ++i) {
					tv(i) = data[i];
				}

				tv -= bd.center();

				//distance from a point to the center of a ball
				let h = tv.norm();

				if (h < rad) {//a point inside an inflated ball
					//If the ball becomes smaller than the pixel size, then we don't go any further.
					if (rad < min_rad) {
						lst->data[2 * dim] = -1;//sign of emptiness
						//does not fit into any list
					} else {
						lst->next = intB;
						intB = lst;
					}
				} else {
					//sum up in data
					let ml = ce->mes * std::pow(h, -(power + 1));
					for (size_t i = 0; i < dim; ++i) {
						data[i + dim] += tv(i) * ml;
					}
					data[2 * dim] += ml;

					lst->next = extB;
					extB = lst;
				}

				lst = nxt;
			}
			m_ext.push_back(extB);

			if (!intB) {//rolling back
				rnfo.work_add(ce->mes);
				ss.to_heap(ce);
				ce = next_ce;
				continue;
			}
		}
		auto* nce = ce->m_next;
		ce = ss.divide(ce, nullptr);
		for (auto* q = ce; q; q = q->m_next) {
			q->depth4 = cd + 1;
		}
		if (ce)ce->append(nce);
		else ce = nce;
	}//end while
}
