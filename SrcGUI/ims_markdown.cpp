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
#include "ims_markdown.h"
#include "imgui_md.h"

#include "platform.h"
#include "gui.h"
#include "ims_settings.h"

extern ImFont* g_font_regular;
extern ImFont* g_font_bold;
extern ImFont* g_font_italic;

struct ims_markdown : public imgui_md 
{
	ImFont* get_font(float* fsz) const override
	{
		*fsz = 1;

		if (m_is_table_header) {
			return g_font_bold;
		}

		if (m_hlevel == 0) {
			if (m_is_em) {
				return g_font_italic;
			}
			if (m_is_strong) {
				return g_font_bold;
			}
			return g_font_regular;
		}

		*fsz = std::pow(1.2f, (7.0f - m_hlevel));
		return g_font_bold;

	};

	ImVec4 get_color() const override
	{
		if (!m_href.empty()) {
			if (get_settings().m_dark) {
				return ImVec4(.43f, .82f, 1.0f, 1.0f);
			} else {
				return ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered];
			}
		}

		return  ImGui::GetStyle().Colors[ImGuiCol_Text];

	}

	void open_url() const override
	{
		platform::open_url(m_href);
	}

	bool get_image(image_info& nfo) const override
	{
		nfo.texture_id = ImGui::GetIO().Fonts->TexRef;
		nfo.size = {40,20};
		nfo.uv0 = { 0,0 };
		nfo.uv1 = {1,1};
		return true;
	}

	void soft_break() override
	{
		ImGui::NewLine();
	}


	void html_div(const std::string& dclass, bool e) override
	{
		if (dclass == "matrix") {
			if (e) {
				m_table_border = false;
				m_table_header_highlight = false;
				//ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			} else {
				m_table_header_highlight = true;
				m_table_border = true;
			//	ImGui::PopStyleColor();
			}
		}
		if (dclass == "red") {
			if (e) {
				m_table_border = false;
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			} else {
				m_table_border = true;
				ImGui::PopStyleColor();
			}
		}
	}

};


void markdown(std::string_view str)
{
	static ims_markdown g_md_printer;
	g_md_printer.print(str.data(), str.data()+str.size());
}

