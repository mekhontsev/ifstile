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
#include "ui_search_session.h"
#include "gui.h"
#include "ims_window_drag.h"
#include "finder.h"
#include "visible_blocks.h"
#include "oper_block.h"
#include "ims_chrono.h"

const char* ws_search_session::get_title()
{
	return "Search statistics";
}



void ws_search_session::show()
{
	if (!is_search_started()) {
		ImGui::TextUnformatted("Search status: stopped");
		return;
	}

	let& fnd = finder::get();
	auto& vb = get_vb();

	if (!fnd.m_search_init_mode) {
	
		let time_elapsed_ms = fnd.m_start_time.to_now_ms();

		let time_elapsed = double(time_elapsed_ms) / 1000;

		let& na = fnd.m_num_attempts;

		let avg_time_ms = (na == 0) ? 0.0 :	time_elapsed * 1000 / na;

		
		ImGui::Text("%zu (%.2f ms)[%s]",
			na,
			avg_time_ms, 
			ims_chrono::fmt_ms_to_hour_minutes_seconds(time_elapsed_ms).buf.data());

		set_tooltip("Attempts and avg time");
	}


	{
		SAME_LINE();
		if (ims_button("xH", "Check hot")) {
			for (let& v : fnd.m_bi_map_vec) {
				for (auto* q : v.m_hot_list) {
					vb.set_checked(const_cast<oper_block*>(q), true);
				}
			}
		}
	}
	{
		SAME_LINE();
		if(ims_button("xD", "Check Domain")){
			for(let& v : fnd.m_bi_map_vec){
				for(auto* q : v.m_domain_checked){
					vb.set_checked(q, true);
				}
			}
		}
	}
	{
		SAME_LINE();
		if (ims_button("+H", "Add the checked blocks to the hot list")) {
			auto& vec = finder::get().m_bi_map_vec;
			for (auto* q : get_vb().m_vis_blocks) {
				if (!q->m_flags.checked)continue;
				let idx = finder::get().get_search_index(q);
				if (idx != ims_max) {
					vec[idx].add_hot(q);
				}
			}
		}
	}
	{
		SAME_LINE();
		if (ims_button("SP", "Set the current block as a prototype")) {
			let* cb = get_cur_block();
			if (cb) {
				finder::get().m_next_proto = cb;
			}
		}
	}

	constexpr std::array sl
	{ 
		"Graph ID",
		"Found", 
		"OSC",
		"Isomers",
		"Complicated", 
		"Overflow", 
		"Hot", 
		"Hot time",
		"Virtual", 
		"Elements", 
		"Domain" 
	};

	bool bv = ImGui::BeginTable("GraphList", (int)std::size(sl), 
		ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable);
	if (!bv)return;

	ims_window_drag w_ch(true);

	ImGui::TableSetupScrollFreeze(0, 1);
	for (let* c : sl)ImGui::TableSetupColumn(c);
	ImGui::TableHeadersRow();


	
	std::string name;

	for (let& v : fnd.m_bi_map_vec) {

		name = v.m_base_block->str_id4();

		size_t domain;
		switch (fnd.m_search_domain)
		{
		default:
			domain = 0;
			break;
		case search_domain::All:
			domain = v.m_domain_all.size();
			break;
		case search_domain::Checked:
			domain = v.m_domain_checked.size();
			break;
		case search_domain::Current:
			domain = 1;
			break;
		}


		ImGui::TableNextRow();

		ImGui::TableNextColumn(); 
		
		bool b = ImGui::Selectable(name.c_str(),
			false,
			ImGuiSelectableFlags_SpanAllColumns);
		if (b && v.m_last_selected) {
			let idx = vb.find_block_by_id(v.m_last_selected->m_block_id);
			if (idx != ims_max) {
				set_block_and_build(idx);
			}
		}


		ImGui::TableNextColumn(); ImGui::Text("%d", (int)v.m_num_found);
		ImGui::TableNextColumn(); ImGui::Text("%d", (int)v.m_num_separated);
		ImGui::TableNextColumn(); ImGui::Text("%d", (int)v.m_num_duplicated);
		ImGui::TableNextColumn(); ImGui::Text("%d", (int)v.m_num_complicated);
		ImGui::TableNextColumn(); ImGui::Text("%d", (int)v.m_num_overflowed);
		ImGui::TableNextColumn(); ImGui::Text("%d", (int)v.m_hot_list.size());

		{
			ImGui::TableNextColumn();

			if(v.m_hot_used){
				let time_elapsed_ms = v.m_hot_time.to_now_ms();
				ImGui::TextUnformatted(
					ims_chrono::fmt_ms_to_hour_minutes_seconds(time_elapsed_ms).buf.data());
			}
	
		}


		ImGui::TableNextColumn(); ImGui::Text("%d", (int)v.m_num_virtual);
		ImGui::TableNextColumn(); ImGui::Text("%d", (int)v.m_domain_all.size());
		ImGui::TableNextColumn(); ImGui::Text("%d", (int)domain);
	}
	ImGui::EndTable();
}
