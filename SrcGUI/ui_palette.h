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

struct ws_palette : public window_state
{
	const char* get_title() override;
	void show() override;

	void show_in_rand_mode();

	//for random change
	bool m_rand_mode = false;
	std::array<float, 2> m_H_range{ 0.0f,	1.0f };
	std::array<float, 2> m_S_range{ 0.75f,	0.75f };
	std::array<float, 2> m_V_range{ 1.0f,	1.0f };
	std::array<float, 2> m_A_range{ 1.0f,	1.0f };
};

