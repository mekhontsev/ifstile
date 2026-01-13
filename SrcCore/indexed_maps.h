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


//stores maps as compositions of the "simplest" ones specified by indices
struct indexed_maps
{
	struct compos
	{
		size_t start = 0;	//initial element in m_compos
		size_t num = 0;		//number of elements
	};

	std::vector<compos> m_maps;

	//all maps are stored in this array in ranges
	std::vector<size_t> m_compos;
	////////////////////////////////////////////////////////////////////////////
	
	void clear();

	//multiply several maps and return the resulting one
	size_t mul_maps(const size_t* pm, size_t num);

	bool equal(size_t m1, size_t m2) const;
	size_t get_hash(size_t m) const;

	size_t get_atom(size_t idx) const;

};
