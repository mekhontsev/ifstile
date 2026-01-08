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
#include "ims_info_counted_map.h"

//all the necessary information to identify variables during inheritance
//each block always belongs to a certain category
struct block_category_data
{
	struct opinfo
	{
#ifndef NDEBUG
		std::string name;//only in the debug version
#endif
		size_t unk_id = ims_max;//which variable does it correspond to?
	};

	//information about named operators (start operator_arr)
	std::vector<opinfo> m_refs;

	//number of parent categories (sorted by index) in key,
	//all other void* variables (ims_identifiers::data*) (also sorted by index)
	size_t m_num_parents = 0;

	//by identifier unk_id gives the ordinal number of the variable in the graph
	ankerl::unordered_dense::map<size_t, size_t> m_unk2var;

	std::string_view get_var_name(size_t ref) const;

	//at the end there can be many nullptr which correspond to unnamed variables
	using key = std::vector<void*>;
};


using block_category_info = ims_info_counted_map<block_category_data>;

