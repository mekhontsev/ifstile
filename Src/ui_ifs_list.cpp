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
#include "ui_ifs_list.h"
#include "ims_window_drag.h"
#include "gui.h"
#include "visible_blocks.h"
#include "ifs_list.h"
#include "data_column.h"
#include "columns.h"
#include "finder.h"
#include "oper_block.h"
#include "ims_worker.h"
#include "edit_helper.h"

void ws_ifs_list::scroll_to_row(size_t row)
{
	m_row_to_scroll = row;
}


const char* ws_ifs_list::get_title()
{
	return "IFS List";
};

static void push_diabled() 
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
}


void ws_ifs_list::show()
{
	auto& lst = ifs_list_get();
	if (lst.empty()) {
		ImGui::TextUnformatted("The list is empty");
		return;
	}

	int next_id = 1;

	static e_action m_action = e_action::Uncheck;//TODO: move to the class

	auto& fnd = finder::get();

	auto& vb = get_vb();


	//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));


	////////////////////////////////////////////////////////

	if (ims_button("C", "Select columns")) {
		set_view_mode(ListViewMode::SELCOL);
		return;
	}

	{
		SAME_LINE();
		if (ims_button("R", "Filter rows")) {
			set_view_mode(ListViewMode::SELVIS);
			return;
		}
	}

	////////////////////////////////////////////////////////

	{
		SAME_LINE();
		ImGui::PushItemWidth(80.0f * get_ui_scale());

		//corresponds to e_action
		static constexpr auto cb =
		{
			"Uncheck","Remove","Invert","Interval","Unique",
			"Hide","Unhide","Convert"
		};
		ImGui::PushID(next_id++);
		ImGui::Combo("", (int*)&m_action, cb.begin(), (int)cb.size(), (int)cb.size());
		ImGui::PopID();
		ImGui::PopItemWidth();
	}

	bool bapply;
	{
		SAME_LINE();
		bapply = ims_button("Apply");
	}
	if (bapply) {
		switch (m_action) {
		case e_action::Uncheck:
		{
			std::scoped_lock lock(get_list_lock());
			
			for (let id : lst.m_blocks) {
				auto* q = lst.m_id2data[id].b.get();
				vb.set_checked(q, false);
			}
			break;
		}
		case e_action::Interval:
			vb.check_interval(m_last_checked[0], m_last_checked[1], m_last_checked_act, false);
			m_last_checked_act = !m_last_checked_act;
			break;
		case e_action::Remove:
			if (vb.get_vis_checked() == 0) {
				ims_show_message("No items selected");
			} else {
				if (is_search_started() ||
					get_thread(e_ims_threads::build).is_running() ||
					is_batch_in_progress())
				{
					ims_show_message("Searching or building in progress");
				} else {

					ims_confirm_dlg("Are you sure to remove selected elements?", []() {
						m_action = e_action::Uncheck;
						remove_checked();
						});
				}
			}
			break;
		case e_action::Convert:
		{
			let num_conv = apply_converters();
			ims_show_message_fmt("{} blocks converted", num_conv);
			m_action = e_action::Uncheck;
			break;
		}
		default:
			vb.list_action(data_column::g_cols.data(), m_action);
			break;
		}

	}

	////////////////////////////////////////////////////////

	{
		SAME_LINE();
		ImGui::Text("V:%u H:%u C:%u",
			uint32_t(vb.m_vis_blocks.size()),
			uint32_t(lst.size() - vb.m_vis_blocks.size()),
			uint32_t(vb.m_checked)
		);
	};




	////////////////////////////////////////////////////////////////////////////
	if (lst.empty()) {
		return;
	}

	let search_in_progress = is_search_started();

	//new line
	if (search_in_progress) {
		{
			size_t num = 0;

			if(fnd.m_search_init_mode){
				num = fnd.m_init_index;
			} else{
				for(let& q : fnd.m_bi_map_vec){
					num += q.m_hot_list.size();
				}
			}

			std::array<char, 32> buf;

			const char* title;
			if(num > 0){
				//### - so that it continues to be pressed during a quick update
				fmt::format_to_n(buf.data(), buf.size(), "{:4}###ssw\0", num);
				title = buf.data();
			} else{
				title = "Info";
			}
		
			if (ims_button(title, "Search Statistics")) {
				set_view_mode(ListViewMode::GRAPHS);
			}
		}
		

		{
			SAME_LINE();
			ImGui::PushID(next_id++);
			ImGui::Checkbox("", &m_show_proto);
			ImGui::PopID();
			set_tooltip("Show prototypes at the beginning");
		}

		{
			SAME_LINE();
			ImGui::PushID(next_id++);
			ImGui::Checkbox("", &m_find_scroll);
			ImGui::PopID();
			set_tooltip("Scroll when a new set is found");
		}

		{
			SAME_LINE();
			ImGui::PushID(next_id++);
			ImGui::Checkbox("", &m_find_build);
			ImGui::PopID();
			set_tooltip("Build when a new set is found");
		}

	}

	//new line
	{
		auto* sr = get_cur_block();
		if (sr) {

			edit_helper::begin(next_id++);
			let sz = ImGui::CalcItemWidth() * 0.8f;
			ImGui::PushItemWidth(sz);
			if (ImGui::InputText("", &sr->m_name)) {
				std::replace(sr->m_name.begin(), sr->m_name.end(), ';', '_');
			};
			ImGui::PopItemWidth();
			edit_helper::end();

			{
				SAME_LINE();
				std::array<char, 16> buf;
				fmt::format_to_n(buf.data(), buf.size(), "{}\0", sr->m_block_id);
				if (ims_button(buf.data(), "Scroll to ID")) {
					scroll_to_row(vb.m_cur_block_pos);
				}
			}
		}

	}

	if (!vb.m_vis_blocks.empty()) {

		{
			SAME_LINE();
			if (ims_button("<<")) {
				scroll_to_row(0);
			}
			set_tooltip("To the first");
		}
		{
			SAME_LINE();
			if (ims_button(">>")) {
				let n = vb.m_vis_blocks.size();
				if (n > 0)scroll_to_row(n - 1);
			}
			set_tooltip("To the last");
		}
	}


	ImGui::Separator();

	////////////////////////////////////////////////////////////////////////////


	int tflags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_Reorderable |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | ImGuiTableFlags_NoSavedSettings;

	if (!fnd.m_search_init_mode) {
		tflags |= ImGuiTableFlags_Sortable;
	}

	bool show_table = ImGui::BeginTable("IFSList", (int)column_id::NUM_COLS, tflags);
	if (!show_table)return;

	ims_window_drag w_ch(true);

	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
		check_navi_ex(true);
	}

	
	auto& ls = fnd.m_list_status;

	for (size_t v = 0; v < column_id::NUM_COLS; ++v) {
		let& h = data_column::g_cols[v];
	
		auto q = columns::get().m_col_visible[v];

		bool upd = need_update_vis_cols() || 
			ls == e_list_status::just_loaded ||
			ls == e_list_status::just_search;

		if (ls < e_list_status::just_search) {
			q = q && !h.is_need_search();
		}
		

		int flags = 0;
		if (v == column_id::CH || v == column_id::HD) {
			flags |= ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed;
		}
		if (!q)flags |= ImGuiTableColumnFlags_DefaultHide;

		if (h.def_order) {
			flags |= ImGuiTableColumnFlags_PreferSortAscending;
		} else {
			flags |= ImGuiTableColumnFlags_PreferSortDescending;
		}

		ImGui::TableSetupColumn(h.title, flags);

		//synchronize column visibility

		if (upd) {
			ImGui::TableSetColumnEnabled((int)v, q);
		}

	}

	need_update_vis_cols() = false;

	const size_t hdr_graphs = (m_show_proto && search_in_progress)?
		fnd.m_bi_map_vec.size():0;

	ImGui::TableSetupScrollFreeze(0, 1+int(hdr_graphs));


	ImGui::TableHeadersRow();


	auto* sorts_specs = ImGui::TableGetSortSpecs();

	if (sorts_specs && sorts_specs->SpecsDirty) {
		for (int n = 0; n < sorts_specs->SpecsCount; n++)
		{
			let* sort_spec = &sorts_specs->Specs[n];

			if (ls == e_list_status::just_loaded) {
				//ImGui tries to sort immediately after loading
				//but we need it to be like in the file (by ID) first
				//sort_spec->SortDirection = ImGuiSortDirection_None;//"const" gets in the way
			}else {
				std::scoped_lock lock(get_list_lock());
				columns::get().sort_by_col(sort_spec->ColumnIndex, 
					lst,
					sort_spec->SortDirection == ImGuiSortDirection_Ascending);

				vb.reset_vis_blocks(lst);
				scroll_to_row(vb.m_cur_block_pos);
			}

		}
		sorts_specs->SpecsDirty = false;
	}


	if (ls == e_list_status::just_loaded) {
		ls = e_list_status::proc_loaded;
	}else if (ls == e_list_status::just_search) {
		ls = e_list_status::proc_search;
	}


	if (m_row_to_scroll !=ims_max) {
		//let y = ImGui::GetScrollY();
		let sz = vb.m_vis_blocks.size();
		if (sz > 1) {
			//let my = ImGui::GetScrollMaxY();
			//let ry = double(g_set_block) / (sz - 1);
			//ImGui::SetScrollY(float(my*ry));

			ImGui::SetScrollY(m_item_height * (float)(m_row_to_scroll));

			m_row_to_scroll = ims_max;
		}

	}


	m_height_in_rows = 0;

	auto hovered_id = block_id_max;

	let ctrl_down = is_control_down();

	ImGuiListClipper clipper;

	clipper.Begin((int)(vb.m_vis_blocks.size()+hdr_graphs));



	while (clipper.Step()) {
		for (int ii = clipper.DisplayStart; ii < clipper.DisplayEnd; ii++) {

			oper_block* sr;

			int idx = ii - (int)hdr_graphs;

			if (idx<0) {
				let& bcl = fnd.m_bi_map_vec[ii];
				sr = const_cast<oper_block*>(bcl.m_last_selected);
				if (!sr)sr = const_cast<oper_block*>(bcl.m_base_block);
			}else {
				sr = vb.get_vis(idx);
			}

			if (!sr)break;

			ImGui::TableNextRow();

			////////////////////////////////////////////////////////////////////

			int pop_color = 0;
			if (sr->m_flags.hidden || !sr->can_exists()) {
				push_diabled();
				pop_color = 1;
			}
			IMS_SCOPE([&] {
				if (pop_color > 0)ImGui::PopStyleColor(pop_color);
			});
			////////////////////////////////////////////////////////////////////

			let base_id = ii + next_id;

			ImGui::PushID(base_id);

			bool sel_used = false;

			bool row_clicked = false;


			for (size_t v = 0; v < column_id::NUM_COLS; ++v) {

				bool cvis = ImGui::TableNextColumn();

				if (!cvis) {
					continue;
				}


				if (v == column_id::CH) {
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

					/////////////////////////////////////
#if 0
					int pop_proto = 0;
					if (sr->m_id == fnd.m_cur_proto_id && i_>0 ) {
						ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 1, 1, 1));
						pop_proto = 1;
					}
					IMS_SCOPE([&] {
						if (pop_proto > 0)ImGui::PopStyleColor(pop_proto);
					});
#endif
					/////////////////////////////////////

					bool b = sr->m_flags.checked;
					if (ImGui::Checkbox("", &b) && w_ch.allow()) {
						vb.set_checked(sr, b);

						if (idx >= 0) {
							m_last_checked[0] = m_last_checked[1];
							m_last_checked[1] = (size_t)idx;
							m_last_checked_act = b;
							if (is_shift_down()) {
								vb.check_interval(
									m_last_checked[0],
									m_last_checked[1],
									m_last_checked_act, false);
							}
						}
						
					}
					ImGui::PopStyleVar();
				} else if (v == column_id::HD) {
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
					bool b = sr->m_flags.hidden;
					let ID = base_id + clipper.DisplayEnd + 1;
					ImGui::PushID(ID);
					if (ImGui::Checkbox("", &b) && w_ch.allow()) {
						sr->m_flags.hidden = b;

						if (idx >= 0) {
							m_last_checked[0] = m_last_checked[1];
							m_last_checked[1] = (size_t)idx;
							m_last_checked_act = b;
							if (is_shift_down()) {
								vb.check_interval(
									m_last_checked[0],
									m_last_checked[1],
									m_last_checked_act, true);
							}
						}
					}
					ImGui::PopID();
					ImGui::PopStyleVar();
				} else {

					let& c = data_column::g_cols[v];

					auto cur_id = block_id_max;
					if (c.m_calc_info & data_column::option::contains_id) {
						if (!c.is_need_search() || sr->m_calc_data) {
							cur_id = (block_id_t)c.get_int(*sr, *sr->m_calc_data);
						}
					}
	
					c.get_column_str(*sr, m_cur_text, false);
		
					let disable = cur_id != block_id_max && m_hovered_id == cur_id;

					if (disable) {
						push_diabled();
					}


					if (sel_used) {
						ImGui::TextUnformatted(m_cur_text.c_str());
					} else {
						sel_used = true;

						//so that it continues to be clicked during a quick refresh
						m_cur_text += "###list_item";
						

						row_clicked = ImGui::Selectable(m_cur_text.c_str(),
							idx == (int)vb.m_cur_block_pos,
							ImGuiSelectableFlags_SpanAllColumns);

						if (ImGui::IsItemVisible() && idx >= 0) {
							++m_height_in_rows;
						}

						row_clicked = row_clicked && w_ch.allow();

						if (row_clicked && !ctrl_down) {
							ims_err_reset();
							stop_build_then([idx, sr]() {
								if (idx < 0)	set_block_direct(sr);
								else			set_block_ex(idx);
								do_rebuild_sync();
							});
						}
					}


					if (disable) {
						ImGui::PopStyleColor(1);
					}

					if (cur_id != block_id_max && ImGui::IsItemHovered()) {
						//Problem: When hovering over any part of a row
						//In addition to the IsItemHovered() call we need
						//Another issue occurs: on the first column due to ImGui::Selectable
						hovered_id = cur_id;
						if (row_clicked && ctrl_down) {
							scroll_to_row(vb.find_block_by_id(cur_id));
						}
					}					
				}

			}


			ImGui::PopID();
		}

	}//clipper.Step()

	clipper.End();

	m_hovered_id = hovered_id;

	m_item_height = clipper.ItemsHeight;
	if (m_height_in_rows > 1)--m_height_in_rows;//usually one element is only partially visible

	ImGui::EndTable();

}