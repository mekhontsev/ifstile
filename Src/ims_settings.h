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


enum class window_mode_type : int
{
	full,
	floating,
	left,
	right,
	up,
	down,
};

bool load_settings(const std::string& ini_filename);
bool save_settings(const std::string& ini_filename);

struct ims_setting
{
	bool m_select_fom_corner = true;
	bool m_show_menu=true;
	float m_window_alpha = 1.0f;
	float m_backgr_alpha = 0.5f;
	float m_ui_scale = 1.0f;
	bool m_dark=true;
	size_t m_max_thmb = 4;		//maximum number of thumbnails in larger size
	size_t m_num_render_threads = max_threads();

	std::string m_last_folder;	//saved in the settings
	std::string m_last_picture_folder;	//saved in the settings

	static constexpr float min_ui_scale = 1;
	static constexpr float max_ui_scale = 8;

	static constexpr float min_ui_alpha = 0.2f;
	static constexpr float max_ui_alpha = 1.5f;

	static constexpr float min_bg_alpha = 0;
	static constexpr float max_bg_alpha = 1;

	static constexpr size_t min_thumbnail = 2;
	static constexpr size_t max_thumbnail = 50;

	

	//save each fully rendered image
	//path = path of the last forced save image
	bool	m_save_every_picture = false;

	window_mode_type m_window_mode = window_mode_type::left;
	float m_docked_size = 0;


	static size_t max_threads();

	bool is_docked() const {
		return
			m_window_mode != window_mode_type::full &&
			m_window_mode != window_mode_type::floating;
	}

	bool is_docked(bool left_right) const {
		if (left_right) {
			return
				m_window_mode == window_mode_type::left ||
				m_window_mode == window_mode_type::right;
		} else {
			return
				m_window_mode == window_mode_type::up ||
				m_window_mode == window_mode_type::down;
		}

	}
};


