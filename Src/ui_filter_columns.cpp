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
#include "ui_filter_columns.h"
#include "gui.h"
#include "ims_window_drag.h"

#include "columns.h"
#include "data_column.h"

const char* ws_filter_columns::get_title()
{
	return "Columns";
}

void ws_filter_columns::show()
{
	//which columns we see and their order
	static std::vector<size_t> s_vcol;

	auto& cols = columns::get();

	if (s_vcol.empty()) {
		for (size_t i = 0; i < column_id::NUM_COLS; ++i) {
			s_vcol.emplace_back(i);
		}
	}

	cols.sort_cols_by_vis(s_vcol);//works quickly
	
	let more_than_one_visible = cols.m_col_visible[s_vcol[1]];
		

	//////////////////////////////////////////////////////////////////
	ImGui::TextUnformatted("Columns visibility");
	ImGui::Separator();
	//////////////////////////////////////////////////////////////////

	constexpr std::array sl{ "ID","Description" };
	bool bv = ImGui::BeginTable("ColumnVis", (int)std::size(sl), ImGuiTableFlags_ScrollY);
	if (!bv)return;

	ims_window_drag w_ch(true);

	ImGui::TableSetupScrollFreeze(0, 1);
	for (let* c : sl)ImGui::TableSetupColumn(c, ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
	ImGui::TableHeadersRow();

	int next_id = 0;

	for (size_t i : s_vcol) {
		ImGui::TableNextRow();

		let& q = data_column::g_cols[i];
		
		////////////////////////////////////////////////////////////////////////
		ImGui::TableNextColumn();
		ImGui::PushID(next_id++);

		{
			bool b = cols.m_col_visible[i];
			if (ImGui::Checkbox(q.title, &b) && w_ch.allow()) {
				if (b || more_than_one_visible) {
					cols.m_col_visible[i] = b;
					need_update_vis_cols() = true;
				}
			};
		}

		ImGui::PopID();

		////////////////////////////////////////////////////////////////////////
		ImGui::TableNextColumn();

		ImGui::TextUnformatted(q.description);
	};
	ImGui::EndTable();
}
