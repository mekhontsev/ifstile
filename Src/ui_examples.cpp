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
#include "ui_examples.h"
#include "gui.h"
#include "ims_window_drag.h"
#include "samples.h"

#include "ims_file.h"
#include "file_dialog_state.h"

const char* ws_examples::get_title()
{
	return "Examples";
}

void ws_examples::on_create()
{
	auto& smp = get_samples();

	if (smp.get_recent().smp.empty() && smp.m_samples.size() > 1) {
		m_cur_samp_category = 1;
	} else {
		m_cur_samp_category = 0;
	}
}

void ws_examples::show()
{
	int id = 0;

	auto& smp = get_samples();

	let num_cat = smp.m_samples.size();

	auto getter = [](void* vs, int idx)
	{
		return ((samples*)vs)->m_samples[idx].name.c_str();
	};

	ImGui::PushID(id++);
	ImGui::Combo("", &m_cur_samp_category, getter, &smp, (int)num_cat);
	ImGui::PopID();

	bool is_recent = (m_cur_samp_category==0);

	if (is_recent) {
		ImGui::SameLine();
		if (ims_button("x", "Clear")) {
			ims_confirm_dlg("Are you sure you want to clear the list?", [&]() {
				smp.get_recent().smp.clear();
			});
		}

		ImGui::SameLine();
		if (ImGui::Checkbox("Folders", &m_show_folders)) {
			if (m_show_folders) {
				m_folders.clear();
				for (let& q : smp.m_samples[m_cur_samp_category].smp) {
					m_folders.emplace_back(ims_file::get_parent(q.path));
				}
				std::sort(m_folders.begin(), m_folders.end());
				m_folders.erase(std::unique(m_folders.begin(), m_folders.end()), m_folders.end());
			}
		}
	}

	ImGui::Separator();

	ImGui::BeginChild("Elements2");
	ims_window_drag w_ch(true);
	if (m_show_folders) {
		for (let& q : m_folders) {
			ImGui::PushID(id++);
			if (ImGui::Selectable(q.c_str()) && w_ch.allow()) {
				get_fds().open_file("Open", q, { "aifs", "js", "*" },
				[](let& fn)
				{
					open_file(fn, "", false, true, nullptr);
				});
			}
			ImGui::PopID();
		}
	} else {
		for (let& q : smp.m_samples[m_cur_samp_category].smp) {
			ImGui::PushID(id++);
			if (ImGui::Selectable(q.name.c_str()) && w_ch.allow()) {
				try_open_file([&q, is_recent]() {
					open_file(is_recent ? q.path.c_str() : q.name.c_str(),
						std::string((const char*)q.data, q.size),
						false, is_recent, nullptr);
					});
			}
			ImGui::PopID();
		}
	}
	
	ImGui::EndChild();
}
