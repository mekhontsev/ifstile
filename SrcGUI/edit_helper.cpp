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
#include "edit_helper.h"

namespace ImGui { int g_edit_hook; }

namespace edit_helper {

using key = std::pair<ImGuiID, int>;

ims_static ankerl::unordered_dense::map<key, int> s_actions;
ims_static key s_current;

static key get_key(int id)
{
	return { ImGui::GetID(""), id };
}

void begin(int id)
{
	s_current = get_key(id);
	auto& act = s_actions[s_current];
	if (act > 0) {
		ImGui::g_edit_hook = act;
		act = 0;
	}
	ImGui::PushID(id);
}

void set_action(int id, int a)
{
	auto& act = s_actions[get_key(id)];
	act = std::max(act, a);
}

void end(bool read_only /*= false*/)
{
	IMS_SCOPE([] {
		ImGui::PopID();
		});

	ImGui::g_edit_hook = 0;

	if (!ImGui::BeginPopupContextItem("InputContextMenu")) {
		return;
	}

	auto& act = s_actions[s_current];

	ImGui::BeginDisabled(read_only);
	if (ImGui::Selectable("Cut"))	act = a_cut;
	ImGui::EndDisabled();

	if (ImGui::Selectable("Copy"))	act = a_copy;

	ImGui::BeginDisabled(read_only);
	if (ImGui::Selectable("Paste"))	act = a_paste;
	if (ImGui::Selectable("Select All")) act = a_selall;
	ImGui::EndDisabled();

	ImGui::EndPopup();
}
}
