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
#include "ims_window_drag.h"
#include "ims_settings.h"
#include "gui.h"

bool ims_window_drag::s_drag_in_progress = false;

ims_static ImVec2 s_last_delta(0, 0);

ims_window_drag::ims_window_drag(bool use_drag)
{
	if (get_settings().m_window_mode == window_mode_type::floating) {
		use_drag = false;
	}

	m_use_drag = use_drag;

	let delta = ImGui::GetMouseDragDelta(0);

	if (ImGui::IsMouseClicked(0)) {
		s_last_delta = delta;
	}

	if (!m_use_drag || !ImGui::IsWindowFocused() || !ImGui::IsMouseDragging(0))return;

	let dy = delta.y - s_last_delta.y;
	s_last_delta = delta;
	//ImGui::ResetMouseDragDelta(0);//not allowed - interferes with other subsystems
	ImGui::SetScrollY(ImGui::GetScrollY() - dy);
	s_drag_in_progress = true;
}

ims_window_drag::~ims_window_drag()
{
	if (!m_use_drag)return;

	if (ImGui::IsMouseReleased(0)) {
		s_drag_in_progress = false;
	}
}
