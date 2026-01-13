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



struct projector;
struct state_stack;
struct gbuffer3d;

struct builder3d
{
	using Real = double;
	using Vec3 = Eigen::Matrix<Real, 3, 1>;
	
#ifdef DEVELOPER_VERSION
	size_t m_z_updates = 0;
	size_t m_z_states = 0;
	size_t m_n_updates = 0;
	size_t m_n_states = 0;

	double m_z_time_s = 0;
	double m_n_time_s = 0;

	void print_statistics();
#endif


private:

	struct pixel
	{
		void clear();
		bool empty() const;

		std::array<Real, 3> pv;//start of the ray

		Real z;		//current depth
		pixel* next;

		std::array<float, 3> pn; //normal
		std::array<float, 3> c;	 //color

		float inc;	//beam cone angle
		float ms;
		float ma;
		float s;
		uint32_t id;

		void clear_color();
		void on_change_id();
	};

	//external depending on depth
	std::vector<pixel*> m_ext;

	ims_image<pixel> m_img;

	static constexpr Real	m_surf_thick = 0.7;

public:
	////////////////////////////////////////////////////////////////////////////
	void reserve_memory(size_t num_pix);

	void init_draw(gbuffer3d& dst,
		const subspace_info<Real>& si,
		const camera<Real>& cam) const;

	//what are we calculating
	enum class calc_type
	{
		surface,
		normals,
		shadows,
	};

	//construct 3D projection images for the vertex ver
	bool calc_buffer(
		state_stack& ss,
		size_t w,
		size_t h,
		const camera<Real>& cam,
		const float quality,
		const float thickness,
		const size_t root,	//which set
		const light_params<Real>& ls);

	//construct a 3D projection cross-section for the root vertex
	//returns false if the camera is inside,
	//returns true if interrupted by the user
	bool calc(
		pixel* intB,
		state_stack& ss,
		calc_type type,
		const double quality,
		const double thickness,
		const size_t root,	//what kind of set are we building?
		const camera<Real> cam,			//by value
		const light_params<Real>&);

};

