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
#include "indexed_maps.h"

void indexed_maps::clear()
{
	m_compos.clear();
	m_maps.clear();
}


size_t indexed_maps::mul_maps(const size_t* pm, size_t num)
{
	assert(num > 0);

	let* p0 = pm;
	let* p1 = pm + num;
	
	//cut off the beginning (identity maps)
	for (; p0 < p1; ++p0) {
		if (m_maps[*p0].num > 0) {
			break;
		}
	}

	if (p0 == p1) {
		return pm[0];//all maps are identity
	}

	//cut off the end (identity maps)
	p1 -= 1;
	for (; p1 > p0; --p1) {
		if (m_maps[*p1].num > 0) {
			break;
		}
	}
	assert(p1 >= p0);

	if (p0 == p1) {
		return *p0;//no new map needed
	}

	let start = m_compos.size();
	for (; p0 <= p1; ++p0) {
		let& m = m_maps[*p0];
		ims_geometric_reserve(m_compos, m.num);
		m_compos.insert(
			m_compos.end(),
			m_compos.begin() + m.start,
			m_compos.begin() + m.start + m.num);
	}

	let ret = m_maps.size();
	m_maps.push_back({ start, m_compos.size() - start });
	assert(m_maps[ret].num >= 2);
	return ret;
}

bool indexed_maps::equal(size_t m1, size_t m2) const
{
	if (m1 == m2)return true;

	let& c1 = m_maps[m1];
	let& c2 = m_maps[m2];

	if (c1.num != c2.num)return false;
	if (c1.num == 0)return true;

	let* p = m_compos.data();
	return std::equal(p + c1.start, p + c1.start + c1.num, p + c2.start);
}

size_t indexed_maps::get_hash(size_t m) const
{
	let& c = m_maps[m];

	let* p = m_compos.data() + c.start;
	let* pe = p + c.num;

	size_t ret = c.num;
	boost::hash_range(ret, p, pe);
	return ret;
}

size_t indexed_maps::get_atom(size_t idx) const
{
	return m_compos[idx];
}
