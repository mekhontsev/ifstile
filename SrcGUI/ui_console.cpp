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

namespace ImGui { extern int g_edit_hook; }



const char* ws_console::get_title()
{
	return "Console";
}

void ws_console::clear_console()
{
	m_buf.clear();
}

void ws_console::show()
{
	int next_id = 0;


	ImGui::BeginDisabled(m_buf.empty());

	if (ims_button("Clear")) { m_buf.clear(); }
#ifndef __EMSCRIPTEN__	
	//In the browser, ImGui uses its own clipboard, which is not accessible from the operating system.
	int vedit = 0;
	{
		SAME_LINE();
		if (ims_button("Copy")) { vedit = 2; }
	}
	ImGui::g_edit_hook = vedit;
	IMS_SCOPE([] {ImGui::g_edit_hook = 0; });
#endif
	{
		SAME_LINE();
		if (ims_button("Copy All")) { platform::ims_to_clipboard(m_buf); }
	}


	ImGui::EndDisabled();
#if 0
	{
		SAME_LINE();
		ImGui::Checkbox("Wrap", &m_wrap);
	}
#endif
	bool bprint;
	{
		SAME_LINE();
		bprint = ims_button("Print:");
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
			"Dimension","Geometry","Measure","Subspaces", "Data", "AST"
		};

		ImGui::Combo("", (int*)&what_print, cb.begin(), (int)cb.size(), (int)cb.size());
		ImGui::PopItemWidth();
		ImGui::PopID();
	}

#if 0
	static std::string user_input_value;
	ImGui::PushID(next_id++);
	if (what_print == 6) {
		SAME_LINE();
		ImGui::InputText("", &user_input_value);
	}
	ImGui::PopID();
#endif

	if (bprint) {
		console_print(what_print);
	}

	auto str = get_con_data(false);
	m_buf += str;
	str = get_con_data(true);
	m_buf += str;


	auto ws = ImGui::GetWindowSize();
	let cp = ImGui::GetCursorPos();
	ws.x -= cp.x + cp.x;
	ws.y -= cp.y + cp.x;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetStyle().Colors[ImGuiCol_ChildBg]);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyle().Colors[ImGuiCol_ChildBg]);
	if (m_wrap)ImGui::PushTextWrapPos(0.0f);
	ImGui::InputTextMultiline("##console", m_buf.data(), m_buf.size() + 1, ws, ImGuiInputTextFlags_ReadOnly);
	if (m_wrap)ImGui::PopTextWrapPos();
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(1);


#if 0
	ImGui::BeginChild("Log");
	ims_window_drag w_ch(true);
	if (m_wrap)ImGui::PushTextWrapPos(0.0f);
	ImGui::TextUnformatted(m_buf.data(), m_buf.data()+ m_buf.size());
	if (m_wrap)ImGui::PopTextWrapPos();
	ImGui::EndChild();
#endif
}
