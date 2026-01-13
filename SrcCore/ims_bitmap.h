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
#include "ims_image.h"

struct ims_rgba
{
	uint8_t r, g, b, a;
	void clear() { r = g = b = a = 0; };
};

using ims_bitmap = ims_image<ims_rgba>;

//draw a red and green frame
void draw_frame(ims_bitmap& bmp);

//make a black transparent image
void clear_color(ims_bitmap& bmp);


//cx,cy - upper left corner
//sx,sy - size
//sx,sy,cx,cy input: ROI, output: result
//if result is empty, returns false

bool crop_alpha_rect(const ims_bitmap&,
	size_t& cx, size_t& cy,
	size_t& sx, size_t& sy);

//copies one image to a part of another
void copy_scale_subimage(
	ims_bitmap& vd,
	size_t dst_w,
	size_t dst_h,
	const ims_bitmap& vs,
	//region on the resulting image
	size_t dx,
	size_t dy,
	size_t tx,
	size_t ty);

//get the opacity rectangle, inclusive
void get_alpha_limits(const ims_bitmap& vs,
	size_t& x1, size_t& x2,
	size_t& y1, size_t& y2);
	
void bitmap_blend(ims_bitmap& bmp, 
	uint8_t r, uint8_t g, uint8_t b);

