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

#include "pch.h"
#include "ui_sel_visible.h"
#include "gui.h"
#include "ims_info.h"
#include "visible_blocks.h"
#include "columns.h"


void ShowFilterColumns(size_t idx, int& next_id);

const char* ws_sel_visible::get_title()
{
	return "Visible";
};

void ws_sel_visible::on_hide()
{
	std::scoped_lock lock(get_list_lock());
	get_vb().reset_vis_blocks(ifs_list_get());
}
void ws_sel_visible::show()
{
	ImGui::TextUnformatted("Rows visibility");
	ImGui::Separator();

	int next_id = 0;
	ShowFilterColumns(ERULE::view, next_id);
}
