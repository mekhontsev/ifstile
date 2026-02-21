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
#include "pool_ptr.h"

struct ims_val;

struct ws_creator : public window_state
{
	const char* get_title() override;
	void show() override;

	void create_ifs3();

	void show_2d_creator();

	//d - type (structure or array)
	//v - value to edit, can be null
	void show_ui_for_val(const ims_val* d, pool_ptr& v);

	int next_id = 0;
	std::string m_cur_name;
	pool_ptr m_constructor_value;
};
