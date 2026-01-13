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
#include "ui_help.h"
#include "gui.h"
#include "ims_markdown.h"
#include "ims_window_drag.h"

const char* ws_help::get_title()
{
	switch (m_mode2)
	{
	case ws_help::mode::file:
		return "Current file";
	case ws_help::mode::block:
		return "Current block";
	case ws_help::mode::other:
		return m_help_id.c_str();
	}
	return "Manual";
}

static void default_help()
{
	ImGui::TextUnformatted("Coming soon...");
};

void ws_help::show()
{
	if (m_mode2 == mode::block || m_mode2 == mode::file) {
		if (m_cur_text.empty()) {
			ImGui::TextUnformatted("The file/block contains no information.");
		}else {
			markdown(m_cur_text);
		}
		return;
	}

	if (m_help_id.empty()) {
		default_help();
		return;
	}

	//lazy loading
	if (m_help.m_map.empty()) {
		m_help.load();
	}

	//ImGui::TextUnformatted(m_help_id.c_str());
	//ImGui::Separator();

	let it = m_help.m_map.find(m_help_id);
	if (it == m_help.m_map.end()) {
		default_help();
		return;
	}
	
//	ImGui::BeginChild("Elements2");
	ims_window_drag w_ch(true);
	markdown(it->second);


}

void ws_help::set_mode(mode m)
{
	m_mode2 = m;
}
