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
#include "ui_finder.h"
#include "gui.h"
#include "ims_window_drag.h"
#include "finder.h"
#include "block_class.h"
#include "oper_block.h"
#include "ims_worker.h"
#include "columns.h"


void ShowFilterColumns(size_t idx, int& next_id);



void update_num_checked_ex();


static size_t get_max_rule() 
{
	return columns::get().m_rule.size() - ERULE::search_first - 1;
}

void ws_finder::adjust_active_filter()
{
	let mr = get_max_rule();
	if (m_active_filter > mr)m_active_filter = mr;
}

const char* ws_finder::get_title()
{
	return "Finder";
}

void ws_finder::show()
{
	int next_id = 0;

	auto& thr = get_thread(e_ims_threads::search);

	if (thr.is_running()) {
		if (ims_button("Stop")) {
			thr.stop();
		}
	} else {
		if (ims_button("Start")) {
			StartSearch();
		}
	};

	auto& fnd = finder::get();

	{
		SAME_LINE();
		ImGui::PushID(next_id++);
		ImGui::PushItemWidth(80 * get_ui_scale());
		auto& v = fnd.m_search_domain;
		auto s = (int)v;

		static constexpr auto cb ={"All","Checked","Current","None"};
		if (ImGui::Combo("Domain", &s, cb.begin(), (int)cb.size())) {
			v = (search_domain)s;
		}
		ImGui::PopItemWidth();
		ImGui::PopID();
		set_tooltip("Search domain");
	}

	////////////////////////////////////////////////////////////////////////////

	let ws = ImGui::GetWindowSize().x - ImGui::GetStyle().ScrollbarSize;
	ImGui::PushItemWidth(ws - 80 * get_ui_scale());

	if (!ImGui::BeginTabBar("RenderTabs"))return;
	

	if (ImGui::BeginTabItem("General"))
	{
		{
			auto& v = fnd.m_var_par.m_search_rad;
			auto s = (float)v;
			if (ImGui::InputFloat("variance", &s, 0.1f, 1, "%.1f")) {
				v = std::max(s, .0f);
			};

			set_tooltip("Search radius");
		}

		{
			auto& v = fnd.m_var_par.m_kernel_defect;
			if (input_size_t("#var maps", &v)) {
				//TODO: make one for each graph
				//s = std::min(s, (int)N);
			};
			set_tooltip("Change several maps at once");
		}

		{
			auto& v = fnd.m_var_par.m_max_disabled;
			if (input_size_t("#empty maps", &v)) {
				//TODO: make one for each graph
				//s = std::min(s, (int)N);
			};
			set_tooltip("Max number of empty maps");
		}

		{
			auto& v = fnd.m_search_attempts;
			input_size_t("#attempts", &v, 100);
			set_tooltip("Mutate latest found element several times");
		}


		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Filter"))
	{
		adjust_active_filter();
		

		if (get_max_rule() > 0) {
			input_size_t("Rule", &m_active_filter);
		}
		adjust_active_filter();//always do it


		ShowFilterColumns(m_active_filter + ERULE::search_first, next_id);

		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Options"))
	{
		
		{
			//SAME_LINE();
			ImGui::PushID(next_id++);
			ImGui::Checkbox("Check new", &fnd.m_check_new_found);
			set_tooltip("Check newly found");
			ImGui::PopID();
		}

		
		{
			//SAME_LINE();
			ImGui::Checkbox("Brute force", &fnd.m_use_full_search);
			set_tooltip("Exhaustive search");
		}

		{
			//SAME_LINE();
			ImGui::Checkbox("Hide filtered", &fnd.m_hide_filtered);
			set_tooltip("Set 'hidden' flag if new element will be non-visible");
		}

		{
			//SAME_LINE();
			ImGui::Checkbox("Skip maximized", &fnd.m_skip_maximized);
			set_tooltip("Skip 'hot' element if number of isomers is maximized");
		}

		{
			auto& v = fnd.m_store_time_to_file;
			//SAME_LINE();
			bool b=ImGui::Checkbox("Store time", &v);
			set_tooltip("Store discovering time in a file");
			if (b) {
				on_change_store_time_flag(v);
			}
		}


		{
			//SAME_LINE();
			ImGui::Checkbox("Relative", &fnd.m_var_par.m_relative_shift);
			set_tooltip("Use relative translations");
		}

		{
			
			{
				//SAME_LINE();
				if (ims_button("Default", "Reset all search parameters to default")) {
					fnd.set_default();
					columns::get().init_columns();
				}
			}

			if (fnd.m_list_status >= e_list_status::just_search &&
				!is_search_started())
			{
				SAME_LINE();
				if (ims_button("Reset search")) {
					remove_search();
				}
			}

		}

		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Advanced"))
	{

		{
			ImGui::PushID(next_id++);
			auto& v = fnd.m_max_isomers;
			input_size_t("#isomers", &v);
			set_tooltip("Maximum number of isomers per graph");
			ImGui::PopID();
		}

		{
			ImGui::PushID(next_id++);
			float s = fnd.m_virtual_quota;
			if(ImGui::InputFloat("virtual", &s, 0.1f)){
				s = std::max(0.0f, s);
				s = std::min(10.0f, s);
				fnd.m_virtual_quota = s;
			};
			set_tooltip("Quota of virtual elements");
			ImGui::PopID();
		}
	

		{
			ImGui::PushID(next_id++);
			float s = fnd.m_proto_complexity_mul;
			if (ImGui::InputFloat("GCX mul", &s, 1)) {
				s = std::max(0.0f, s);
				s = std::min(10.0f, s);
				fnd.m_proto_complexity_mul = s;
			};
			set_tooltip("Prototype complexity multiplier");
			ImGui::PopID();
		}

		{
			ImGui::PushID(next_id++);
			auto& v = fnd.m_max_bits;
			if (input_size_t("max bits", &v, 32)) {
				v = std::max(v, (size_t)1);
			};
			set_tooltip("Integer precision");
			ImGui::PopID();
		}

		{
			ImGui::PushID(next_id++);
			ImGui::InputFloat("max err", &fnd.m_find_prec, 0.3f);
			set_tooltip("Irrational search (0-disabled)");
			ImGui::PopID();
		}

		

		ImGui::BeginDisabled(thr.is_running());

		{
			auto num_filter = columns::get().m_rule.size() - ERULE::search_first;
			if (input_size_t("#filters", &num_filter)) {
				num_filter += ERULE::search_first;
				columns::get().resize_rules(num_filter);
				adjust_active_filter();
			}
		}

	

		if(ims_button("Isomers", "Check extra isomers")){
			fnd.check_extra_isomers();
			update_num_checked_ex();
		}
		ImGui::EndDisabled();

	

		ImGui::EndTabItem();
	}
	
	ImGui::EndTabBar();
	ImGui::PopItemWidth();
}
