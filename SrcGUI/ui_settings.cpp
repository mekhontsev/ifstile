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
#include "ui_settings.h"
#include "gui.h"
#include "ims_settings.h"
#include "report_params.h"

const char* ws_settings::get_title()
{
	return "Settings";
}

void ws_settings::show()
{
	auto& s = get_settings();
	
	if (ImGui::BeginTabBar("SettingsBar", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("General")) {


			{
				float scale = get_ui_scale();
				let b=ImGui::DragFloat("UI Scale",
					&scale, 0.005f, 
					s.min_ui_scale, 
					s.max_ui_scale, "%.1f"); // scale everything
				if (b) {
					set_ui_scale(scale);
				}
			}

			static constexpr auto cb = 
			{
				"Full","Floating","Left", "Right", "Top","Bottom"
			};
			let b = ImGui::Combo("Window", 
				(int*)&s.m_window_mode, cb.begin(),(int)cb.size());
			if (b) {
				if (s.m_window_mode != window_mode_type::full) {
					reset_window_width();
				}
			}

			{
				auto q = s.m_max_thmb;
				if (input_size_t("Thumbnail", &q)) {
					q = std::max(q, s.min_thumbnail);
					q = std::min(q, s.max_thumbnail);
					on_change_max_thumb(q);
				};
			}


			{
				auto q = s.m_num_render_threads;
				if (input_size_t("Threads", &q)) {
					q = std::max(q, size_t(1));
					q = std::min(q, ims_setting::max_threads());
					s.m_num_render_threads = q;
				};

				ImGui::SetItemTooltip("Number of threads used when batch rendering.");
				
			}

			if (!s.is_docked()) 
			{
				ImGui::DragFloat("UI Alpha", &s.m_window_alpha, 0.005f, s.min_ui_alpha, s.max_ui_alpha, "%.2f");
				ImGui::DragFloat("BG Alpha", &s.m_backgr_alpha, 0.005f, s.min_bg_alpha, s.max_bg_alpha, "%.2f");
			}

			if (ImGui::Checkbox("Dark theme", &s.m_dark)) {
				set_dark_theme(s.m_dark);
			}
			{
				SAME_LINE();
				if (ImGui::Checkbox("Max Viewport", &s.m_max_viewport)) {
					redraw_gui(1);
				};
			}
			{
				SAME_LINE();
				ImGui::Checkbox("Corner zoom", &s.m_select_fom_corner);
			}

			ImGui::Separator();

			
			ImGui::Checkbox("Automatically save every picture", &s.m_save_every_picture);
			ImGui::SetItemTooltip("%s", get_settings().m_last_picture_folder.c_str());
			

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Custom IFS")) {

			auto& r = get_report_params();

			static constexpr auto cb =
			{
				"None","Positive dim","Max dim"
			};

			{
				auto rf = static_cast<int>(r.filer);
				if (ImGui::Combo("Pre filter", &rf, cb.begin(), (int)cb.size())) {
					r.filer = static_cast<report_params::filter_type>(rf);
				};
			}
			{
				auto rf = static_cast<int>(r.filer_post);
				if (ImGui::Combo("Post filter", &rf, cb.begin(), (int)cb.size())) {
					r.filer_post = static_cast<report_params::filter_type>(rf);
				};
			}

			{
				auto& v = r.max_inter;
				auto q = static_cast<int>(v);
				if (ImGui::InputInt("#Inters", &q)) {
					q = std::max(q, 2);
					v = (size_t)q;
				};
			}

			{
				bool q = r.only_max_inters;
				if (ImGui::Checkbox("Only max inters", &q)) {
					r.only_max_inters = q;
				}
			}
#if 0
			{
				SAME_LINE();
				bool q = r.only_strong_intres;
				if (ImGui::Checkbox("Strong", &q)) {
					r.only_strong_intres = q;
				}
			}
#endif
			ImGui::Separator();

			{
				bool q = r.intersections;
				if (ImGui::Checkbox("Intersections", &q)) {
					r.intersections = q;
				}
			}
			{
				bool q = r.connections;
				if (ImGui::Checkbox("Connections", &q)) {
					r.connections = q;
				}
			}
			{
				bool q = r.neighbourhoods;
				if (ImGui::Checkbox("Neighborhoods", &q)) {
					r.neighbourhoods = q;
				}
			}
			{
				bool q = r.neighbourhoods_graph;
				if (ImGui::Checkbox("Neighborhoods graph", &q)) {
					r.neighbourhoods_graph = q;
				}
			}
			{
				bool q = r.relators;
				if (ImGui::Checkbox("Relators", &q)) {
					r.relators = q;
				}
			}
			{
				bool q = r.nboundary;
				if (ImGui::Checkbox("Boundary", &q)) {
					r.nboundary = q;
				}
			}

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}
