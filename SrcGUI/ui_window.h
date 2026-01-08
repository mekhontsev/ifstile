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

#pragma once

struct window_state
{
	//called when the program starts
	virtual void on_create() {};
	//called immediately before displaying on the screen
	virtual void on_show() {};
	//called immediately after closing the window
	virtual void on_hide() {};

	//reset when loading a new file
	virtual void on_load() {};

	//show contents,
	//not called if a new file is currently being downloaded
	virtual void show() {};

	virtual const char* get_title() { return ""; };

	virtual const char* get_help_id() { return get_title(); };

	std::array<float, 2> m_scroll_pos = { 0,0 };
};
