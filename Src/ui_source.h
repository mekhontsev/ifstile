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
#include "ui_window.h"


struct oper_block;

struct edit_info
{
	//block source code/JS
	std::string src;			
	//the block we're editing, where we save the changes
	//if null, then we're currently creating a new one
	const struct oper_block* b = nullptr;

	bool js_mode = false;
};

struct ws_source
{
	const char* get_title();
	void show(int& id, bool js);

	void update_line(const char* buf, int cursor_pos);

	//reset when loading a new file
	void on_load();

	void apply(bool js);
	void append();

private:

	size_t m_line = 1;
	int m_cursor_pos = 0;

	bool m_reset = true;

	edit_info m_ed; //passed out via a pointer
	edit_info m_vis; //used directly in the UI
	
};
