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
//no one else can be included - that's the point

void set_tooltip(const char* fmt, ...);
bool ims_button(const char* title, const char* tip = nullptr, int* id = nullptr);
void set_ui_scale(float f);
float& get_ui_scale();
bool drag_exp(const char* label, float& v, bool positive);
bool input_double(double& v, const char* label = nullptr, bool positive =false);
bool input_size_t(const char* label, size_t* v, int step = 1);

//ma<mi means there are no limits for the value
//ma==mi==0 means from 0 to infinity
bool input_double2(
	double& v, 
	double mi = 0,
	double ma =-1,
	const char* label=nullptr,
	double mul = 20);

void set_dark_theme(bool dark);


struct ims_width
{
	ims_width(float sz);
	~ims_width();
	void reset(float sz);
};


struct line_helper
{
	float p[2] = { 0,0 };
	void begin();
	void end();
	struct same_line
	{
		same_line(line_helper& lh) : m_lh(lh) { m_lh.begin(); };
		~same_line() { m_lh.end(); };
		line_helper& m_lh;
	};

	//alternative implementation in case of API changes
	static float limit(float elem_size = -1);
	static void same(float lim = 0);
};
//#define SAME_LINE() line_helper::same()
#define SAME_LINE() static line_helper lh;line_helper::same_line _sl_(lh)

