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
#include "ui_clipboard.h"
#include "gui.h"

const char* ws_clipboard::get_title()
{
	return "Clipboard";
}

void ws_clipboard::show()
{
	ImGui::OpenPopup(get_title());

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(
		center,
		ImGuiCond_Always,
		ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal(get_title(), nullptr,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		return;
	}
	IMS_SCOPE([] {ImGui::EndPopup(); });

	int next_id = 0;

	if (ims_button("Copy current", "", &next_id)) {
		do_copy_to_clipboard(m_as_url, m_merge_parents);
		m_show = false;
	}
	ImGui::SameLine();
	if (ims_button("All", "Copy entire list", &next_id)) {
		do_copy_all(m_as_url);
		m_show = false;
	}

	ImGui::Checkbox("As URL", &m_as_url);
	set_tooltip("As a URL to paste as a hyperlink");
	ImGui::SameLine();
	ImGui::Checkbox("Merge", &m_merge_parents);
	set_tooltip("Combine with parents into one block");

	ImGui::Separator();

	if (ims_button("Paste", "And add to the end of the list", &next_id)) {
		if (do_from_clipboard()) {
			m_show = false;
		}
	}

	ImGui::SameLine();

	if (ims_button("Cancel", "", &next_id)) {
		ImGui::CloseCurrentPopup();
		m_show = false;
	}

}

bool ws_clipboard::close()
{
	if (!m_show) {
		return false;
	}
	m_show = false;
	redraw_gui(1);
	return true;
}

