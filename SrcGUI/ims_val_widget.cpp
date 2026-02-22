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
		if (*dst)*dst = v->numerator();
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


static size_t entry_size(const ims_val* d)
{
	if (!d || !d->is(ims_val_b::ETP::vector, ims_val_b::EST::other)) {
		return 0;
	}

	let sz = d->get_size();
	for (size_t i = 0; i < sz; ++i) {
		if (!d->p_v(i)) {
			return i;
		}
	};

	return sz;
}

void ims_val_widget::show_ui_for_val(const ims_val* d, pool_ptr& v, int& next_id)
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
			if (!d->p_v(i)->is(ims_val_b::ETP::string)) {
				return;
			}
		}

		if (!v || !v->is(ims_val_b::ETP::number, ims_val_b::EST::rational)) {
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
			v->get_size() != (size_t)nume_el)
		{
			v = eval_pool::ep.get_vector(nume_el);
		}

		for (size_t i = 0; i < (size_t)nume_el; ++i) {
			fmt::format_to(std::back_inserter(m_cur_name), "[{}]", i);
			ImGui::TextUnformatted(m_cur_name.data(), m_cur_name.data() + m_cur_name.size());
			ImGui::SameLine();
			pool_ptr vi = v->p_v(i);
			if (vi)vi->add_ref();
			show_ui_for_val(d->p_v(1), vi, next_id);//recursive call
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
			if (szi >= 3 && entry->p_v(2)->is(ims_val_b::ETP::string)) {
				let tt = entry->p_v(2)->get_string();
				if (!tt.empty()) {
					set_tooltip("%.*s", static_cast<int>(tt.size()), tt.data());
				}
			}
			ImGui::SameLine();


			pool_ptr vi = v->p_v(i);
			if (vi)vi->add_ref();
			show_ui_for_val(entry->p_v(1), vi, next_id);//recursive call
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

		if (vmin == 0 && vmax == 1) {
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

void ims_val_widget::show(const ims_val* d, int& next_id)
{
	m_cur_name.clear();
	show_ui_for_val(d, m_value, next_id);
}

void ims_val_widget::reset()
{
	m_value.reset();
}

