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
#include "save_type.h"

struct flame_edge
{
	size_t vs;
	size_t vt;
	size_t m;//map
	double w;//weigth
	size_t clr_idx;
};

struct variator_params;
struct palette;
struct background;
struct oper_block;
struct eval_data;
template<typename Real> struct screen_disk;

size_t calc_flame(
	std::ostream& of,
	eval_data& ed,
	const save_type st,
	const variator_params& vp,
	const palette& pal,
	const background& backg,
	const screen_disk<double>& sd,
	std::span<const oper_block*> vis); //multiple
	