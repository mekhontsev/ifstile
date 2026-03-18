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
#include "ims_val_b.h"

struct pool_ptr;

struct param_walker
{
	enum class node_type
	{
		invalid,
		drop_down,
		array,
		struct_type,
		string,
		i64,
		f64,
	};

	node_type m_t;
	ims_val_b::EST m_s;//for array and struct
	//for numbers
	bool m_e1{}, m_e2{};//has min, max
	std::array<int64_t, 3> m_i;	//def, min, max
	std::array<double, 3> m_r;	//def, min, max


	//0 indicates not an object array, empty array or error
	static size_t checked_size(const ims_val* d);

	static node_type classify(const ims_val* d);

	bool check_type(const ims_val* d);

	//returns true if a value reset was requested
	using Func = bool(
		const param_walker& t,
		size_t rec_level,
		const ims_val* d,
		pool_ptr& v);


	const ims_val* create_def(const ims_val* d);

	//return true on error
	bool process(
		const ims_val* d,
		pool_ptr& v,
		size_t rec_level,
		const std::function<Func>& f);
};
