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
#include "gbuffer3d.h"
#include "ims_worker.h"

void gbuffer3d::init(
	float brightness,
	float border_pow,
	float ssao_density,
	float fog_dens
)
{
	m_brightness = brightness;
	m_border_pow = border_pow;
	m_fog_dens = fog_dens;
	m_ssao_pow = ssao_density;

	////////////////////////////////////////////////////////////////////////////
	//luminance compensation for SSAO
	if (m_ssao_pow > 0) {
		size_t num_pixels = 0;
		double avg_ssao = 0;

		m_img.for_each([&](let& s) {
			if (s.empty()) return;
			++num_pixels;
			avg_ssao += std::pow(1.0 - (double)s.ssao, m_ssao_pow);
			});

		if (avg_ssao > 0) {
			m_brightness *= num_pixels / avg_ssao;
		}
	}
}

bool gbuffer3d::get_pixel(pixel_RGBA& rgba, const size_t ix, const size_t iy) const
{
	let& pxl = m_img(ix, iy);
	if (pxl.empty()) {
		return false;
	};


	Vec3 N(pxl.n[0], pxl.n[1], pxl.n[2]);


	//direction from the point to the camera (light source in the camera)
	auto L = proj.back_proj_dir(m_cam, ix, iy);
	L.normalize();

	let pt = m_cam.m_loc + L * pxl.z;

	let NdotL = -N.dot(L);

	double fg = 1;
	if (m_fog_dens > 0) {
		const Vec3 lr = m_cam.m_loc - m_cam.m_ref;
		fg = exp(-pxl.z * m_fog_dens / lr.norm());//from 1 to 0
	}
	rgba(3) = fg;

	if (NdotL <= 0) {//black
		rgba(0) = 0;
		rgba(1) = 0;
		rgba(2) = 0;
		return true;
	};

	let sf = pxl.ssao;
	let ssao_factor = sf > 0 ?
		std::pow(1 - (double)sf, m_ssao_pow) : 1.0;

	let shade =
		NdotL *
		ssao_factor *
		m_brightness *
		std::pow(pxl.border, m_border_pow);

	rgba(0) = pxl.c[0] * shade;
	rgba(1) = pxl.c[1] * shade;
	rgba(2) = pxl.c[2] * shade;
	return true;
}

gbuffer3d::Real gbuffer3d::getz(Real p0, Real p1) const
{
	//this is also a check for infinity and NAN
	if (!(
		p0 >= 0 &&
		p1 >= 0 &&
		p0 < m_img.w() &&
		p1 < m_img.h()
		))
	{
		return 0;
	}

	let offset = m_img.get_index(
		static_cast<size_t>(p0),
		static_cast<size_t>(p1)
	);

	return m_img(offset).z;
}



void gbuffer3d::calc_ssao(
	float ssao_rad_perc,
	unsigned ssao_samples)
{
	let iw = m_img.w();
	let ih = m_img.h();

	let ssao_rad =
		static_cast<unsigned>(ssao_rad_perc * 0.01f * std::max(iw, ih));

	m_ssao_ready = false;

	if (ssao_samples == 0 || ssao_rad == 0) {
		m_img.for_each([](auto& p) {p.ssao = 0; });
		m_ssao_ready = true;
		return;
	}

	///////////////////////////////////////////////////////

	auto& rng = ims_random::getR().rng;

	//create an array of random offsets
	if (m_ssao_kernel.size() != ssao_samples) {
		m_ssao_kernel.resize(ssao_samples);
		std::uniform_real_distribution<double>	d(-1, 1);
		for (unsigned i = 0; i < ssao_samples; ++i) {
			auto& r = m_ssao_kernel[i];
			r << d(rng), d(rng), d(rng);
			r(2) = std::abs(r(2));//upper semicube
			r.normalize();
			r *= double(i + 1) / ssao_samples;
		}
	}

	std::uniform_real_distribution<Real> distr(0, 1);

	using Mat3 = Eigen::Matrix<Real, 3, 3>;

	auto& cth = *ims_worker::get();

	double w = 1.0 / (iw * ih);

	for (size_t iy = 0; iy < ih; ++iy) {
		for (size_t ix = 0; ix < iw; ++ix) {
			if (cth.is_need_stop2()) {
				return;
			}
			cth.work_add(w);

			let ofs = iy * iw + ix;

			auto& pxl = m_img(ofs);
			if (pxl.empty())continue;

			let& pn = pxl.n;
			Vec3 N(pn[0], pn[1], pn[2]);


			Vec3 rvec(distr(rng), distr(rng), 0);
			rvec.normalize();

			Vec3 tangent = rvec - N * rvec.dot(N);
			tangent.normalize();
			Vec3 bitangent = tangent.cross(N);

			Mat3 rotate;
			rotate.col(0) = tangent;
			rotate.col(1) = bitangent;
			rotate.col(2) = N;

			let pixel_size = pxl.z / proj.m_prj[2];
			let radius = ssao_rad * pixel_size;

			double sum = 0;

			auto L = proj.back_proj_dir(m_cam, ix, iy);
			L.normalize();

			let pt = m_cam.m_loc + L * pxl.z;


			for (Vec3 pk : m_ssao_kernel) {
				pk *= radius;
				pk[2] += pixel_size / 2;//offset

				pk = rotate * pk;
				pk += pt;

				Vec3 p;
				if (!proj.get_proj(m_cam, p, pk))continue;
				let sampleDepth = getz(p[0], p[1]);
				if (sampleDepth <= 0)continue;
				let zSamp = (pk - m_cam.m_loc).norm();
				if (sampleDepth < zSamp) {
					sum += 1;
				}
			}
			let val = sum / m_ssao_kernel.size();
			pxl.ssao = (float)(val);
		}
	}

	m_ssao_ready = true;
}


void gbuffer3d::zoom_out_cam(camera<Real>& cam, bool keep_lr) const
{
	let& arr = m_img;
	let sz = arr.w() * arr.h();
	Real min_depth = std::numeric_limits<Real>::max();
	for (size_t i = 0; i < sz; ++i) {
		let d = arr(i).z;
		if (d <= 0)continue;
		min_depth = std::min(min_depth, d);
	}
	if (min_depth >= std::numeric_limits<Real>::max())return;
	typename camera<Real>::Vec3 lr = cam.m_loc - cam.m_ref;
	lr.normalize();
	cam.m_loc += 2 * min_depth * lr;

	if (keep_lr) {
		cam.m_ref += 2 * min_depth * lr;
	}
}

bool gbuffer3d::zoom_in_cam(
	camera<Real>& cam,
	float fcx,
	float fcy,
	float fw,
	float fh,
	bool keep_lr) const
{
	let& arr = m_img;

	using Vec3 = Eigen::Matrix<Real, 3, 1>;

	let cx = static_cast<size_t>(fcx * arr.w());
	let cy = static_cast<size_t>(fcy * arr.h());
	let w = static_cast<size_t>(fw * arr.w());
	let h = static_cast<size_t>(fh * arr.h());


	Real sx1, sx2, sy1, sy2;

	sx1 = sy1 = std::numeric_limits<Real>::max();
	sx2 = sy2 = std::numeric_limits<Real>::lowest();

	cam_proj<Real> cp;
	cp.init(cam, arr.w(), arr.h());


	for (size_t ix = cx; ix < cx + w; ++ix) {
		for (size_t iy = cy; iy < cy + h; ++iy) {
			let az = arr(ix, iy).z;
			if (az <= 0)continue;

			Vec3 p;

			p(0) = (Real(ix) + cp.m_prj(0));
			p(1) = (Real(iy) + cp.m_prj(1));
			p(2) = cp.m_prj(2);
			p.normalize();//pixel direction
			p *= az;//obtained the coordinates of the point in the camera space

			sx1 = std::min(sx1, p(0));
			sy1 = std::min(sy1, p(1));
			sx2 = std::max(sx2, p(0));
			sy2 = std::max(sy2, p(1));

		}
	}

	if (sx1 > sx2) {
		return false;//emptiness
	}

	let qx = (sx1 + sx2) / 2;
	let qy = (sy1 + sy2) / 2;

	typename camera<Real>::Vec3 vec = cam.m.row(0) * qx + cam.m.row(1) * qy;

	typename camera<Real>::Vec3 rl = cam.m_ref - cam.m_loc;

	cam.m_loc += vec;

	Real minz = std::numeric_limits<Real>::max();
	Real tfv = cp.m_tf2_ver;
	Real tfh = tfv * arr.w() / arr.h();


	for (size_t ix = cx; ix < cx + w; ++ix) {
		for (size_t iy = cy; iy < cy + h; ++iy) {
			let az = arr(ix, iy).z;
			if (az <= 0)continue;

			Vec3 p;

			p(0) = (Real(ix) + cp.m_prj(0));
			p(1) = (Real(iy) + cp.m_prj(1));
			p(2) = cp.m_prj(2);
			p.normalize();//pixel direction
			p *= az;//obtained the coordinates of the point in the camera space

			p(0) -= qx;
			p(1) -= qy;

			let dz = std::max(std::abs(p(0)) / tfh, std::abs(p(1)) / tfv);
			minz = std::min(minz, p(2) - dz);

		}
	}

	let offset_vec = minz * cam.m.row(2);
	cam.m_loc += offset_vec;
	if (keep_lr) {
		cam.m_ref = cam.m_loc + rl;
	} else {
		rl.normalize();
		cam.m_ref = cam.m_loc + rl * minz;
	}


	return true;
}
