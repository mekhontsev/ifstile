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
#include "column_id.h"

struct  search_info;
struct  oper_block;

struct data_column
{
	using Integer = int64_t;
	using Real = double;

	//powers of two
	enum option :uint8_t
	{
		none = 0,
		need_search = 1,//requires the presence of a search structure
		early_calc = 2,//calculated early, before finding the bounding sphere
		contains_id = 4,//contains the ID
		def_vis = 8,//visible - default
	};


	using get_int_type = Integer(const oper_block&, const search_info&);
	using get_float_type = Real(const oper_block&, const search_info&);

	using back_iter = std::back_insert_iterator<std::string>;
	
	using to_string_type = 
		void (const oper_block&, const search_info&, back_iter&& str);

	////////////////////////////////////////////////////////////////////////////
	const char* title;
	const char* description;

	uint8_t m_calc_info = none;

	//default sort order
	//true - sort in ascending order, false - sort in descending order
	bool def_order;

	get_int_type* get_int;
	get_float_type* get_float;
	to_string_type* to_string;

	//limits at program start
	struct
	{
		std::array<Integer, 2> idef;//for integer
		std::array<Real, 2> rdef;//for real
	} limit;

	////////////////////////////////////////////////////////////////////////////
	
	bool is_def_vis() const 
	{
		return (m_calc_info & data_column::option::def_vis) != 0;
	}

	bool is_need_search() const 
	{
		return (m_calc_info & data_column::option::need_search) != 0;
	}
	

	
	bool has_value() const
	{
		return get_int || get_float || to_string;
	}

	bool has_number() const
	{
		return get_int || get_float;
	}

	bool is_same(const oper_block* b1, const oper_block* b2) const;

	void get_column_str(const oper_block& sr, std::string& s, bool raw) const;

	using arr = std::array<data_column, column_id::NUM_COLS>;

	static const arr g_cols;
};






