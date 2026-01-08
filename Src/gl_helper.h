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
#include "ims_bitmap.h"

struct gl_helper
{
	void init();
	void deinit();

	//copy part of the image to the texture image,
	bool prepare_screen_bitmap(const ims_bitmap& bmp, size_t gw, size_t gh);

	//fill the texture image into video memory
	void upload_screen_bitmap();


	////////////////////////////////////////////////////////////////
	bool set_region(size_t cx, size_t cy, size_t sw, size_t sh);

	bool empty() const { return m_tm_bmp.empty(); } ;

	uintptr_t get_screen_texture() const;


	void clear_tmp_bmp();

	float get_scale(uint8_t idx) const;

	void delete_screen_texture();

private:
	

	std::array<float, 2> m_scale = { 0,0 };

	//size and position of the fragment
	size_t m_cx = 0, m_cy = 0, m_sx = 0, m_sy = 0;

	//texture image, dimensions - powers of two
	//corresponds to the portion currently visible on the screen
	ims_bitmap m_tm_bmp;

	uintptr_t m_screen_texture = 0;

};

