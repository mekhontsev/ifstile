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


#ifdef _MSC_VER
#include <stdio.h>
FILE* fopen_utf8(const char* filename, const char* mode);
#define CUTE_PNG_FOPEN fopen_utf8
#endif//_MSC_VER


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4189)
#pragma warning(disable : 4505)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#define CUTE_PNG_IMPLEMENTATION
#include "cute_png.h"

#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

void ims_png_free(void* pix)
{
	CUTE_PNG_FREE(pix);
}

//modified body of the cp_save_png_to_memory function
cp_saved_png_t ims_save_png_to_memory(
	const cp_image_t* img,
	const char* text_chank,
	size_t text_chank_size)
{
	cp_saved_png_t result = { 0 };
	cp_save_png_data_t s = { 0 };
	cp_lz77_state_t* lz = NULL;
	long dataPos, dataSize, fileSize;
	if (!img) return result;

	// Allocate LZ77 state (~72KB)
	lz = (cp_lz77_state_t*)CUTE_PNG_ALLOC(sizeof(cp_lz77_state_t));
	if (!lz) return result;

	s.adler = 1;
	s.bits = 0x80;
	s.bufcap = 1024;
	s.buffer = (char*)CUTE_PNG_ALLOC(1024);
	s.lz = lz;

	cp_save_header(&s, (cp_image_t*)img);

	if (text_chank && text_chank_size > 0) {
		cp_begin_chunk(&s, "tEXt", (uint32_t)text_chank_size);
		for (size_t i = 0; i < text_chank_size; ++i) {
			cp_put8(&s, (uint8_t)text_chank[i]);
		}
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

	CUTE_PNG_FREE(lz);

	result.size = fileSize;
	result.data = s.buffer;
	return result;
}
