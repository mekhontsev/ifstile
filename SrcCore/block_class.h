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

struct ims_info;


struct block_class
{	
	block_class(const ims_info* nfo) : m_nfo(nfo) {};

	block_class& operator=(const block_class&) = delete;

	struct opinfo 
	{	
#ifndef NDEBUG
		std::string name2;//only in the debug version
#endif
		size_t unk_id = ims_max;//which variable does it correspond to

		//the variable may become empty during random changes
		bool can_be_empty = true;
		//the variable is locked from changes
		bool var_is_locked = false;
	};

	
	//information about named operators
	std::vector<opinfo> m_refs;

	//gives the variable's ordinal number in the column using the identifier unk_id
	//inverse of m_refs
	ankerl::unordered_dense::map<size_t, size_t> m_unk2var;

	//which file does it belong to?
	const ims_info* m_nfo = nullptr;

	//default operator (changeable by user)
	size_t m_active_ref = ims_max;

	////////////////////////////////////////////////////////////////////////////

	std::string_view get_var_name(size_t ref) const;
	size_t find_var_by_name(std::string_view name) const;

	size_t add_var(size_t unk_id);
	size_t find_var(size_t unk_id) const;
};
