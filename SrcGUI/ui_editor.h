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
#include "ui_source.h"

struct ws_editor : public window_state
{
	const char* get_title() override;
	void show() override;
	void on_load() override;

	struct mut_data 
	{
		size_t ref;
		size_t pos;
	};

	std::vector<mut_data> m_mut;

	enum
	{
		EDITOR_CONTROLS,
		EDITOR_BUILTIN,
		EDITOR_SOURCE,
		EDITOR_JS,
		EDITOR_IFSTILE,
	};

	int m_eidt_type = EDITOR_CONTROLS;

	ws_source m_source_editor;

};


