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
#include "ui_creator.h"
#include "gui.h"
#include "ims_window_drag.h"

#include "creator_state.h"
#include "ims_worker.h"
#include "oper_block.h"
#include "columns.h"
#include "finder.h"
#include "ims_info.h"
#include "call_thread.h"


ims_static creator_state g_creator;


oper_block* create_ifs5(
	ims_info* nfo, 
	size_t idx, 
	const creator_state& cs, 
	const std::string& name);



const char* ws_creator::get_title()
{
	return "Creator";
}

static void create_ifs6(const std::string name) 
{
	clear_before_load();

	auto& nfo = ims_info_get();

	if (!create_ifs5(&nfo, g_creator.m_cur_poly, g_creator, name)) {
		return;//it's not easy to get here
	}

	init6(true);

	set_block_ex(0);

	////////////////////////////////////////////////////////////////////////////
	auto& cols = columns::get();
	cols.init_columns();
	{
		auto& r = cols.m_rule[ERULE::search_first][column_id::DIM2];
		r.filter = false;
		r.lim[0] = 0;
		r.lim[1] = 2;
	}
	{
		auto& r = cols.m_rule[ERULE::search_first][column_id::CT];
		r.filter = true;
		r.ilim[0] = (int64_t)connectedness::disconnected;
		r.ilim[1] = (int64_t)connectedness::strong;
	}
	////////////////////////////////////////////////////////////////////////////

	finder::get().on_create_ifs(g_creator.m_cg.m_ig2.m_edges.size());

	//unconditionally save only the search parameters
	save_env_block(false, true, false);

	set_last_file(name + ".aifs", true, false);

	StartSearch();
}

void ws_creator::create_ifs3() 
{
	let* fp = g_creator.get_found(g_creator.m_cur_poly);
	if (!fp) {
		ims_show_message("Select element!\n");
		return;
	} 

	auto& thr = get_thread(e_ims_threads::creator);

	thr.stop();//stop the thread

	std::string code;
	fp->second->get_group_code(code);

	using namespace std::string_literals;

	let name =
		"["s + g_creator.m_graph_str.data() + "]"s +
		code +
		"["s + fp->second->title + "]"s;

	stop_build_then([name]() {
		create_ifs6(name);
		ui_onload();
	});
	
}

void ws_creator::show_2d_creator()
{

	ImGui::InputText("Graph", &g_creator.m_graph_str);

	if (ims_button("Check")) {
		if (!g_creator.m_cg.parse(g_creator.m_graph_str)) {
			ims_show_message("Invalid graph !\n");
		}
	}

	if (g_creator.m_cg.m_graph_sp > 0) {
		SAME_LINE();
		ImGui::Text("%.16f : %s", (double)g_creator.m_cg.m_graph_sp, g_creator.m_cg.m_str_poly.c_str());
	}

	ImGui::Separator();


	auto& thr = get_thread(e_ims_threads::creator);

	if (thr.is_running()) {
		if (ims_button("Stop  ")) {
			thr.stop();
		}
	} else {
		if (ims_button("Search")) {
			if (!g_creator.m_cg.parse(g_creator.m_graph_str.data())) {
				ims_show_message("Invalid graph !\n");
			} else if (g_creator.m_search_hdim_min > g_creator.m_search_hdim_max) {
				ims_show_message("dim H min > dim H max !\n");
			} else {
				g_creator.m_cur_poly = 0;
				thr.start([]() {
					g_creator.find_poly();
					});
			}
		}
	}

	bool b_create;
	{
		SAME_LINE();
		b_create = ims_button("Create");
	}

	if (b_create) {
		try_open_file([this]() {
			create_ifs3();
			});
	}

	{
		SAME_LINE();
		ImGui::Checkbox("Integer", &g_creator.m_search_integer);
	}

	if (thr.is_running()) {
		SAME_LINE();
		let nc = log(1.0 + g_creator.m_num_checked) / log(2);
		ImGui::Text(":%.3f", nc);
	}
	//////////////////////////////////////////////////////////////////////////////////////

	{
		int deg = g_creator.m_search_poly_degree;
		if (ImGui::InputInt("dim Algebraic", &deg)) {
			deg = std::max(deg, 1);
			deg = std::min(deg, 30);
			g_creator.m_search_poly_degree = static_cast<uint8_t>(deg);
		}
	}


	{
		ImGui::PushID(m_next_id++);
		double td = g_creator.m_search_hdim_min;
		if (input_double(td, "dim H min")) {
			g_creator.m_search_hdim_min = std::max(td, 0.0);
		}
		ImGui::PopID();
	}

	{
		ImGui::PushID(m_next_id++);
		double td = g_creator.m_search_hdim_max;
		if (input_double(td, "dim H max")) {
			g_creator.m_search_hdim_max = std::max(td, 0.0);
		}
		ImGui::PopID();
	}

	{
		auto& v = g_creator.m_variance;
		auto s = (float)v;
		if (ImGui::InputFloat("variance", &s, 0.1f, 1, "%.1f")) {
			v = std::max(s, .0f);
		};
	}

	constexpr std::array sl{ "Group Poly","Poly","DIM" };
	bool bv = ImGui::BeginTable("Families", (int)std::size(sl),
		ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable);
	if (!bv)return;

	ims_window_drag w_ch(true);

	ImGui::TableSetupScrollFreeze(0, 1);
	for (let* c : sl)ImGui::TableSetupColumn(c);
	ImGui::TableHeadersRow();

	for (size_t i = 0; ; ++i) {
		auto* sr = g_creator.get_found(i);
		if (!sr)break;
		let& fp = *sr->second;

		ImGui::TableNextRow();

		////////////////////////////////////////////////////////////////////////
		static std::string code;
		fp.get_group_code(code);

		ImGui::TableNextColumn();

		ImGui::PushID(m_next_id++);
		bool b = ImGui::Selectable(code.c_str(), i == g_creator.m_cur_poly,
			ImGuiSelectableFlags_SpanAllColumns) && w_ch.allow();

		ImGui::PopID();
		if (b) {
			g_creator.m_cur_poly = i;
		}

		////////////////////////////////////////////////////////////////////////
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(fp.title.c_str());


		////////////////////////////////////////////////////////////////////////
		ImGui::TableNextColumn();
		ImGui::Text("%f", fp.si.back().ifs_dim);//TODO
	}
	ImGui::EndTable();
}



void ws_creator::show()
{
	m_next_id = 0;

	auto& nfo = ims_info_get();
	let* dialog = nfo.m_constructor_dialog.get();

	if (!dialog) {
		show_2d_creator();
		return;
	}

	if (!ImGui::BeginTabBar("Creators"))return;
	IMS_SCOPE([] { ImGui::EndTabBar(); });

	if (ImGui::BeginTabItem("This")){
		
		if (ims_button("Create")) {
			get_thread(e_ims_threads::aux).start([this]() {
				auto& nfo = ims_info_get();
				let blocks_start_from = nfo.m_list.m_blocks.size();

				auto ret = nfo.create_from_constructor(
					m_val_widget.m_value.get());

				auto lambda = [ret, blocks_start_from]
				{
					if (!ret.empty()) {
						ims_show_message(ret);
						return;
					}
					on_constructor_success(blocks_start_from);
				};
				main_thread::post(lambda);
			});
		}
		ImGui::SameLine();
		if (ims_button("Reset")) {
			m_val_widget.reset();
		}
		m_val_widget.show(dialog, m_next_id);

		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("2D Digraphs")){
		show_2d_creator();
		ImGui::EndTabItem();
	}
};
