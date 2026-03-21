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
#include "ui_window.h"
#include "geometry.h"
#include "def_number_types.h"

struct location_state 
{
	using Real = DefNumTypes::Real;
	camera_ex m_xcam;
	subspace_info<Real> m_si;
	bool m_lock_dist_target = false;
	Real m_zt = 1;
};

struct standard_vars;
struct ws_location : public window_state
{
	using Real = DefNumTypes::Real;

	const char* get_title() override;
	
	void show() override;

	void on_hide() override { m_cliked_state = cliked_state::idle; };

	//use dimension 2 for rendering
	bool m_force2D = false;

	void from_mouse(const camera_ex& xc, std::string& status, Eigen::Vector3d p, bool clicked, bool is2d);

	location_state m_state;
	location_state m_state_saved;



	Real get_zt() const
	{
		return m_state.m_lock_dist_target ? m_state.m_zt : 1;
	}

	enum class cliked_state 
	{
		idle, //idle mode
		wait_point, //waiting for the user to select a point
		point_ready, //the user has selected a point, it needs to be taken into account (m_p[0])
	};

	cliked_state m_cliked_state = cliked_state::idle;

	double m_square = 0;
	
	Eigen::Vector3d m_p[2];

	void on_change(standard_vars& sv, bool reset, bool c, bool si_ch);
};
