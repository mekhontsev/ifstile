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
#include "ui_utils.h"
//no one else can be included - that's the point

template<typename Real>
Real calc_max(Real x)
{
	if (x == 0)return 0;
	let ax = std::abs(x);
	let y = std::pow(Real(2), 1 + floor(log(ax) / log(Real(2))));
	return x > 0 ? y : -y;
};


template<typename Real>
void calc_min_max(const Real v, Real& vmin, Real& vmax, bool positive)
{
	if (positive) {
		if (v < 1) {
			vmin = 0;
			vmax = 1;
		} else {
			vmax = calc_max(v);
			vmin = vmax / 4;
		}
	} else {
		if (abs(v) < 1) {
			vmin = -1;
			vmax = 1;
		} else {
			if (v > 0) {
				vmax = calc_max(v);
				vmin = vmax / 4;
			} else {
				vmin = calc_max(v);
				vmax = vmin / 4;
			}
		}
	}
};

void set_tooltip(const char* fmt, ...)
{
	if (!fmt || !*fmt || !ImGui::IsItemHovered())return;
	let& s = ImGui::GetStyle();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, s.FramePadding);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, s.WindowRounding);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, s.WindowBorderSize);

	va_list args;
	va_start(args, fmt);
	ImGui::SetTooltipV(fmt, args);
	va_end(args);

	ImGui::PopStyleVar(3);
}

bool ims_button(const char* title, const char* tip, int* id)
{
	if (id) {
		ImGui::PushID(*id);
		(*id)++;
	}

	bool ret = ImGui::Button(title);

	if (id) {
		ImGui::PopID();
	}

	if (tip) {
		set_tooltip(tip);
	}

	return ret;
}



bool drag_exp(const char* label, float& v, bool positive)
{
	float vmax, vmin;
	calc_min_max(v, vmin, vmax, positive);
	return ImGui::DragFloat(label, &v, (vmax - vmin) / 200, vmin, vmax, "%.7f");
}

bool input_double(double& v, const char* label, bool positive)
{
	static std::array<char, 32> buf = { 0 };

	fmt::format_to_n(buf.data(), buf.size(), "{:.16}\0", v);

	auto& s = ImGui::GetStyle();

	let* ldrag = "*";

	let lsize = ImGui::CalcTextSize(ldrag, nullptr, true);
	let px = s.FramePadding.x;

	let sz = ImGui::CalcItemWidth() - lsize.x - px * 3.0f;
	ImGui::PushItemWidth(sz);
	let text_changed = ImGui::InputText("", buf.data(), buf.size());
	ImGui::PopItemWidth();

	let old = s.ItemSpacing.x;
	s.ItemSpacing.x = px;
	ImGui::SameLine();
	s.ItemSpacing.x = old;

	ImGui::Button(ldrag);
	set_tooltip("Drag to change");
	let drag_active = ImGui::IsItemActive();
	let drag_clicked = ImGui::IsItemClicked(0);

	if (label) {
		s.ItemSpacing.x = px;
		ImGui::SameLine();
		s.ItemSpacing.x = old;
		ImGui::TextUnformatted(label);
	}
	
	////////////////////////////////////////////////////////////////////////////
	double nv;

	if (drag_active) {
		static double start_val, mag;

		if (drag_clicked) {
			mag = std::abs(v);
			if (mag < 1 && (!positive || v == 0) )mag = 1;
			start_val = v;
		}

		if (!ImGui::IsMouseDragging(0)) {
			return false;
		}
		let mp = ImGui::GetMousePos();
		if (!ImGui::IsMousePosValid(&mp)) {
			return false;
		}

		nv = start_val +
			mag * ImGui::GetMouseDragDelta(0).x / ImGui::GetWindowWidth();

	} else if (text_changed) {
		if (!boost::conversion::try_lexical_convert(
			buf.data(), strlen(buf.data()), nv))
		{
			return false;
		}
	} else {
		return false;
	}

	if (v == nv)return false;

	v = nv;
	return true;
}



bool input_size_t(const char* label, size_t* v, int step)
{
	assert(*v < (size_t)std::numeric_limits<int>::max());
	auto s = (int)*v;
	if (!ImGui::InputInt(label, &s, step)) {
		return false;
	}
	*v = (size_t)std::max(s, 0);
	return true;
}

bool input_double2(double& v, double mi, double ma, const char* label, double mul)
{
	//TODO: a multiplier of 20 is equal in order of magnitude to fps...
	auto ws = mul / ImGui::GetWindowWidth();
	
	if (ma < mi) {//no restrictions
		ma = std::max(2 * fabs(v), 1.0);
		mi = -ma;
	} else if (ma == mi && ma == 0) {
		ma = 2 * fabs(v);
		if (ma == 0)ma = 1;
		mi = 0;
	} else {
	
	}
	let speed = float((ma - mi) * ws);

	return ImGui::DragScalar(label? label:"", ImGuiDataType_Double, &v,
		speed, &mi, &ma, "%.16f");
}

void set_dark_theme(bool dark)
{
	if (dark) {
		ImGui::StyleColorsDark();
	} else {
		ImGui::StyleColorsLight();
	}

#if defined(_MSC_VER)
	//takes effect only after redrawing
	struct SDL_Window* MainWindow_get();
	void set_dark_mode(SDL_Window*, bool); set_dark_mode(MainWindow_get(), dark);
	//SDL_ShowWindow(g_window);
#endif
}

////////////////////////////////////////////////////////////////////////////////

ims_width::ims_width(float sz)
{
	ImGui::PushItemWidth(sz);
}

ims_width::~ims_width()
{
	ImGui::PopItemWidth();
}

void ims_width::reset(float sz)
{
	ImGui::PopItemWidth();
	ImGui::PushItemWidth(sz);
}
////////////////////////////////////////////////////////////////////////////////

void line_helper::begin()
{
	let c = ImGui::GetItemRectMax().x;
	if (p[0] != c) {
		p[0] = c;
		p[1] = 0;
	}

	if (p[1] <= ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x) {
		ImGui::SameLine();
	}
}

void line_helper::end()
{
	if (p[1] == 0) {
		p[1] = ImGui::GetItemRectMax().x;
	}
}

float line_helper::limit(float elem_size)
{
	let& s = ImGui::GetStyle();

	let wsz = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;

	if (elem_size < 0) {
		//lower bound for the size of the next element
		elem_size = 20 * get_ui_scale() + s.FramePadding.x * 2;
	}

	return  wsz - s.ItemSpacing.x - elem_size;
};

void line_helper::same(float lim)
{
	if (lim <= 0)lim = limit();
	if (ImGui::GetItemRectMax().x < lim) {
		ImGui::SameLine();
	}
}

