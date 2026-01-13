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

struct ws_animation : public window_state
{
	const char* get_title() override;
	void show() override;

	size_t m_num_frames = 600;

	std::string m_anim_prefix;

	enum class batch_domain
	{
		All = 0, //for all in the list
		Checked = 1, //only for those checked at launch
		Current = 2, //randomly vary the parameters of the current
	};

	batch_domain s_domain2 = batch_domain::Checked;
	//bool m_random_mode = false;

	//index of the variable used as time
	size_t m_time_ref = ims_max;

	std::vector<size_t> m_refs_arr;


};

