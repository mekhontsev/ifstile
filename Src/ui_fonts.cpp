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
#include "ui_fonts.h"

using font_data = std::span<const uint8_t>;

font_data get_icon_font();
font_data get_bold_font();
font_data get_regular_font();
font_data get_italic_font();

ims_global ImFont* g_font_regular = nullptr;
ims_global ImFont* g_font_bold = nullptr;
ims_global ImFont* g_font_italic = nullptr;
ims_global ImFont* g_font_awesome = nullptr;


void init_fonts()
{
	//ImGui::GetIO().Fonts->AddFontDefault();

	auto* fonts = ImGui::GetIO().Fonts;

	ImFontConfig fontCfg;
	fontCfg.FontDataOwnedByAtlas = false;
	//fontCfg.OversampleH = 4;
	//fontCfg.OversampleV = 4;

	//selected so that the height of lowercase letters at scale=1
	//is equal to that of the Windows window title at 100% zoom (6 pixels)
	//and so that the uppercase T and F have a clear top
	constexpr float font_size = 15.0;//cannot be made fractional (14.6) - ImGuiListClipper has problems
	constexpr float toolbar_font_size = 32;

	let reg_font = get_regular_font();
	g_font_regular = fonts->AddFontFromMemoryTTF(
		(void*)reg_font.data(),
		(int)reg_font.size(),
		font_size,
		&fontCfg);

	let bold_font = get_bold_font();
	g_font_bold = fonts->AddFontFromMemoryTTF(
		(void*)bold_font.data(),
		(int)bold_font.size(),
		font_size,
		&fontCfg);

	let italic_font = get_italic_font();
	g_font_italic = fonts->AddFontFromMemoryTTF(
		(void*)italic_font.data(),
		(int)italic_font.size(),
		font_size,
		&fontCfg);

	let icon_font = get_icon_font();
	g_font_awesome = fonts->AddFontFromMemoryTTF(
		(void*)icon_font.data(),
		(int)icon_font.size(),
		toolbar_font_size,
		&fontCfg);
}
