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



struct ws_ifs_list : public window_state
{
	const char* get_title() override;
	void show() override;

	void scroll_to_row(size_t row);

	bool m_find_scroll = false;
	bool m_find_build = false;
	bool m_show_proto = false;

	//the line to scroll to
	size_t m_row_to_scroll = ims_max;

	//for page_up, page_down
	int m_height_in_rows = 0;




private:

	//the block the user hovers the mouse over
	size_t m_hovered_id = ims_max;

	//height of one element in the list
	float m_item_height = 0;

	size_t m_last_checked[2] = { 0,0 };
	//last action - unchecking the box
	bool m_last_checked_act = false;

	
	std::string m_cur_text;


	void show_prototypes() {



	}

};



