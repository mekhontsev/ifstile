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

////////////////////////////////////////////////////////////////////////////

struct render_params 
{
	palette m_palette;

	background m_background;

	//current coloring algorithm
	colorize_params m_colorize;

	//additional parameters for other modes
	std::array<colorize_params::additional, colorize_params::e_numpar> m_colorize_var;

	/////////////////////////////////////////////
	//can be saved in the environment block

	float m_brightness;
	float m_contrast;
	int	  m_oversamp;
	float m_quality;
	float m_thickness;//thickness in pixels
	float m_border_pow;
	bool  m_inv_mode;


	float m_ssao_rad_perc;
	float m_ssao_density;
	int	  m_ssao_samples;

	int	  m_resolution[2] = { 1920,1080 };

	/////////////////////////////////////////////
	
	int		m_mesh_resolution = 128;
	bool	m_mesh_colors = true;
	bool	m_use_window_res = true;

	void reset_render_params()
	{
		m_background.reset();

		m_brightness = 0.8f;
		m_contrast = 0.2f;
		m_oversamp = 0;
		m_quality = 2;
		m_thickness = 1;//thickness in pixels
		m_border_pow = 1;
		m_inv_mode = false;

		m_ssao_rad_perc = 2;
		m_ssao_density = 2;
		m_ssao_samples = 500;

		m_colorize.reset();

		for (size_t i = 0; i < colorize_params::e_numpar; ++i) {
			colorize_params::set_default((colorize_params::EPAR)i, m_colorize_var[i]);
		}
	};

	void adjust_quality()
	{
		m_quality = std::max(m_quality, 1.0f);
		m_quality = std::min(m_quality, 16.0f);
	};

	void adjust_mesh_resolution()
	{
		m_mesh_resolution = std::max(m_mesh_resolution, 32);
		m_mesh_resolution = std::min(m_mesh_resolution, 2048);
	}


};
