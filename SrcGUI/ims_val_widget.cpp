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
#include "ims_val_widget.h"
#include "ims_val.h"
#include "eval_pool.h"
#include "ui_utils.h"
#include "edit_helper.h"
#include "param_walker.h"


void ims_val_widget::show(const ims_val* d, int& next_id)
{
	auto ui_handler =[this](
		const param_walker& t,
		const ims_val* d, 
		ims_val* v,
		param_action& res)->bool
	{
		return handler(t, d, v, res);
	};

	m_next_id = next_id;

	param_walker t;
	if (t.init(d)) {
		t.process(d, m_value, ui_handler);
	}
}

void ims_val_widget::reset()
{
	m_value.reset();
}

bool ims_val_widget::handler(
	const param_walker& t,
	const ims_val* d,
	ims_val* v,
	param_action& res)
{
	assert(res.a == param_action::action::idle);

	bool ret = false;

	if (!v && t.m_t != param_walker::node_type::struct_type) {
		ImGui::TextUnformatted("Invalid value");
		ImGui::SameLine();
		if (ims_button("Reset", nullptr, &m_next_id)) {
			res.a = param_action::action::reset;
			ret = true;
		}
		return false;
	}

	switch (t.m_t) {
	case param_walker::node_type::drop_down:
	{
		assert(d->is(ims_val_b::ETP::vector, ims_val_b::EST::other));
		let sz = (size_t)t.m_i[2] + 1;

		std::string item;
		auto get_text = [&](size_t i) {
			item = d->p_v(i + 1)->get_string();
			return item.c_str();
		};

		int64_t cv = 0;
		if (!v->get_i64(cv)) {
			return false;
		}
		ImGui::PushID(m_next_id++);
		if (ImGui::BeginCombo("", get_text(cv))) {
			for (size_t i = 0; i < sz; ++i) {
				bool selected = (i == (size_t)cv);
				ImGui::PushID(m_next_id++);
				if (ImGui::Selectable(get_text(i), selected)) {
					v->set_i64((int64_t)i);
					ret = true;
				}
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		ImGui::PopID();
		return ret;
	}
	case param_walker::node_type::array:
	{
		int64_t cv;
		if (!v->get_i64(cv)) {
			return false;
		}
		int arr_sz = static_cast<int>(cv);
		ImGui::PushID(m_next_id++);
		if (ImGui::InputInt("", &arr_sz)) {
			if (arr_sz < 0)arr_sz = 0;
			v->set_i64(arr_sz);
			ret = true;
		}
		ImGui::PopID();
		return ret;
	}
	case param_walker::node_type::struct_type:
	{
		let str = d->p_v(0)->get_string();
		if (str.length() > 0) {
			ImGui::PushTextWrapPos(0.0f);
			ImGui::TextUnformatted(str.data(), str.data() + str.size());
			ImGui::PopTextWrapPos();
		}
		return false;
	}
	case param_walker::node_type::string:
	{
		static std::string val;
		val = v->get_string();

		edit_helper::begin(m_next_id++);
		if (ImGui::InputText("", &val)) {
			res.v = eval_pool::ep.update_string(v, val);
			ret = true;
		};
		edit_helper::end();
		return ret;
	}
	case param_walker::node_type::i64:
	{
		int64_t cv = 0;
		v->get_i64(cv);

		let both_lim = !t.m_e1 && !t.m_e2;
		ImGui::PushID(m_next_id++);
		if (both_lim && t.m_i[1] == 0 && t.m_i[2] == 1) {
			bool b = (cv != 0);
			ImGui::SameLine();
			if (ImGui::Checkbox("", &b)) {
				if (v->set_i64(b ? 1 : 0)) {
					ret = true;
				}
			}
		} else {
			let speed = both_lim ? 0.5f / float(t.m_i[2] - t.m_i[1] + 1) : 1;
			if (ImGui::DragScalar("", ImGuiDataType_S64, &cv, speed,
				t.m_e1 ? nullptr : &t.m_i[1],
				t.m_e2 ? nullptr : &t.m_i[2]))
			{
				if (v->set_i64(cv)) {
					ret = true;
				}
			}
		}
		ImGui::PopID();
		return ret;
	}
	case param_walker::node_type::f64:
	{
		double cv = 0;
		if (!v->get_f64(cv)) {
			return false;
		}

		let both_lim = !t.m_e1 && !t.m_e2;

		ImGui::PushID(m_next_id++);
		let speed = both_lim ? 0.5f / float(t.m_r[2] - t.m_r[1] + 1) : 1;
		if (ImGui::DragScalar("", ImGuiDataType_Double, &cv, speed,
			t.m_e1 ? nullptr : &t.m_r[1],
			t.m_e2 ? nullptr : &t.m_r[2]))
		{
			if (v->set_f64(cv)) {
				ret = true;
			}
		};
		ImGui::PopID();
		return ret;
	}
	default:
	{
		break;
	}
	}
	return ret;
}

