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
#include "ui_modal_msg.h"
#include "gui.h"

#include "ims_worker.h"
#include "call_thread.h"


bool ws_modal_msg::need_to_show() const
{
	return !m_message.empty();
}


bool ws_modal_msg::close()
{
	if (!need_to_show()) {
		return false;
	}
	m_confirm = nullptr;
	m_message.clear();
	redraw_gui(1);
	return true;
}

const char* ws_modal_msg::get_title()
{
	return "Message";
}

void ws_modal_msg::show_ext(
	const char* title,
	std::string_view text, 
	bool* p_ok, 
	bool* p_cancel)
{
	
	ImGui::OpenPopup(title);

	let& ds = ImGui::GetIO().DisplaySize;

	ImGui::SetNextWindowPos(ImVec2(ds.x * 0.5f, ds.y * 0.5f),
		ImGuiCond_Always, ImVec2(0.5f, 0.5f));

	ImGui::SetNextWindowSize(ImVec2(0, 0));


	if (!ImGui::BeginPopupModal(title, nullptr,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (*p_ok)*p_ok = false;
		if (*p_cancel)*p_cancel = false;
		return;
	}

	IMS_SCOPE([] {ImGui::EndPopup(); });

	let cp = ImGui::GetCursorPosX();

	let* tb = text.data();
	let* te = tb + text.size();

	let sz = ImGui::CalcTextSize(tb, te).x + cp * 2;

	bool need_wrap = ds.x < sz;

	if (need_wrap) {
		ImGui::PushTextWrapPos(ds.x - cp);
	}
	ImGui::TextUnformatted(tb, te);

	if (need_wrap) {
		ImGui::PopTextWrapPos();
	}

	ImGui::Separator();

	if (p_ok) {
		*p_ok = ImGui::Button("  Ok  ");
	}
	if (p_cancel) {
		if (p_ok)ImGui::SameLine();
		*p_cancel = ImGui::Button("Cancel");
	}
};

void ws_modal_msg::show()
{
	bool b_ok = false, b_cancel = false;

	show_ext(get_title(), m_message.c_str(), 
		&b_ok, 
		m_confirm ? &b_cancel : nullptr);


	auto close = [this]() {
		m_confirm = nullptr;
		m_message.clear();
		ImGui::CloseCurrentPopup();
		redraw_gui(1);
	};


	if (b_ok) {		
		if (m_confirm) {
			m_confirm();
		}
		close();
	}

	if (b_cancel) {
		close();
	}
}

void ws_modal_msg::confirm(std::string_view msg, std::function<void()>&& F)
{
	assert(ims_worker::is_main_thread());//only as a reaction to the user
	assert(!msg.empty());
	assert(F);

	m_confirm = std::move(F);
	m_message = msg;
	redraw_gui(1);
}

void ws_modal_msg::message(std::string_view msg)
{
	assert(!msg.empty());

	if (!ims_worker::is_main_thread()) {
		auto lambda = [msg = std::string(msg),this] {message(msg); };
		main_thread::post(lambda);
		return;
	}
	m_confirm = nullptr;
	m_message = msg;
	redraw_gui(1);
	//platform::message(msg.c_str());
}
