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

enum class BUILTIN_FUNC: uint8_t
{
	sin,
	cos,
	tan,
	asin,
	acos,
	atan,
	exp,
	log,
	floor,
	ceil,
	arg,	//atan2(y, x)

	invalid, //should be the last one
};

namespace built_in_func 
{

struct func_info
{
	std::string_view name;
	size_t sz;			//number of arguments
};

ims_static constexpr size_t s_num = (size_t)BUILTIN_FUNC::invalid;

ims_static constexpr std::array<func_info, s_num> info =
{ {
	{"sin",	1},
	{"cos",	1},
	{"tan",	1},
	{"asin",1},
	{"acos",1},
	{"atan",1},
	{"exp",	1},
	{"log",	1},
	{"floor",1},
	{"ceil",1},
	{"arg",	2},
} };


size_t num_args(BUILTIN_FUNC t);
size_t num_args(size_t);

double eval(const double* pargs, BUILTIN_FUNC t);

//gives the type by string 
BUILTIN_FUNC from_string(std::string_view str);
}

