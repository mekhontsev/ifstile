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
#include "ims_val.h"
#include "eval_pool.h"
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
		ImGui::PushID(next_id++);
		double td = g_creator.m_search_hdim_min;
		if (input_double(td, "dim H min")) {
			g_creator.m_search_hdim_min = std::max(td, 0.0);
		}
		ImGui::PopID();
	}

	{
		ImGui::PushID(next_id++);
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

		ImGui::PushID(next_id++);
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


static size_t entry_size(const ims_val* d)
{
	if (!d || !d->is(ims_val_b::ETP::vector, ims_val_b::EST::other)) {
		return 0;
	}
	return d->get_size();
}

static bool get_i64(const ims_val* d, int64_t* dst)
{
	if (!d->is(ims_val_b::ETP::number)) {
		return false;
	}
	if (d->is(ims_val_b::EST::rational)) {
		let* v = d->p_i();
		if (v->denominator() != 1) {
			return false;
		};
		if(*dst)*dst = v->numerator();
		return true;
	}
	if (!d->is(ims_val_b::EST::real)) {
		return false;
	}
	let v = d->p_r()[0];
	let iv = static_cast<int64_t>(v);
	if (v != iv) {
		return false;
	}
	if (*dst)*dst = iv;
	return true;
}


static void show_invalid_field()
{
	ImGui::TextUnformatted("**Invalid field**");
}


void ws_creator::show_ui_for_val(const ims_val* d, pool_ptr& v)
{
	let sz = entry_size(d);

	bool err = true;

	IMS_SCOPE([&] {
		if (err)show_invalid_field();
	});

	//recognize field type

	if (sz == 0) {
		return;
	}
	
	let* d0 = d->p_v(0);

	if (sz >= 2 && d0->is(ims_val_b::ETP::number) &&
		d->p_v(1)->is(ims_val_b::ETP::string))
	{//drop down list

		int64_t def_val;
		if (!get_i64(d0, &def_val)) {
			return;
		}
		if (def_val < 0 || def_val >= (int64_t)sz - 1) {
			return;
		}
		for (size_t i = 2; i < sz; ++i) {
			if(!d->p_v(i)->is(ims_val_b::ETP::string)) {
				return;
			}
		}

		if (!v || !v->is(ims_val_b::ETP::number, ims_val_b::EST::rational)){
			v = eval_pool::ep.get_scalar_int(def_val);
		}
		int64_t cur_val = def_val;
		if (!v->to_int(cur_val)) {
			*v->p_i() = def_val;
		}

		std::vector<std::string> items;
		items.reserve(sz - 1);
		for (size_t i = 1; i < sz; ++i) {
			let sv = d->p_v(i)->get_string();
			items.emplace_back(sv.data(), sv.size());
		}

		std::string item;
		auto get_text = [&](size_t idx) {
			item = d->p_v(idx + 1)->get_string();
			return item.c_str();
		};

		ImGui::PushID(next_id++);
		if (ImGui::BeginCombo("", get_text(cur_val))) {
			for (size_t i = 0; i < sz - 1; ++i) {
				bool selected = (i == (size_t)cur_val);
				ImGui::PushID(next_id++);
				if (ImGui::Selectable(get_text(i), selected)) {
					*v->p_i() = i;
				}
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		ImGui::PopID();

		err = false;
		return;
	}
	if (sz >= 2 && d0->is(ims_val_b::ETP::number) &&
		entry_size(d->p_v(1)) > 0)
	{//array
		let old_length = m_cur_name.size();
		if (old_length > 0) {
			ImGui::NewLine();
		}
		int64_t nume_el;
		if (!get_i64(d0, &nume_el)) {
			return;
		}
		if (nume_el < 0 || nume_el > 1024) {
			return;
		}

		if (!v ||
			!v->is(ims_val_b::ETP::vector, ims_val_b::EST::other) ||
			v->get_size()!= (size_t)nume_el)
		{
			v = eval_pool::ep.get_vector(nume_el);
		}

		for (size_t i = 0; i < (size_t)nume_el; ++i) {
			fmt::format_to(std::back_inserter(m_cur_name),"[{}]", i);
			ImGui::TextUnformatted(m_cur_name.data(), m_cur_name.data()+m_cur_name.size());
			ImGui::SameLine();
			pool_ptr vi = v->p_v(i);
			if(vi)vi->add_ref();
			show_ui_for_val(d->p_v(1), vi);//recursive call
			if (v->p_v(i)) eval_pool::ep.release(v->p_v(i));
			v->p_v()[i] = vi.release();
			m_cur_name.resize(old_length);
		}
		err = false;
		return;
	}
	if (entry_size(d0) > 0)
	{//struct
		let old_length = m_cur_name.size();
		if (old_length > 0) {
			ImGui::NewLine();
		}

		if (!v ||
			!v->is(ims_val_b::ETP::vector, ims_val_b::EST::other) ||
			v->get_size() != (size_t)sz)
		{
			v = eval_pool::ep.get_vector(sz);
		}

		for (size_t i = 0; i < sz; ++i) {
			let* entry = d->p_v(i);
			let szi = entry_size(entry);
			if (szi < 2) {
				show_invalid_field();
				continue;
			}
			auto* id = entry->p_v(0);
			if (!id->is(ims_val_b::ETP::string)) {
				show_invalid_field();
				continue;
			};
			if (old_length > 0)m_cur_name += '.';
			m_cur_name += id->get_string();
			ImGui::TextUnformatted(m_cur_name.data(), m_cur_name.data() + m_cur_name.size());
			ImGui::SameLine();

			pool_ptr vi = v->p_v(i);
			if (vi)vi->add_ref();
			show_ui_for_val(entry->p_v(1), vi);//recursive call
			if (v->p_v(i)) eval_pool::ep.release(v->p_v(i));
			v->p_v()[i] = vi.release();

			m_cur_name.resize(old_length);
		}
		err = false;
		return;
	};
	if (d0->is(ims_val_b::ETP::string))
	{//string
		let def_val = d0->get_string();

		static std::string val;
		if (v && v->is(ims_val_b::ETP::string)) {
			val = v->get_string();
		} else {
			v.reset(eval_pool::ep.get_string(def_val));
			val = def_val;
		}

		ImGui::PushID(next_id++);
		if (ImGui::InputText("", &val)) {
			v.reset(eval_pool::ep.update_string(v.get_mut(), val));
		};
		ImGui::PopID();

		err = false;
		return;
	};

	if (!d0->is(ims_val_b::ETP::number)) {
		return;
	}

	
	if (d0->is(ims_val_b::EST::rational)) 
	{//integer
		int64_t def_val;
		if (sz < 3 || !d0->to_int(def_val)) {
			return;
		}

		int64_t vmin, vmax;
		if (!d->p_v(1)->to_int(vmin) || !d->p_v(2)->to_int(vmax)) {
			return;
		}

		if (def_val < vmin || def_val > vmax) {
			return;
		}

		if (!v || !v->is(ims_val_b::ETP::number, ims_val_b::EST::rational)) {
			v = eval_pool::ep.get_scalar_int(def_val);
		}
		int64_t cur_val = def_val;
		if (!v->to_int(cur_val) || cur_val<vmin || cur_val>vmax) {
			*v->p_i() = def_val;
		}

		ImGui::PushID(next_id++);

		if (vmin==0 && vmax==1){
			bool b = (cur_val != 0);
			if (ImGui::Checkbox("", &b)) {
				*v->p_i() = b ? 1 : 0;
			}
		} else {
			if (ImGui::DragScalar("", ImGuiDataType_S64, &cur_val,
				0.5f / float(vmax - vmin + 1), &vmin, &vmax))
			{
				*v->p_i() = cur_val;
			}
		}
		ImGui::PopID();

		err = false;
		return;
	}

	if (d0->is(ims_val_b::EST::real))
	{//real
		double def_val;

		if (sz < 3 || !d0->to_real(def_val)) {
			return;
		}

		double vmin, vmax;
		if (!d->p_v(1)->to_real(vmin) || !d->p_v(2)->to_real(vmax)) {
			return;
		}

		if (def_val<vmin || def_val>vmax) {
			return;
		}

		if (!v || !v->is(ims_val_b::ETP::number, ims_val_b::EST::real)) {
			v = eval_pool::ep.get_scalar_real(def_val);
		}
		double cur_val = *v->p_r();
		if (cur_val < vmin || cur_val > vmax) {
			*v->p_r() = def_val;
		}

		ImGui::PushID(next_id++);
		if (ImGui::DragScalar("", ImGuiDataType_Double, &cur_val,
			0.5f / float(vmax - vmin + 1), &vmin, &vmax))
		{
			*v->p_r() = cur_val;
		};
		ImGui::PopID();
		err = false;
		return;
	}

};

void ws_creator::show()
{
	next_id = 0;

	auto& nfo = ims_info_get();
	let* dialog = nfo.m_constructor_dialog.get();

	if (!dialog) {
		show_2d_creator();
		return;
	}

	if (!ImGui::BeginTabBar("Creators"))return;
	IMS_SCOPE([] { ImGui::EndTabBar(); });

	if (ImGui::BeginTabItem("This")){
		m_cur_name.clear();
		if (ims_button("Create")) {
			get_thread(e_ims_threads::aux).start([this]() {
				auto ret = ims_info_get().create_from_constructor(
					m_constructor_value.get());
				if (!ret.empty()) {
					auto lambda = [ret] {
						ims_show_message(ret);
					};
					main_thread::post(lambda);
				}
			});
		}
		ImGui::SameLine();
		if (ims_button("Reset")) {
			m_constructor_value.reset();
		}
		show_ui_for_val(dialog, m_constructor_value);

		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("2D Digraphs")){
		show_2d_creator();
		ImGui::EndTabItem();
	}
};
