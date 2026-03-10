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

#include "oper_block.h"
#include "spline.h"
#include "def_number_types.h"
#include "standard_vars.h"


struct animator
{

	using Real = DefNumTypes::Real;
	using Integer = DefNumTypes::Integer;


	struct keyframe
	{
		//filled before initialization
		standard_vars sv;
		oper_block b;

		//////////////////////////////////////////////////////
		builtin_arr ptrs;

		struct real_ctl
		{
			std::vector<double> data;
		};
		
		std::vector<real_ctl> ctls;

		double time = 0;
	};


	//initial check and filling
	//argument - a variable in the graph that serves as time
	bool anim_init(std::vector<keyframe>& kframes, size_t ref_time);

	//ft - relative time [0,1]
	void interpolate(oper_block& dst, double ft);

private:

	struct real_offs
	{
		std::vector<size_t> data;
	};
	std::vector<real_offs> m_offsets;

	//temporary, for interpolation
	oper_block m_bl_dummy;

	
	//////////////////////////////////////////////////////
	//data for interpolation

	
	using aspline_t = aspline<double>;
	

	struct sp_cam2d_t
	{
		aspline_t pos;
		aspline_t a;
	};

	struct sp_cam3d_t
	{
		aspline_t loc_ref, ref, ver, fov;
	};

	struct sp_sect_t
	{
		std::vector<aspline_t> basis_user;
		aspline_t origin;
	};


	aspline_t m_bac;
	aspline_t m_fog;


	std::vector<aspline_t> m_sp_pal;
	sp_cam2d_t	m_sp_cam2d;
	sp_cam3d_t	m_sp_cam3d;
	sp_sect_t	m_sp_sect;

	aspline_t m_sp_zoom;

	//one spline per variable
	std::vector<aspline_t> m_spv;

	//array of all times, duplicates those in keyframes
	std::vector<double> m_time_arr;

	//////////////////////////////////////////////////////

	bool m_imm_pal, m_imm_sec, m_imm_cam2, m_imm_cam3, m_imm_bac;

	standard_vars m_sv;

	std::vector<double> m_temp;

	

};