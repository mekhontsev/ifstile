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
#include "string_view_hash.h"

//identifiers that were ever encountered in the file,
//require further classification
//can only be expanded
struct ims_identifiers
{	
	struct data 
	{
		size_t unk_id;//index in m_idx2unknown
		block_id_t block_id;//if the file contains a block with such a string id

		bool has_block() const 
		{
			return block_id != block_id_max;
		}
	};

	//references must be stable
	using map = UNAMESPACE::unordered_map<std::string, data, 
		string_view_hash, std::equal_to<>>;

	map m_unknown2idx;
	using val = map::value_type;
	
	//gives a string representation by index
	std::vector<val*> m_idx2unknown;

	////////////////////////////////////////////////////////////////////////////

	//inserts the element if it doesn't exist
	data& get_data(std::string_view v);
	
	const data* find_data(std::string_view str_id) const;

	//inserts the element if it doesn't exist
	size_t get_unk_id(std::string_view v)
	{
		return get_data(v).unk_id;
	};

	std::string_view get_str_from_unk(size_t unk_id) const 
	{
		return m_idx2unknown[unk_id]->first;
	}

	block_id_t find_block_id(std::string_view str_id) const;

	//create a unique block ID by adding a number to the prefix
	std::string gen_unique_block_id(
		std::string_view prefix, size_t* suffix = nullptr) const;

	size_t create_unique_identifier(std::string_view prefix);

	static bool is_identifier(std::string_view s);

};
