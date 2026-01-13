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

#include "ims_image.h"
#include "geometry.h"

using ims_bitmap = ims_image<struct ims_rgba>;

struct projector;
struct state_stack;

struct draw_info2d
{
	using Real = double;

	using pixel_RGBA = Eigen::Matrix<Real, 4, 1>;

	float m_brightness;
	float m_border_pow;

	float m_mul;
	size_t num_pix;
	float m_mes_avg;
	float m_mes_dis;

	bool m_transparent_mode;

	struct pixel
	{
		std::array<float, 3> c;//color
		float ma;//maximum by different IDs
		float ms;//total for all

		bool empty() const { return ms == 0; };
	};

	ims_image<pixel> m_img;

	void init(
		float brightness,
		float contrast,
		const float border_pow,
		const bool transparent_mode
	);

	bool get_pixel(
		pixel_RGBA& rgba,
		const size_t ix,
		const size_t iy) const;



};


//construct 2D cross-section images
struct builder2d
{
	using Real = double;

	
	struct pixel2d
	{
		using type = float;

		std::array<float, 3> c;	//color
		type ma;//maximum by different IDs
		type ms;//total for all

		type s; //by current ID
		uint32_t id2;

		void clear_color();

		void on_change_id();
	};


	ims_image<pixel2d> m_img2;


	static void draw_lattice(
		ims_bitmap& img,
		size_t numpt,
		size_t node_size,
		const projector& fproj,
		const subspace_info<Real> si,//screen plane, by value
		const screen_params<Real> sp//drawing area, by value
	);


	void init_draw(draw_info2d& dst) const;

	static bool overlap(
		Real r1x1, Real r1y1, Real r1x2, Real r1y2,
		Real r2x1, Real r2y1, Real r2x2, Real r2y2);


	void reserve_memory(size_t num_pix);


	void calc_buffer(
		state_stack& ss,
		const float quality,
		const float thickness,
		const size_t root,		//set to build
		const screen_params<Real> sp);//drawing area, by value

};




