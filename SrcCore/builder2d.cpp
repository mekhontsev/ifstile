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
#include "builder2d.h"
#include "builder.h"
#include "projector.h"
#include "eval_helpers.h"

void builder2d::draw_lattice(
	ims_bitmap& img, 
	size_t lattice_to,
	size_t node_size, 
	const projector& fproj, 
	const subspace_info<Real> si,
	const screen_params<Real> sp)
{
	let w = img.w();
	let h = img.h();
	let hx = sp.ps * w / 2;
	let hy = sp.ps * h / 2;
	let scr_x1 = sp.x - hx;
	let scr_y1 = sp.y - hy;
	//let scr_x2 = sp.x + hx;
	//let scr_y2 = sp.y + hy;
	let scale = Real(1.0) / sp.ps;
	projector proj;
	proj.R = si.basis;
	proj.calc_L_ortho();

	DynVec<Real> pc(2);
	let fdim = fproj.dim_algebraic();
	let sdim = fproj.dim_proj();

	let sz = 2 * lattice_to + 1;
	DynVec<Real> fp(fdim);
	DynVec<Real> hp(sdim);
	std::vector<size_t> vec(fdim, 0);

	for (;;) {
		//draw
		for (size_t i = 0; i < fdim; ++i) {
			fp(i) = int(vec[i]) - int(lattice_to);
		}
		hp.noalias() = fproj.L * fp;//project from phase space

		hp -= si.origin;
		pc.noalias() = proj.L * hp;//project onto a plane

		//screen coordinates
		let fx = (pc(0) - scr_x1) * scale;
		let fy = (pc(1) - scr_y1) * scale;
		let sx = static_cast<int>(floor(fx));
		let sy = static_cast<int>(floor(fy));

		for (int dy = 0; dy < (int)(node_size); ++dy) {
			let iy = sy + dy;
			if (iy < 0 || iy >= (int)h)continue;
			for (int dx = 0; dx < (int)(node_size); ++dx) {
				let ix = sx + dx;
				if (ix < 0 || ix >= (int)w)continue;

				auto& px = img((size_t)ix, (size_t)iy);
				px.r = 255;
				px.g = 255;
				px.b = 255;
				px.a = 255;
			}
		}


		/////////////////////////////////////////////
		size_t idx = 0;
		while (idx < fdim) {
			auto& v = vec[idx];
			v++;
			if (v < sz) {
				break;
			}
			v = 0;
			++idx;
		}
		if (idx == fdim) {
			break;
		}
	}
}

void builder2d::init_draw(draw_info2d& dst) const
{
	////////////////////////////////////////////////////////////
	//calculation of brightness and contrast
	Real sum = 0;
	Real sum2 = 0;
	size_t n = 0;

	dst.m_img.recreate(m_img2.w(), m_img2.h());

	let w = dst.m_img.w();
	let h = dst.m_img.h();

	let sz = w * h;

	for (size_t i = 0; i < sz; ++i) {
		let& q = m_img2(i);

		///////////////////////////////////
		auto& d = dst.m_img(i);
		d.c[0] = float(q.c[0]);
		d.c[1] = float(q.c[1]);
		d.c[2] = float(q.c[2]);

		d.ma = float(q.ma);
		d.ms = float(q.ms);

		///////////////////////////////////

		let v = q.ms;
		if (v > 0) {
			n++;
			sum += v;
			sum2 += v * v;
		}

	}

	

	let ma = sum / Real(n);
	dst.m_mes_avg = (float)ma;
	dst.m_mes_dis = (float)sqrt(std::max(Real(0), sum2 / n - ma * ma));
	dst.num_pix = n;

	//This formula gives a much more accurate result for tiles.
#if 0
	double sq = 0;
	for (size_t i = 0; i < sz; ++i) {
		let& q = m_img2(i);
		auto v= q.ms / ma;
		if (v > 1)v = 1;
		sq += v;
	}
	dst.num_pix = (size_t)sq;
#endif
}

bool builder2d::overlap(Real r1x1, Real r1y1, Real r1x2, Real r1y2, Real r2x1, Real r2y1, Real r2x2, Real r2y2)
{
	// The rectangles don't overlap if
	// one rectangle's minimum in some dimension 
	// is greater than the other's maximum in
	// that dimension.

	return
		r1x1 < r2x2&&
		r2x1 < r1x2&&
		r1y1 < r2y2&&
		r2y1 < r1y2;
}

void builder2d::reserve_memory(size_t num_pix)
{
	m_img2.reserve(num_pix);
}

void builder2d::calc_buffer(
	state_stack& ss,
	const float quality,
	const float thickness,
	const size_t root,
	screen_params<Real> sp)
{
	IMS_SCOPE([&ss] {ss.release_elems(); });
	
	let* si = ss.m_psi;

	let dim = si->get_dim_space();
	let dpr = si->get_section_dim();


	auto* img = &m_img2;


	//adding a root element
	auto* ce = ss.create_root(root);


	size_t id = 0;


	let a = sp.a * boost::math::constants::pi<double>() / 180;
	let c = cos(a);
	let s = sin(a);
	Eigen::Matrix<Real, 2, 2> m;
	m << c, -s, s, c;


	Eigen::Matrix<Real, 2, 1> center;
	center << sp.x, sp.y;
	center = m * center;
	sp.x = center(0);
	sp.y = center(1);

	let w = img->w();
	let h = img->h();
	let hx = sp.ps * w / 2;
	let hy = sp.ps * h / 2;
	let scr_x1 = sp.x - hx;
	let scr_y1 = sp.y - hy;
	let scr_x2 = sp.x + hx;
	let scr_y2 = sp.y + hy;
	let scale = Real(1.0) / sp.ps;

	projector P;
	if (dim >= 2) {
		P.R = si->basis * m.transpose();
	} else {
		P.R = si->basis;
	}

	P.calc_L_ortho();


	DynVec<Real> bc;//projection of the center onto the plane
	DynVec<Real> tv;//temporary

	auto& rnfo = ims_stage::get();


	Real pc0 = 0, pc1 = 0;

	while (ce) {

		if (ims_need_stop())break;

		auto& bd = ce->b;

	

		auto* next_ce = ce->next();

		Real sw = 1;//measure of section

		if (bd.defined2()) {

			if (bd.dim() < dim) {
				bd.reset(eval_helpers::extend_real_vector_size(bd.get(), dim + 1));
			}

			tv = bd.center();
			tv -= si->origin;
			//project the center onto the plane
			
			bc.noalias() = P.L * tv;

			let bdr = bd.radius();

			//does it intersect with the required plane?
			if (dpr != dim) {
				//orthogonal projection of the center of a ball onto a plane
				tv *= -1;
				tv.noalias() += P.R * bc;
				
				let dist = tv.norm();//distance from the center of the ball to the plane
				if (dist >= bdr) {
					rnfo.work_add(ce->mes);
					ss.to_heap(ce);
					ce = next_ce;
					continue;
				}
				sw = dist / bdr;
				//sw = sqrt(1 - sw*sw);
				//sw = 1 - sw;
				//sw = 1; //worst case
				sw = 1 - sw * sw;//best case
			}

			pc0 = bc(0);
			pc1 = bc.size() > 1 ? bc(1) : 0;

			let& cx = pc0;
			let& cy = pc1;

			
			let is_inter = overlap(
				cx - bdr, cy - bdr, cx + bdr, cy + bdr,
				scr_x1, scr_y1, scr_x2, scr_y2);

			if (!is_inter) {
				rnfo.work_add(ce->mes);
				ss.to_heap(ce);
				ce = next_ce;
				continue;
			}
		}


		////////////////////////////////////////////////////////////

		if (bd.defined2() && bd.radius() * quality < sp.ps) {//draw
			
			let bdr = bd.radius();

			/*
			//center of mass
			bd.c.noalias() = cur_elem.m.A*im.me[cur_elem.ver].C;
			bd.c += cur_elem.m.b;
			bd.c -= si.origin;

			//project the center onto the plane
			pc.noalias() = proj.P * bd.c;
			*/
			////////////////////////////////////////////

			//let& clr = pal.get(cur_elem.color_idx).c;
			let& clr = ce->c.t;
			let sx = static_cast<float>((pc0 - scr_x1) * scale);
			let sy = static_cast<float>((pc1 - scr_y1) * scale);

			let weight = (float)ce->mes;



			float ix = floor(sx);
			float iy = floor(sy);

			let th = ce->get_thickness(thickness);
			
			if (th <= 1) {

				float rx = sx - ix;
				float ry = sy - iy;


				struct pixel_data
				{
					float x, y, w;
				};

				//bilinear interpolation
				if (dpr > 1) {
					const std::array<pixel_data, 4> pd =
					{ {
						{ ix,     iy,     (1 - rx) * (1 - ry) },
						{ ix + 1, iy,     rx * (1 - ry) },
						{ ix,     iy + 1, (1 - rx) * ry },
						{ ix + 1, iy + 1, rx * ry },
					} };

					for (let& q : pd) {

						if (q.x < 0 || q.x >= (float)w ||
							q.y < 0 || q.y >= (float)h) {
							continue;
						}


						auto& px = img->get_pixel((size_t)q.x, (size_t)q.y);

						let wx = weight * q.w * float(sw);


						px.c[0] += (float)(clr[0] * wx);
						px.c[1] += (float)(clr[1] * wx);
						px.c[2] += (float)(clr[2] * wx);


						if (px.id2 != ce->id3) {
							px.id2 = (uint32_t)ce->id3;
							px.on_change_id();
						}

						px.s = static_cast<pixel2d::type>(px.s + wx);


					}
				} else {//1D
					const std::array<pixel_data, 2> pd =
					{ {
						{ ix,     0,     (1 - rx) },
						{ ix + 1, 0,     rx },
					} };

					for (let& q : pd) {

						if (q.x < 0 || q.x >= (float)w) {
							continue;
						}

						for (size_t qy = 0; qy < h; ++qy) {
							auto& px = img->get_pixel((size_t)q.x, qy);

							let wx = weight * q.w;

							px.c[0] += (float)(clr[0] * wx);
							px.c[1] += (float)(clr[1] * wx);
							px.c[2] += (float)(clr[2] * wx);


							if (px.id2 != ce->id3) {
								px.id2 = (uint32_t)ce->id3;
								px.on_change_id();
							}

							px.s += wx;
						}

					}
				}
			} else {
				let r = (float)std::max(bdr / sp.ps, th);

				let ith = (float)ceil(r);
				let x1 = std::max(ix - ith, 0.0f);
				let x2 = std::min(ix + ith + 2.0f, (float)w);//not inclusive

				let y1 = std::max(iy - ith, 0.0f);
				let y2 = std::min(iy + ith + 2.0f, (float)h);//not inclusive

				let r2 = float(ith * ith);

				/*
				let ith = (int)ceil(th - 1);
				let x1 = std::max(ix - ith, 0);
				let x2 = std::min(ix + ith + 2, (int)img.m_w);//not inclusive

				let y1 = std::max(iy - ith, 0);
				let y2 = std::min(iy + ith + 2,(int)img.m_h);//not inclusive

				let r2 = float(ith * ith);
				*/

				for (int xx = (int)x1; xx < (int)x2; ++xx) {
					for (int yy = (int)y1; yy < (int)y2; ++yy) {
						let dx = sx - xx;
						let dy = sy - yy;
						let d2 = dx * dx + dy * dy;
						if (d2 >= r2)continue;

						auto& px = img->get_pixel(xx, yy);
						let wx = weight * (r2 - d2) / (r2 * r2);

						px.c[0] += (float)(clr[0] * wx);
						px.c[1] += (float)(clr[1] * wx);
						px.c[2] += (float)(clr[2] * wx);

						if (px.id2 != ce->id3) {
							px.id2 = (uint32_t)ce->id3;
							px.on_change_id();
						}

						px.s += wx;
					}
				}


			}

			//rolling back
			rnfo.work_add(ce->mes);
			ss.to_heap(ce);
			ce = next_ce;

			continue;
		}

		auto* nce = ce->m_next;
		ce = ss.divide(ce, &id);
		if (ce)ce->append(nce);
		else ce = nce;

	}//end while

	img->for_each([](auto& px) {px.on_change_id(); });
}

void builder2d::pixel2d::clear_color()
{
	c[0] = c[1] = c[2] = s = ma = ms = 0;
	id2 = std::numeric_limits<decltype(id2)>::max();
}

void builder2d::pixel2d::on_change_id()
{
	ma = std::max(s, ma);
	ms += s;
	s = 0;
}

void draw_info2d::init(float brightness, float contrast, const float border_pow, const bool transparent_mode)
{
	m_brightness = brightness;
	m_transparent_mode = transparent_mode;
	m_border_pow = border_pow;
	m_mul = m_mes_dis == 0 ? 0 : (contrast / m_mes_dis);
}

bool draw_info2d::get_pixel(pixel_RGBA& rgba, const size_t ix, const size_t iy) const
{
	let& s = m_img(ix, iy);

	if (s.empty()) {
		return false;
	}

	//brightness multiplier
	let b = m_brightness * (1 + (s.ms - m_mes_avg) * m_mul);

	//darkening/lightening of borders
	let h = b * std::pow(s.ma / s.ms, m_border_pow);

	//average color
	let q0 = s.c[0] / s.ms;
	let q1 = s.c[1] / s.ms;
	let q2 = s.c[2] / s.ms;

	if (m_transparent_mode) {
		rgba(0) = q0;
		rgba(1) = q1;
		rgba(2) = q2;
		rgba(3) = h;
	} else {
		rgba(0) = q0 * h;
		rgba(1) = q1 * h;
		rgba(2) = q2 * h;
		rgba(3) = 1;
	}

	return true;
}
