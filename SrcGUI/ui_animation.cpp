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
#include "ui_animation.h"
#include "gui.h"
#include "ims_settings.h"
#include "file_dialog_state.h"
#include "oper_block.h"
#include "block_class.h"
#include "eval_context.h"
#include "variable.h"


const char* ws_animation::get_title()
{
	return "Animation";
}


void ws_animation::show()
{
	if (!ImGui::BeginTabBar(""))return;
	IMS_SCOPE([] { ImGui::EndTabBar(); });

	if (ImGui::BeginTabItem("Batch"))
	{
		
		ImGui::BeginDisabled(is_batch_in_progress());
		if (ims_button("Render", "Render checked elements")) {
			do_batch_rendering();
		}
		ImGui::EndDisabled();
		
#if 0
		{
			SAME_LINE();
			auto s = (int)s_domain2;
			static constexpr auto cb = { "All","Checked","Current" };

			ImGui::PushItemWidth(80 * get_ui_scale());
			if (ImGui::Combo("###Domain", &s, cb.begin(), (int)cb.size())) {
				s_domain2 = (batch_domain)s;
			}
			ImGui::PopItemWidth();
		}
#endif		
		{
			let& path = get_settings().m_last_picture_folder;
			if (ims_button("Path", "Destination")) {
				get_fds().select_folder("Select folder for rendering", path,
					[](let& dir)
					{
						get_settings().m_last_picture_folder = dir;
					});
			}
			SAME_LINE();
			ImGui::TextUnformatted(path.c_str());
		}

		//ImGui::Checkbox("Random mode", &m_random_mode);

		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Interpolation"))
	{
		if (ims_button("Create frames")) {
			try_open_file([this]()
			{
				do_create_anim(m_time_ref);
			}, false);
		}
		
		
		if (input_size_t("Frames ", &m_num_frames, 0)) {
			m_num_frames = std::max(m_num_frames, (size_t)2);
			m_num_frames = std::min(m_num_frames, (size_t)1'000'000);
		};

		

		ImGui::InputText("Prefix ", &m_anim_prefix);

		
		m_refs_arr.clear();
		let* sr = get_cur_block();
		if (sr) {
			let* g = sr->get_class();

			if (m_time_ref >= g->m_refs.size()) {
				m_time_ref = ims_max;
			}

			
			std::string cur_text;
			if (m_time_ref == ims_max) {
				cur_text = "Auto";
			} else {
				cur_text = g->get_var_name(m_time_ref);
			}
			
			if (ImGui::BeginCombo("Time", cur_text.c_str()))
			{
				if (ImGui::Selectable("Auto", m_time_ref == ims_max)) {
					m_time_ref = ims_max;
				}

				for (size_t i = 0; i < g->m_refs.size(); ++i) {
					
					if (!sr->ctx()->is_geom(i)) continue;

					std::string vname{ g->get_var_name(i) };

					if (ImGui::Selectable(vname.c_str(), m_time_ref == i)) {
						m_time_ref = i;
					}

				}
				ImGui::EndCombo();
			}
		}
		ImGui::EndTabItem();
	}
	
}
