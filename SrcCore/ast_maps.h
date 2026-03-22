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
#include "indexed_maps.h"
#include "operator_ptr.h"
#include "pool_ptr.h"

struct variable;
struct ims_val;

struct ast_maps
{
	std::vector<pool_ptr> m_atoms;
	indexed_maps m_ixm;
	size_t m_num_refs = 0;

	////////////////////////////////////////////////////////////////////////////

	void clear()
	{
		m_ixm.clear();
		m_atoms.clear();
		m_num_refs = 0;
	};


	bool atom_is_used(size_t idx) const;

	void inherit(const ast_maps& other, std::span<const variable> ec);

	bool has_templates() const;

	const indexed_maps::compos& get_map(size_t idx) const
	{
		return m_ixm.m_maps[idx + m_num_refs];
	}

private:

	void put_atom(size_t idx, std::span<const variable> ec);
	void put_value(const ims_val* v, std::span<const variable> ec);
};