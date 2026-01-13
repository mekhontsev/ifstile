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
#include "ims_bitmap.h"

void clear_color(ims_bitmap& bmp)
{
	uint32_t* b = (uint32_t*)bmp.data();
	std::fill(b, b + bmp.w() * bmp.h(), (uint32_t)0);
};

void draw_frame(ims_bitmap& bmp)
{
	let w = bmp.w();
	let h = bmp.h();
	if(w < 2 || h < 2)return;

	auto& v = bmp;
	let c1 = ims_rgba{255, 0, 0, 255};
	let c2 = ims_rgba{0, 255, 0, 255};

	for(size_t i = 0; i < 1; ++i){
		let c = (i % 2 == 0) ? c1 : c2;
		for(size_t y = i; y < h - i; ++y){
			v.row_begin(y)[i] = c;
			v.row_begin(y)[w - 1 - i] = c;
		}
		for(size_t x = i; x < w - i; ++x){
			v.row_begin(i)[x] = c;
			v.row_begin(h - 1 - i)[x] = c;
		}
	}
}

bool crop_alpha_rect(const ims_bitmap& v,
	size_t& cx, size_t& cy,
	size_t& sx, size_t& sy)
{
	size_t
		x1 = sx,
		x2 = 0,
		y1 = sy,
		y2 = 0;

	for(size_t y = 0; y < sy; ++y){
		auto* it = v.row_begin(y + cy) + cx;
		for(size_t x = 0; x < sx; ++x, ++it){
			if(it->a == 0) continue;
			x1 = std::min(x, x1);
			x2 = std::max(x, x2);
			y1 = std::min(y, y1);
			y2 = std::max(y, y2);
		}
	}

	if(x2 < x1 || y2 < y1){
		return false;//empty image
	}

	sx = x2 - x1 + 1;
	sy = y2 - y1 + 1;

	cx += x1;
	cy += y1;

	return true;
}


void copy_scale_subimage(
	ims_bitmap& vd,
	size_t dst_w,
	size_t dst_h,
	const ims_bitmap& vs,
	size_t dx,
	size_t dy,
	size_t tx,
	size_t ty)
{
	let w = vs.w();
	let h = vs.h();
	if(w == 0 || h == 0)return;

	let mx = std::min(tx, dst_w - dx);
	let my = std::min(ty, dst_h - dy);

	//special case - without scaling
	if(w == tx && h == ty){
		for(size_t y = 0; y < my; y++){
			let* src = vs.row_begin(y);
			auto* dst = vd.row_begin(y + dy) + dx;
			std::copy(src, src + mx, dst);
		}
		return;
	}

	//scaling
	let wt = float(w) / float(tx);
	let ht = float(h) / float(ty);

	for(size_t y = 0; y < my; y++){
		//need to multiply ht * y, otherwise the "float" error is summed

		let ry = y * ht;
		let iy1 = size_t(ry);
		let* src1 = vs.row_begin(iy1);
#if 0		
		let fy = ry - std::floor(ry);
		let qy = 1 - fy;
		let iy2 = std::min(iy1 + 1, vs.height() - 1);
		let* src2 = vs.row_begin(iy2);
#endif

		auto* dst = vd.row_begin(y + dy) + dx;


		for(size_t x = 0; x < mx; x++){
			let rx = x * wt;
			let ix1 = size_t(rx);
			auto& d = dst[x];
#if 0

			let fx = rx - std::floor(rx);
			let qx = 1 - fx;
			let ix2 = std::min(ix1 + 1, vs.width() - 1);

			let p11 = src1[ix1];
			let p12 = src1[ix2];
			let p21 = src2[ix1];
			let p22 = src2[ix2];
			let qxqy = qx * qy;
			let fxqy = fx * qy;
			let qxfy = qx * fy;
			let fxfy = fx * fy;




			d.r = (uint8_t)(p11.r * qxqy + p12.r * fxqy + p21.r * qxfy + p22.r * fxfy);
			d.g = (uint8_t)(p11.g * qxqy + p12.g * fxqy + p21.g * qxfy + p22.g * fxfy);
			d.b = (uint8_t)(p11.b * qxqy + p12.b * fxqy + p21.b * qxfy + p22.b * fxfy);
			d.a = (uint8_t)(p11.a * qxqy + p12.a * fxqy + p21.a * qxfy + p22.a * fxfy);
#endif
			d = src1[ix1];
		}
	}
#if 0
	let xx = size_t(1 / wt / 2);
	let yy = size_t(1 / ht / 2);

	for(size_t y = yy; y < my; y++){
		//need to multiply ht * y, otherwise the "float" error is summed
		let* src = vs.row_begin((size_t)(ht * y));
		auto* dst = vd.row_begin(y + dy - yy) + dx - xx;
		for(size_t x = xx; x < mx; x++){
			dst[x] = src[(size_t)(x * wt)];
		}

		dst += mx;
		for(size_t x = 0; x < xx; x++){
			dst[x].clear();
		}
	}
	for(size_t y = my; y < my + yy; y++){
		auto* dst = vd.row_begin(y + dy - yy) + dx;
		for(size_t x = 0; x < mx; x++){
			dst[x].clear();
		}
	}
#endif
};

void bitmap_blend(ims_bitmap& bmp, uint8_t r, uint8_t g, uint8_t b)
{
	let w = bmp.w();
	let h = bmp.h();
	for(size_t y = 0; y < h; ++y){
		for(size_t x = 0; x < w; ++x){
			auto& px = bmp(x, h - y - 1);

			int ir = px.r;
			int ig = px.g;
			int ib = px.b;
			int ia = px.a;

			ir = (ir * ia + r * (255 - ia) + 128) / 255;
			ig = (ig * ia + g * (255 - ia) + 128) / 255;
			ib = (ib * ia + b * (255 - ia) + 128) / 255;

			px.r = (uint8_t)ir;
			px.g = (uint8_t)ig;
			px.b = (uint8_t)ib;

		}
	}
}



void get_alpha_limits(const ims_bitmap& v,
	size_t& x1, size_t& x2,
	size_t& y1, size_t& y2)
{
	x1 = v.w();
	x2 = 0;
	y1 = v.h();
	y2 = 0;

	for(size_t y = 0; y < v.h(); ++y){
		auto* it = v.row_begin(y);
		for(size_t x = 0; x < v.w(); ++x){
			let& d = *it++;
			if(d.a != 0){
				x1 = std::min(x, x1);
				x2 = std::max(x, x2);
				y1 = std::min(y, y1);
				y2 = std::max(y, y2);
			};
		}
	}
};