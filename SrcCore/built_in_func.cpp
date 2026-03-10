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
#include "built_in_func.h"

namespace built_in_func 
{



size_t num_args(BUILTIN_FUNC t)
{
	return num_args(static_cast<size_t>(t));
}

size_t num_args(size_t t)
{
	return info[t].sz;
}

double eval(const double* pargs, BUILTIN_FUNC t)
{
	let& a = pargs[0];
	switch (t) {
	case BUILTIN_FUNC::sin:
		return sin(a);
	case BUILTIN_FUNC::cos:
		return cos(a);
	case BUILTIN_FUNC::tan:
		return tan(a);
	case BUILTIN_FUNC::asin:
		return asin(a);
	case BUILTIN_FUNC::acos:
		return acos(a);
	case BUILTIN_FUNC::atan:
		return atan(a);
	case BUILTIN_FUNC::exp:
		return exp(a);
	case BUILTIN_FUNC::log:
		return log(a);
	case BUILTIN_FUNC::floor:
		return floor(a);
	case BUILTIN_FUNC::ceil:
		return ceil(a);
	case BUILTIN_FUNC::abs:
		return abs(a);
	case BUILTIN_FUNC::arg:
		return atan2(pargs[1], a);
	default:
		assert(false);
		return 0;
	};
}




BUILTIN_FUNC from_string(std::string_view str)
{
	ims_func_static UNAMESPACE::unordered_map <std::string_view, BUILTIN_FUNC> s_map;

	if (s_map.empty()) {
		for (size_t i = 0; i < info.size(); ++i) {
			s_map[info[i].name] = static_cast<BUILTIN_FUNC>(i);
		}
	}
	let it = s_map.find(str);
	if (it == s_map.end()) {
		return BUILTIN_FUNC::invalid;
	}
	
	return it->second;
};


}

