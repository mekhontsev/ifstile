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
#include "ui_palette.h"
#include "gui.h"

#include "palette.h"
#include "ims_window_drag.h"


const char* ws_palette::get_title()
{
	return "Palette";
}

void ws_palette::show_in_rand_mode() 
{
	auto& p = get_cur_palette();

	if (ims_button("Randomize")) {
		stop_build_then([&p, this]() {
			p.randomize(
				m_H_range.data(),
				m_S_range.data(),
				m_V_range.data(),
				m_A_range.data());
			on_change_color();
			});
	};

	ImGui::SliderFloat2("H", m_H_range.data(), 0, 1);
	ImGui::SliderFloat2("S", m_S_range.data(), 0, 1);
	ImGui::SliderFloat2("V", m_V_range.data(), 0, 1);
	ImGui::SliderFloat2("A", m_A_range.data(), 0, 1);
};


void ws_palette::show()
{
	if (ims_button(m_rand_mode?" <= ": "Rand")) {
		m_rand_mode = !m_rand_mode;
	};
	
	if (m_rand_mode) {
		SAME_LINE();
		show_in_rand_mode();
		return;
	}

	

	auto& p = get_cur_palette();


	{
		SAME_LINE();
		if (ims_button("Load")) {
			do_load_palette(p);
		};
	}

	{
		SAME_LINE();
		if (ims_button("Save")) {
			do_save_palette(p);
		};
	}

	{
		SAME_LINE();
		if (ims_button("Sort", "checked->first")) {
			stop_build_then([&p]() {
				p.sort();
				on_change_color();
			});
		};
	}


	{
		SAME_LINE();
		if (ims_button("Reset")) {
			stop_build_then([&p]() {
				p.reset();
				on_change_color();
			});
		};
	}


	////////////////////////////////////////////////////////////////////////////
	{

		auto s = static_cast<int>(p.data.size());
		if (ImGui::InputInt("Size", &s)) {

			if (s < 1)s = 1;
			else if (s > 4096)s = 4096;

			stop_build_then([s, &p]() {
				p.resize((size_t)s);
				on_change_color();
			});
		}
	};


	auto& d = p.data;

	ImGui::BeginChild("ColorList");
	ims_window_drag w_ch(false);
	for (size_t i = 0; i < d.size(); ++i) {
		auto& e = d[i];

		ImGui::PushID((int)i);

		bool b = e.checked_p;
		if (ImGui::Checkbox("", &b) && w_ch.allow()) {
			e.checked_p = b;
			do_rebuild();
		}

		{
			//SAME_LINE();
			ImGui::SameLine();
			let lb = (e.name.empty()) ? std::to_string(i) : e.name;
			auto clr = e.c;
			if (ImGui::ColorEdit4(lb.c_str(), clr.data())) {
				stop_build_then([&e, clr]() {
					e.c = clr;
					on_change_color();
				});
			}
		}

		ImGui::PopID();
	}
	ImGui::EndChild();
}
