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

#include "palette.h"
#include "geometry.h"
#include "ims_image.h"

struct state_stack;

struct draw_info_ext
{
	using Real = double;
	palette m_pal;
	colorize_params m_par;

	using pixel_RGBA = Eigen::Matrix<Real, 4, 1>;


	struct pixel
	{
		float f,vv,vx, vy;//field
	};

	ims_image<pixel> m_img;

	void init(
		const palette& pal,
		const colorize_params& cp);



	bool get_pixel_raw(
		Real& dst,
		const size_t ofs,
		double shift) const;

	bool get_pixel(
		pixel_RGBA& rgba,
		const size_t ix,
		const size_t iy) const;

};



struct builder_ext
{
	using Real = double;

	const size_t max_depth = 100000;

	ims_image<Real> m_data;
	size_t m_width;
	size_t m_height;

	////////////////////////////////////////////////////////////////////////////


	void init_draw(draw_info_ext& dst, const DynMat<Real>& L) const;
	
	struct list_entry
	{
		Real* data;//coordinates + sum
		list_entry* next;
	};

	//make a list for processing
	void prepare(
		size_t w, size_t h,
		const subspace_info<Real> si,//by value
		const screen_params<Real> sp);//by value

	void reserve_memory(size_t num_pix, size_t dim);



	//construct 2D projection images for the root vertex
	void calc_buffer(
		size_t dim,
		state_stack& ss,
		const size_t root,//set for building
		const Real min_rad,
		const float quality,
		const double power
	);


private:

	//external depending on depth
	std::vector<list_entry*> m_ext;

	std::vector<list_entry> m_list;
};




