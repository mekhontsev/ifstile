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

template<typename T> struct ims_image;
using ims_bitmap = ims_image<struct ims_rgba>;

namespace ims_png
{
//the result must be removed using rgba_free
uint8_t* rgba_from_mem(const void* mem, size_t len, size_t& w, size_t &h);

void rgba_free(void* pix);

bool save(
	const std::function<void(const void*, size_t)>& of,
	std::string_view aifs,
	const ims_bitmap& bmp,
	bool crop, 
	const uint8_t* background);

bool find_aifs(std::istreambuf_iterator<char>& iter);

};


