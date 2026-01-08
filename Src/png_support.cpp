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
#include "png_support.h"
#include "ims_bitmap.h"
#include "ims_keywords.h"


#ifdef _MSC_VER
FILE* fopen_utf8(const char* filename, const char* mode);
#define CUTE_PNG_FOPEN fopen_utf8
#endif//_MSC_VER

#define CUTE_PNG_IMPLEMENTATION
#include "cute_png.h"

//to check the PNG write: http://entropymine.com/jason/tweakpng/

namespace ims_png
{

static constexpr std::string_view png_chunk_aifs{ "ifstile.aifs" };

uint8_t* rgba_from_mem(const void* mem, size_t len, size_t& w, size_t &h) 
{
	auto img = cp_load_png_mem(mem, (int)len);
	w = (size_t)img.w;
	h = (size_t)img.h;
	return (uint8_t*)img.pix;
};

void rgba_free(void* pix)
{
	CUTE_PNG_FREE(pix);
};

#if 0
static bool read_mem(const void* mem, size_t len, ims_bitmap& bitmap)
{
	auto img = cp_load_png_mem(mem, (int)len);
	IMS_SCOPE([&] {free(img.pix); });

	if (!img.pix) {
		return false;
	}

	bitmap.recreate(img.w, img.h);


	size_t i = 0;

	bitmap.for_each([&img, &i](auto& q) {
		let& p = img.pix[i++];
		q.r = p.r;
		q.g = p.g;
		q.b = p.b;
		q.a = p.a;
	});
	

	return true;
};
#endif

using key_vals_type = std::vector<std::pair<std::string_view, std::string_view>>;

//the result must be removed using CUTE_PNG_FREE
static uint8_t* write_mem(
	size_t& fileSize,
	const ims_bitmap& v,
	const uint8_t* background,
	size_t cx, size_t cy,
	size_t sx, size_t sy,
	const key_vals_type& keyvals)
{

	std::vector<cp_pixel_t> pixels;
	pixels.resize(sx*sy);

	size_t i = 0;
	for (size_t y = 0; y < sy; ++y) {
		let* it = v.row_begin(sy-1-y+cy) + cx;
		let* ite = it + sx;
		while (it < ite){
			let& d = *it++;
			auto& p = pixels[i++];
			if (!background) {
				p.r = d.r;
				p.g = d.g;
				p.b = d.b;
				p.a = d.a;
			} else {
				let a = d.a / 255.0f;
				p.r = uint8_t(d.r * a + background[0] * (1 - a));
				p.g = uint8_t(d.g * a + background[1] * (1 - a));
				p.b = uint8_t(d.b * a + background[2] * (1 - a));
				p.a = 255;
			}
		}
	}

	cp_image_t timg;
	timg.w = (int)sx;
	timg.h = (int)sy;
	timg.pix = pixels.data();

	////////////////////////////////////////////////////////////////////////////
	let* img = &timg;

	//modified body of the cp_save_png_to_memory function
	cp_save_png_data_t s = { 0 };
	long dataPos, dataSize;
	
	s.adler = 1;
	s.bits = 0x80;
	s.prev = 0xFFFF;
	s.bufcap = 1024;
	s.buffer = (char*)CUTE_PNG_ALLOC(1024);

	cp_save_header(&s, (cp_image_t*)img);

	////////////////////////////////////////////
	for (let& q : keyvals) {
		let& key = q.first;
		let& val = q.second;

		let n = key.length() + val.length() + 2;

		cp_begin_chunk(&s, "tEXt", (uint32_t)n);

		for (let& c : key)cp_put8(&s, (uint8_t)c);
		cp_put8(&s, 0);
		for (let& c : val)cp_put8(&s, (uint8_t)c);
		cp_put8(&s, 0);

		cp_put32(&s, ~s.crc);
	}

	dataPos = s.buflen;


	cp_save_data(&s, (cp_image_t*)img, dataPos, &dataSize);

	// End chunk.
	cp_begin_chunk(&s, "IEND", 0);
	cp_put32(&s, ~s.crc);

	// Write back payload size.
	fileSize = s.buflen;
	s.buflen = dataPos;
	cp_put32(&s, dataSize);

	return  (uint8_t*)s.buffer;
}


#if 0
static std::string get_text_chunk_name(std::string_view key)
{
	std::string ret("tEXt");
	ret += key;
	ret.push_back(0);
	return ret;
}
#endif

bool save(
	const std::function<void(const void*, size_t)>& of,
	std::string_view aifs, 
	const ims_bitmap& bmp, 
	bool crop,
	const uint8_t* background)
{
	size_t cx = 0, cy = 0, sx = bmp.w(), sy = bmp.h();

	if (crop && !crop_alpha_rect(bmp, cx, cy, sx, sy)) {
		return false;//empty image
	}

	key_vals_type keyvals;

	if (!aifs.empty()) {
		keyvals.emplace_back(png_chunk_aifs, aifs);
	}

	size_t buf_size = 0;
	auto* buf = write_mem(buf_size, bmp, background, cx, cy, sx, sy, keyvals);

	if (!buf)return false;

	IMS_SCOPE([buf] {
		CUTE_PNG_FREE(buf);
	});

	of(buf, buf_size);
	return true;
}

bool find_aifs(std::istreambuf_iterator<char>& iter)
{
	constexpr auto end = std::istreambuf_iterator<char>();

	constexpr auto sz = png_chunk_aifs.length();
	std::string buf;
	buf.reserve(sz + 1);

	for (size_t i = 0; i < sz; ++i) {
		if (iter == end)return false;
		buf += *iter++;
	};

	while (buf != png_chunk_aifs) {
		if (iter == end)return false;
		buf.erase(0, 1);
		buf += *iter++;
	}

	if (iter != end && *iter == 0) {
		++iter;
	}

	return true;
}

}//namespace ims_png