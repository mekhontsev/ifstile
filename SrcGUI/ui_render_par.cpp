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
#include "ui_render_par.h"
#include "render_params.h"
#include "gui.h"
#include "draw_task.h"

const char* ws_render_par::get_title()
{
	return "Rendering";
}

void ws_render_par::ShowRenderGeneral()
{
	int next_id = 0;
	bool ch;

	auto& rend = get_rpars();

	////////////////////////////////////////////////////////////////////////////
	// 
	////////////////////////////////////////////////////////////////////////////
	{
		//SAME_LINE();
		if (ims_button("Default", "Reset rendering parameters.", &next_id)) {
			get_rpars().reset_render_params();
			do_rebuild();
		};
	}
	////////////////////////////////////////////////////////////////////////////
	{
		SAME_LINE();
		if (ims_button("Rebuild", nullptr, &next_id)) {
			do_rebuild();
		};
	}
	////////////////////////////////////////////////////////////////////////////
	{
		SAME_LINE();
		if (ims_button("Stop", nullptr, &next_id)) {
			stop_build();
		};
	}

	////////////////////////////////////////////////////////////////////////////
	if (rend.m_use_window_res) {
		ImGui::PushStyleColor(ImGuiCol_Text,
			ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
	}


	let iw = 45 * get_ui_scale();

	ImGui::PushItemWidth(iw);

	ImGui::PushID(next_id++);
	{
		ImGui::InputInt("", &rend.m_resolution[0], 0);
	}
	ImGui::PopID();

	ImGui::PushID(next_id++);
	{
		SAME_LINE();
		ImGui::InputInt("", &rend.m_resolution[1], 0);
	}
	ImGui::PopID();

	ImGui::PopItemWidth();
	if (rend.m_use_window_res) {
		ImGui::PopStyleColor();
	}


	ImGui::PushID(next_id++);
	{
		SAME_LINE();
		if (ImGui::Checkbox("Auto", &rend.m_use_window_res)) {
			do_rebuild();
		};
		if (ImGui::IsItemHovered()) {
			size_t w, h;
			get_working_res(w, h);
			ImGui::SetTooltip("%zux%zu", w, h);
		}
	}
	ImGui::PopID();

	////////////////////////////////////////////////////////////////////////////
	int ovs = get_rpars().m_oversamp;
	ImGui::PushID(next_id++);
	if (ImGui::InputInt("Oversampling", &ovs)) {
		get_rpars().m_oversamp = ims_clamp(ovs, -4, 16);
	};
	ImGui::PopID();
	////////////////////////////////////////////////////////////////////////////
	ImGui::PushID(next_id++);
	if (ImGui::InputFloat("Quality", &get_rpars().m_quality, 1)) {
		get_rpars().adjust_quality();
	};
	ImGui::PopID();
	////////////////////////////////////////////////////////////////////////////

	ImGui::PushID(next_id++);
	ImGui::ColorEdit3("Background", get_background().data.data());
	ImGui::PopID();

	////////////////////////////////////////////////////////////////////////////

	auto& crz = get_colorize_params();

	{
		auto v = crz.shift;
		ImGui::PushID(next_id++);
		ch = input_double(v, "Color shift");
		ImGui::PopID();

		if (ch) {
			stop_build_then([v, &crz]() {
				crz.shift = v;
				on_change_color();
				});
		};
	}



	ImGui::Separator();

	auto crztype = (int)crz.type;
	ImGui::PushItemWidth(120.0f * get_ui_scale());
	ImGui::PushID(next_id++);


	static constexpr auto cb =
	{
		"Edges","Vertices","Field lines","Equipotential"
	};

	ch = ImGui::Combo("", &crztype, cb.begin(), (int)cb.size());
	ImGui::PopID();
	ImGui::PopItemWidth();

	if (ch)
	{
		let old_type = crz.type;
		let new_type = (colorize_params::EPAR)(crztype);

		draw_task dt;

		if (colorize_params::is_field(old_type) != colorize_params::is_field(new_type) ||
			(colorize_params::is_tiling(old_type) && old_type != new_type))
		{
			dt.rebuild();
		} else {
			dt.paint_2d_ext = true;
			dt.paint_2d_std = true;
			dt.paint_3d = true;
		}

		stop_build_then([new_type, old_type, &crz, dt]() {
			//we'll keep the old one
			get_rpars().m_colorize_var[old_type] = crz.params;
			//restore the new one
			crz.params = get_rpars().m_colorize_var[new_type];
			crz.type = new_type;

			bool ff = colorize_params::is_field(old_type) &&
				colorize_params::is_field(new_type);

			bool ft = colorize_params::is_tiling(old_type) &&
				colorize_params::is_tiling(new_type);

			if (ff || ft) {//reuse the first parameter (depth and index)
				crz.params[0] = get_rpars().m_colorize_var[old_type][0];
			}
			do_build(dt);
			});
	}


	if (crz.is_field()) {
		ImGui::SameLine();

		ImGui::PushID(next_id++);
		if (ims_button("Save EXR")) {
			do_save_exr();
		};
		ImGui::PopID();

		draw_task dt;


		ImGui::PushID(next_id++);
		if (input_double(crz.params[0], "Power")) {
			dt.build_2d_ext = true;
		};
		ImGui::PopID();


		if (crz.type == colorize_params::e_equipotential) {
			ImGui::PushID(next_id++);
			if (input_double(crz.params[1], "Magnitude")) {
				dt.paint_2d_ext = true;
			};
			ImGui::PopID();
		}

		if (dt.changed()) {
			stop_build_then([dt]() {
				do_build(dt);
			});
		}

	}

	if (crz.is_tiling()) {
		{
			auto v = crz.get_depth();
			ImGui::PushID(next_id++);
			ch = input_double(v,"Depth");
			ImGui::PopID();

			//-1 means that they tried to reduce from 0 to -1, no need to build
			if (ch) {
				v = std::clamp(v, 0.0, 1000.0);
				draw_task dt;
				dt.build_2d_std = true;
				dt.build_3d = true;
				stop_build_then([v, dt, &crz]() {
					crz.params[0] = v;
					do_build(dt);
				});
			}
		};

		if (ImGui::InputFloat("Thickness", &get_rpars().m_thickness, 1)) {
			get_rpars().m_thickness = std::max(get_rpars().m_thickness, 1.0f);
		};

		{

			ImGui::PushID(next_id++);
			ch = drag_exp("Borders", get_rpars().m_border_pow, false);
			ImGui::PopID();
			if (ch) {
				draw_task dt;
				dt.paint_2d_std = true;
				dt.paint_3d = true;

				stop_build_then([dt]() {
					do_build(dt);
				});
			};
		}

		////////////////////////////////////////////////////////////////////////
		//for edges 2d mode
		{
			ch = false;
			ImGui::PushID(next_id++);
			ch = drag_exp("Brightness", get_rpars().m_brightness, true) || ch;
			ImGui::PopID();
			ImGui::PushID(next_id++);
			ch = ImGui::SliderFloat("Contrast", &get_rpars().m_contrast, 0, 1) || ch;
			ImGui::PopID();
			if (ch) {
				draw_task dt;
				dt.paint_2d_std = true;
				dt.paint_3d = true;
				stop_build_then([dt]() {
					do_build(dt);
				});
			};
		}

		{
			bool v = get_rpars().m_inv_mode;
			ImGui::PushID(next_id++);
			ch = ImGui::Checkbox("Transparent", &v);
			ImGui::PopID();

			if (ch) {
				stop_build_then([v]() {
					get_rpars().m_inv_mode = v;
					draw_task dt;
					dt.paint_2d_std = true;
					do_build(dt);
					});
			}
		}
	}
}

void ws_render_par::ShowRender3DParams()
{
	auto& rend = get_rpars();

	draw_task dt;
	
	//////////////////////////////////////////
	ImGui::TextUnformatted("Ambient occlusion");
	if (ImGui::SliderFloat("Amount", &rend.m_ssao_density, 0.0f, 10.0f)){
		dt.paint_3d = true;
	}
	if (ImGui::SliderFloat("Radius %", &rend.m_ssao_rad_perc, 0.0f, 100.0f)) {
		dt.ssao_3d = true;
		dt.paint_3d = true;
	};
	if (ImGui::SliderInt("Samples", &rend.m_ssao_samples, 100, 10000)) {
		dt.ssao_3d = true;
		dt.paint_3d = true;
	};

	//////////////////////////////////////////
	//fog density relative to loc-ref!
	ImGui::TextUnformatted("Fog");
	if (ImGui::InputFloat("Density", &get_background().get_fog(), 0.1f)) {
		dt.paint_3d = true;
	};
	if (dt.changed()) {
		stop_build_then([dt]() {
			do_build(dt);
		});
	}
}

void ws_render_par::ShowRenderMesh()
{
	auto& rend = get_rpars();

	int next_id = 0;

	////////////////////////////////////////////////////////////////////////////
	ImGui::PushID(next_id++);
	ImGui::Checkbox("Colors", &rend.m_mesh_colors);
	ImGui::PopID();
	////////////////////////////////////////////////////////////////////////////
	ImGui::PushID(next_id++);
	if (ImGui::InputInt("Resolution", &rend.m_mesh_resolution, 100)) {
		rend.adjust_mesh_resolution();
	};
	ImGui::PopID();

	if (ims_button("Build", "Build mesh", &next_id)) {
		stop_build_then([]() {
			do_build_mesh();
		});
	};
}

void ws_render_par::show()
{
	if (!ImGui::BeginTabBar("RenderTabs"))return;
	IMS_SCOPE([] { ImGui::EndTabBar(); });

	if (ImGui::BeginTabItem("General"))
	{
		ShowRenderGeneral();
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Mesh"))
	{
		ShowRenderMesh();
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("3D"))
	{
		ShowRender3DParams();
		ImGui::EndTabItem();
	}
}
