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

struct variable;
struct ims_val;

struct ast_maps: public boost::noncopyable
{
	
	std::vector<ast_context> m_atoms;
	indexed_maps m_ixm;
	size_t m_num_refs = 0;

	////////////////////////////////////////////////////////////////////////////

	void clear()
	{
		m_ixm.clear();
		m_atoms.clear();
		m_num_refs = 0;
	};

	ast_context get_graph_atom(size_t idx) const
	{
		let ai = m_ixm.get_atom(idx);
		if (ai >= m_num_refs) {
			return m_atoms[ai - m_num_refs];
		}
		ast_context ret;
		ret.call_offset = 0;
		ret.h.set_reference(ai);
		ret.a = nullptr;
		return ret;
	};


	static bool atom_is_used(const ast_context& c);

	void inherit(const ast_maps& other, std::span<const variable> ec);

	bool has_tempaltes() const;

	const indexed_maps::compos& get_map(size_t idx) const
	{
		return m_ixm.m_maps[idx + m_num_refs];
	}

private:

	void put_atom(size_t idx, std::span<const variable> ec);
	void put_value(const ims_val* v, std::span<const variable> ec);
};