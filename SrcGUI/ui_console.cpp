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
#include "ui_console.h"
#include "ims_window_drag.h"
#include "gui.h"
#include "platform.h"
#include "edit_helper.h"

const char* ws_console::get_title()
{
	return "Console";
}

void ext_console_sync(std::string& str);
void ext_console_clear();

void ws_console::show()
{

	int next_id = 0;

	bool bexec;
	{
		bexec = ims_button("=>", "Perform the action");
	}

	static e_what_print what_print = e_what_print::Definition;
	{
		SAME_LINE();
		ImGui::PushID(next_id++);
		ImGui::PushItemWidth(100.0f * get_ui_scale());

		//e_what_print
		static constexpr auto cb =
		{
			"Definition","Evaluation", "Normal Maps","Projection",
			"Dimension","Geometry","Measure","Subspaces", "Data", "AST", "Execute JS",
		};

		static_assert((size_t)e_what_print::NUMBER_OF_WHAT_PRINT == cb.size());

		ImGui::Combo("", (int*)&what_print, cb.begin(), (int)cb.size(), (int)cb.size());
		ImGui::PopItemWidth();
		ImGui::PopID();
	}

	ImGui::BeginDisabled(m_buf.empty());

	bool bclear;
	{
		SAME_LINE();
		bclear = ims_button("Clear");
	}

	if (bclear) {
		ext_console_clear();
		m_buf.clear();
	}
	let id_con = next_id++;
#ifndef __EMSCRIPTEN__	
	//In the browser, ImGui uses its own clipboard, which is not accessible from the OS.
	int vedit = edit_helper::a_none;
	{
		SAME_LINE();
		if (ims_button("Copy")) { vedit = edit_helper::a_copy; }
	}
	edit_helper::set_action(id_con, vedit);
#endif
	{
		SAME_LINE();
		if (ims_button("Copy All")) { platform::ims_to_clipboard(m_buf); }
	}

	ImGui::EndDisabled();

	{
		SAME_LINE();
		ImGui::Checkbox("Wrap", &m_wrap);
	}


	let ws = ImGui::GetWindowSize();
	ImVec2 sz;

	///////////////////////////////////////////////////////////////////////////
	// Command-line
	if (what_print == e_what_print::ExecJS) {
		bool reclaim_focus = false;
		ImGuiInputTextFlags input_text_flags =
			ImGuiInputTextFlags_CtrlEnterForNewLine |
			ImGuiInputTextFlags_EnterReturnsTrue |
			ImGuiInputTextFlags_EscapeClearsAll|
			ImGuiInputTextFlags_AllowTabInput;

		sz = { ws.x - 2 * ImGui::GetCursorPos().x , ImGui::GetFrameHeight() };

		edit_helper::begin(next_id++);
		let exec = ImGui::InputTextMultiline("", &m_input_buf, sz, input_text_flags);
		edit_helper::end();

		//set_tooltip("Execute JavaScript");
		if (exec || bexec) {
			std::string_view script{ m_input_buf };
			script = boost::algorithm::trim_copy_if(
				script, boost::algorithm::is_any_of(" \t\r\n"));
			if (!script.empty()) {
				std::cout << script << std::endl;
				console_execute(script);
			}
			m_input_buf.clear();
			reclaim_focus = true;
		}

		// Auto-focus on window apparition
		ImGui::SetItemDefaultFocus();
		if (reclaim_focus) {
			ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
		}
	} else if (bexec) {
		console_print(what_print);
	}
	///////////////////////////////////////////////////////////////////////////

	ext_console_sync(m_buf);

	edit_helper::begin(id_con);
	IMS_SCOPE([] {edit_helper::end(true);});

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetStyle().Colors[ImGuiCol_ChildBg]);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyle().Colors[ImGuiCol_ChildBg]);

	ImGuiInputTextFlags flags = ImGuiInputTextFlags_ReadOnly;
	if (m_wrap)flags|= ImGuiInputTextFlags_WordWrap;

	let cp = ImGui::GetCursorPos();
	sz = { ws.x - (cp.x + cp.x), ws.y - (cp.y + cp.x) };

	ImGui::InputTextMultiline("", &m_buf, sz, flags);

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(1);
}
