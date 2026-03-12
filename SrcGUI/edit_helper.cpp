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
