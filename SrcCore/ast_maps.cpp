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
#include "ast_maps.h"
#include "ims_val.h"
#include "variable.h"


static constexpr size_t max_co = ims_max >> 1;



void ast_maps::inherit(const ast_maps& other, std::span<const variable> ec)
{
	m_ixm.m_compos.clear();


	let nmaps = other.m_ixm.m_maps.size();

	m_num_refs = ec.size();
	let nvars = m_num_refs;

	//at the beginning, m_ixm.m_maps stores cached variables
	ims_resize(m_ixm.m_maps, nvars + nmaps);
	for (size_t i = 0; i < nvars; ++i) {
		m_ixm.m_maps[i].start = ims_max;//it was not cached
	}

	////////////////////////////////////////////////////////////////////////////
	//filling atoms from the base graph
	m_atoms.resize(other.m_atoms.size() + nvars);

	//beginning reserved for atom-references
	for (size_t i = 0; i < nvars; ++i) {
		m_atoms[i] = ec[i].c;
	}

	//then come the edge atoms, then the remaining atoms (fills put_atom below)
	for (size_t i = 0; i < other.m_atoms.size(); ++i) {
		m_atoms[i + nvars] = other.m_atoms[i];
	}
	////////////////////////////////////////////////////////////////////////////
	//fill the edge maps by recursively substituting variables and compositions
	for (size_t i = 0; i < nmaps; ++i) {

		let start = m_ixm.m_compos.size();

		let& c = other.m_ixm.m_maps[i];
		for (size_t j = 0; j < c.num; ++j) {
			auto ai = other.m_ixm.m_compos[j + c.start];
			if (ai >= other.m_num_refs) {
				ai += nvars - other.m_num_refs;
			}
			put_atom(ai, ec);
		}

		m_ixm.m_maps[i + nvars] = { start, m_ixm.m_compos.size() - start };
	}

	////////////////////////////////////////////////////////////////////////////
	//fill in the usage flag
	
	for (auto& a : m_atoms) {
		a.call_offset += max_co;
	}

	for (size_t i = 0; i < nmaps; ++i) {
		let& m = get_map(i);
		for (size_t j = 0; j < m.num; ++j) {
			auto& a = m_atoms[m_ixm.get_atom(j + m.start)];
			if (a.call_offset >= max_co) {
				a.call_offset -= max_co;//restore for useful
			}
		}
	}
}


bool ast_maps::atom_is_used(const ast_context& c)
{
	return c.call_offset < max_co;
}

void ast_maps::put_value(const ims_val* v, std::span<const variable> ec)
{
	assert(v);

	if (v->is(ims_val_b::ETP::compos)) {
		auto** x = v->p_v();
		for (let** ae = x + v->get_size(); x < ae; ++x) {
			put_value(*x, ec);
		}
		return;
	}

	let idx = m_atoms.size();

	if (v->is(ims_val_b::ETP::ast_ptr)) {
		let* ast = v->template gp<ast_context>();
		if (ast->h.tt == ETYPE::reference) {
			put_atom(ast->get_ref_idx(), ec);
			return;
		}
		m_atoms.emplace_back(*ast);
	}else if (v->is(ims_val_b::ETP::uni)) {//empty union
		assert(v->is_empty());
		m_atoms.emplace_back();
	} else {
		m_atoms.emplace_back();
		assert(false);
	}

	put_atom(idx, ec);
};

bool ast_maps::has_tempaltes() const
{
	for (size_t i = 0; i < m_atoms.size(); ++i) {
		let& ast = m_atoms[i];
		if (!ast_maps::atom_is_used(ast)) continue;

		if (ast.h.is_template()) {//a new variation has appeared
			return true;
		}
	}
	return false;
}

void ast_maps::put_atom(size_t ai, std::span<const variable> ec)
{
	auto& c = m_ixm.m_compos;

	if (ai >= ec.size()) {
		c.emplace_back(ai);//insert directly
		return;
	}

	let& q = ec[ai];
	if (q.v[0]) {
		c.emplace_back(ai);//insert directly
		return;
	}

	auto& m = m_ixm.m_maps[ai];//try the cache

	if (m.start != ims_max) {//take from cache
		for (size_t i = 0; i < m.num; ++i) {
			c.emplace_back(c[i + m.start]);
		}
	} else {
		assert(q.ready[1]);
		let start = c.size();
		put_value(q.v[1].get(), ec);//topological value
		m = { start, c.size() - start };//caching
	}
};
