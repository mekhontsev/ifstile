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
#include "mpeg_support.h"
#include "ims_bitmap.h"
#include "jo_mpeg.h"

void ims_mpeg::write_file(
	std::ostream& of, 
	const ims_bitmap& bitmap)
{
	let w = bitmap.w();
	let h = bitmap.h();
	m_mem.resize(w*h * 4);

	let s = encode_mpeg(m_mem.data(), (uint8_t*)bitmap.data(), (int)w, (int)h, 60);
	of.write((const char*)m_mem.data(), s);
}

