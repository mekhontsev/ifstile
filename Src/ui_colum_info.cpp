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
#include "gui.h"
#include "ims_window_drag.h"
#include "data_column.h"
#include "columns.h"

void ShowFilterColumns(size_t r, int& next_id)
{
	//which columns we see and their order
	static std::vector<size_t> s_vcol;
	
	if (s_vcol.empty()) {
		for (size_t i = 0; i < column_id::NUM_COLS; ++i) {
			if (data_column::g_cols[i].has_number()) {
				s_vcol.emplace_back(i);
			}
		}
	}

	columns& cols = columns::get();
	cols.sort_cols(s_vcol, r);//works quickly

	////////////////////////////////////////////////////////////////////////////

	constexpr std::array sl{ "Column","From","To" };
	bool bv = ImGui::BeginTable("RowFilter", (int)std::size(sl), ImGuiTableFlags_ScrollY);
	if (!bv)return;
	ims_window_drag w_ch(true);
	
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn(sl[0], ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
	ImGui::TableSetupColumn(sl[1]);
	ImGui::TableSetupColumn(sl[2]);
	ImGui::TableHeadersRow();


	let ws = ImGui::GetWindowSize().x - ImGui::GetStyle().ScrollbarSize;
	let off = 50 * get_ui_scale() + ImGui::GetStyle().ColumnsMinSpacing;
	let iw = (ws - off) / 2 - 15 * get_ui_scale();




	for (let i : s_vcol) {
		ImGui::TableNextRow();

		let& q = data_column::g_cols[i];

		auto& fl = cols.m_rule[r][i];


		////////////////////////////////////////////////////////////////////////
		ImGui::TableNextColumn();
		ImGui::PushID(next_id++);

		{
			bool b = fl.filter;
			if (ImGui::Checkbox(q.title, &b) && w_ch.allow()) {
				fl.filter = b;

				//you can't disable GCX for the first search filter
				if (i == column_id::GCX &&	r == ERULE::search_first) {
					fl.filter = true;
				}
			}
		}
		set_tooltip(q.description);

		ImGui::PopID();


		////////////////////////////////////////////////////////////////////////


		if (i == column_id::CH || i == column_id::HD) {

			ImGui::TableNextColumn();
			ImGui::PushID(next_id++);
			{
				bool b = fl.bval;
				if (ImGui::Checkbox("", &b) && w_ch.allow()) {
					fl.bval = b;
				};
			}

			ImGui::PopID();
			ImGui::TableNextColumn();
			//empty column
		} else if (i == column_id::CT) {

			static constexpr auto cb = 
			{
				"None","Weak","Regular","Positive","Strong"
			};
			
			for (auto& fq : fl.ilim) {

				ImGui::TableNextColumn();

				ImGui::PushID(next_id++);
				ImGui::PushItemWidth(iw);

				auto vx = (int)fq;
				let b = ImGui::Combo("", &vx, cb.begin(), (int)cb.size());
				if (b && w_ch.allow()) {
					fq = vx;
				}
				ImGui::PopItemWidth();
				ImGui::PopID();
			}

		} else if (q.get_int) {

			int step = 1;

			if (i == column_id::GCX) {
				step = 1000;
			}

			for (auto& fq : fl.ilim) {

				ImGui::TableNextColumn();
				ImGui::PushID(next_id++);
				ImGui::PushItemWidth(iw);

				auto vx = (int)fq;
				if (ImGui::InputInt("", &vx, step) && w_ch.allow()) {
					if (i == column_id::GCX) {
						vx = std::max(vx, 0);
					}
					fq = vx;
				}
				ImGui::PopItemWidth();
				ImGui::PopID();

			}

		} else if (q.get_float) {
			for (auto& fq : fl.lim) {

				ImGui::TableNextColumn();
				ImGui::PushID(next_id++);
				ImGui::PushItemWidth(iw);

				double vx = fq;
				if (input_double(vx)/* && w_ch.allow()*/) {
					fq = vx;
				};
				ImGui::PopItemWidth();
				ImGui::PopID();

			}
		} else {
			assert(false);
		}


	};
	ImGui::EndTable();
};

