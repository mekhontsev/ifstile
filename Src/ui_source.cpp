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
#include "ui_source.h"
#include "gui.h"
#include "oper_block.h"
#include "aifs_printer.h"
#include "utf8_decoder.h"
#include "def_number_types.h"
#include "ims_info.h"
#include "edit_helper.h"

const char* ws_source::get_title()
{
	return "Source";
}

//find how many lines a line takes, 0 if an error
static size_t utf8_num_rows(std::string_view str) 
{
	size_t row = 1;
		
	let* p = str.data();
	let* e = p + str.size();
	while (p < e) {
		uint32_t c;
		int err;
		p = utf8_decode(p, &c, &err);
		if (err) return 0;
		if (c == '\n')++row;
	}
	
	return row;
}


void ws_source::update_line(const char* buf, int cursor_pos)
{
	if (m_cursor_pos == cursor_pos)return;
	m_cursor_pos = cursor_pos;
	m_line = utf8_num_rows({ buf,(size_t)cursor_pos });
}


void ws_source::on_load()
{
	m_reset = true;
}



static void get_source(const oper_block& b, std::string& dst)
{
	dst.clear();
	std::ostringstream str;
	ims_write_block(str, &b);
	dst = str.str();
}

#if 0
static void send_ctrl_a()
{
	SDL_Event e = {};
	e.type = SDL_EVENT_KEY_DOWN;
	e.key.type = SDL_EVENT_KEY_DOWN;
	e.key.timestamp = 0;
	e.key.windowID = 0;
	e.key.which = 0;
	e.key.scancode = SDL_SCANCODE_A;
	e.key.key = SDLK_A;
	e.key.mod = SDL_KMOD_CTRL;
	e.key.down = true;
	e.key.repeat = false;
	SDL_PushEvent(&e);
};
#endif

void ws_source::apply(bool js)
{
	if (!js) {
		let* cb = get_cur_block();
		if (cb != m_vis.b || (cb && cb->m_flags.from_js)) {
			show_generic_error_msg();
			return;
		}
	}
	
	m_ed = m_vis;
	m_ed.js_mode = js;
	try_open_file([this]() {
		open_file("", "", true, false, &m_ed);
	}, false);
}


void ws_source::append()
{
	m_ed.b = nullptr;
	m_ed.src = m_vis.src;
	m_ed.js_mode = false;
	try_open_file([this]() {
		open_file("", "", true, false, &m_ed);
	}, false);
}


void ws_source::show(int& id, bool js)
{
	const oper_block* cb = nullptr;

	let& js_src = ims_info_get().m_js_src;

	m_reset = m_reset || m_vis.js_mode != js;

	if (!js) {
		cb = get_cur_block();
	
		if (m_reset  || cb != m_vis.b) {
			m_vis.b = cb;
			if (cb)get_source(*cb, m_vis.src);
			else m_vis.src.clear();
		}
	}
	else {
		if (m_reset) {
			m_vis.src = js_src;
			m_vis.b = nullptr;
		}
	}

	m_reset = false;
	m_vis.js_mode = js;

	/////////////////////////////////////////////////////////////////

	if (!js) {
		SAME_LINE();
		if (ims_button("+Block", "Save changes as a new block", &id)) {
			append();
		}
	}

	enum
	{
		EDITOR_Cut,
		EDITOR_Copy,
		EDITOR_Paste,
		EDITOR_Undo,
		EDITOR_Redo,
		EDITOR_All,
	};

	static int s_sel_item = EDITOR_Copy;
	{
		SAME_LINE();
		ImGui::PushID(id++);
		ImGui::PushItemWidth(70 * get_ui_scale());
		static constexpr auto cx = { "Cut","Copy", "Paste", "Undo", "Redo", "All" };
		ImGui::Combo("", &s_sel_item, cx.begin(), (int)cx.size(), (int)cx.size());
		ImGui::PopItemWidth();
		ImGui::PopID();

	}

	bool but_clicked = false;
	{
		SAME_LINE();
		but_clicked = ims_button("Ok", "", &id);
	}

	{
		SAME_LINE();
		ImGui::Text(":%zu", m_line);
		set_tooltip("Cursor line");
	}

	let id_edit = id++;
	int vedit = edit_helper::a_none;

	if (but_clicked) {
		switch (s_sel_item) {
		case EDITOR_Cut:
			vedit = edit_helper::a_cut;
			s_sel_item = EDITOR_Paste;
			break;
		case EDITOR_Copy:
			vedit = edit_helper::a_copy;
			s_sel_item = EDITOR_Paste;
			break;
		case EDITOR_Paste:
			vedit = edit_helper::a_paste;
			s_sel_item = EDITOR_Copy;
			break;
		case EDITOR_Undo:
			vedit = edit_helper::a_undo;
			break;
		case EDITOR_Redo:
			vedit = edit_helper::a_redo;
			break;
		case EDITOR_All:
			//send_ctrl_a();
			vedit = edit_helper::a_selall;
			s_sel_item = EDITOR_Copy;
		}
	}

	edit_helper::set_action(id_edit, vedit);

	//stretch InputTextMultiline to cover the entire window
	auto ws = ImGui::GetWindowSize();
	
	let cp = ImGui::GetCursorPos();

	ws.x -= cp.x + cp.x;
	ws.y -= cp.y + cp.x;

	ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;

 	flags |= ImGuiInputTextFlags_CallbackAlways;
 	auto text_cb = [](ImGuiInputTextCallbackData* data)
 	{
		((ws_source*)(data->UserData))->update_line(data->Buf, data->CursorPos);
		return 0;
 	};

	edit_helper::begin(id_edit);
	IMS_SCOPE([] {edit_helper::end(); });

	//fixes a bug where the cursor was incorrectly positioned when using a mouse
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetStyle().Colors[ImGuiCol_ChildBg]);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyle().Colors[ImGuiCol_ChildBg]);
	if (but_clicked) {
		ImGui::SetItemDefaultFocus();
		ImGui::SetKeyboardFocusHere();
	}

	ImGui::InputTextMultiline("", &m_vis.src, ws, flags, text_cb, this);

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(1);
}
