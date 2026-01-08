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
#include "geometry.h"
#include "ims_image.h"

struct gbuffer3d
{
	using Real = double;

	using pixel_RGBA = Eigen::Matrix<Real, 4, 1>;
	using Vec3 = Eigen::Matrix<Real, 3, 1>;
	using Mat3 = Eigen::Matrix<Real, 3, 3>;

	camera<Real> m_cam;
	cam_proj<Real> proj;

	double m_border_pow;
	double m_brightness;

	bool m_ssao_ready = false;


	void calc_ssao(
		float ssao_rad_perc,
		unsigned ssao_samples
	);

	double m_fog_dens;

	//random points in the upper hemisphere
	std::vector<Vec3> m_ssao_kernel;
	double m_ssao_pow;


	struct pixel
	{
		double z;

		std::array<float, 3> n;//normal
		std::array<float, 3> c;//color

		float border;
		float ssao;

		bool empty() const { return z == 0; };
	};
	ims_image<pixel> m_img;


	void init(
		float brightness,
		float border_pow,
		float ssao_density,
		float fog_dens
	);

	bool get_pixel(
		pixel_RGBA& rgba,
		const size_t ix,
		const size_t iy) const;

	//get depth by screen coordinates
	Real getz(Real p0, Real p1) const;


	void zoom_out_cam(camera<Real>& cam, bool keep_lr) const;


	bool zoom_in_cam(
		camera<Real>& cam,
		float fcx,
		float fcy,
		float fw,
		float fh,
		bool keep_lr)  const;

};

