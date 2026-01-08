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

//if always stride == sizeof(T) then use std::span !
template<typename T>
struct ims_view
{
	ims_view(const T* d, size_t st = sizeof(T)) :
		m_data{ d }, m_stride{ st }
	{
		assert(st >= sizeof(T));
	};

	template<typename Container>
	ims_view(const Container& c) : ims_view(c.data()) {};

	const T& operator [](size_t idx) 
	{
		return *shift(m_data, idx);
	}

	const T* shift(const T* p, size_t num_el = 1) const
	{
		return reinterpret_cast<const T*>(
			reinterpret_cast<const uint8_t*>(p) + m_stride * num_el);
	};

private:

	const T* m_data;
	size_t m_stride;
};

template <typename Container>
ims_view(const Container&) -> ims_view<typename Container::value_type>;
