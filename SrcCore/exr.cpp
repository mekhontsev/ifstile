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
#include "exr.h"

//https://github.com/syoyo/tinyexr

#ifdef _MSC_VER
#pragma warning(disable : 4706)//assignment within conditional expression
#pragma warning(disable : 4127)//conditional expression is constant
#pragma warning(disable : 4245)//conversion from 'int' to 'size_t', signed/unsigned mismatch
#pragma warning(disable : 4702)//unreachable code
#endif

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

void exr_data::init(size_t width, size_t height, size_t num_chan)
{
	w = width;
	h = height;
	set_num_chan(num_chan);
}

void exr_data::set_num_chan(size_t nc)
{
	m_img.resize(nc);
}

void exr_data::set_channel(size_t idx, float* pixels, const char* chan_name)
{
	auto& c = m_img[idx];
	c.image = pixels;
	c.name = chan_name;
	c.is_float = true;
}

void exr_data::set_channel(size_t idx, uint8_t* pixels, const char* chan_name)
{
	auto& c = m_img[idx];
	c.image = pixels;
	c.name = chan_name;
	c.is_float = false;
}


bool exr_data::save(std::ostream& fs)
{
	unsigned char *mem = nullptr;
	const char* err = nullptr;

	EXRHeader hdr;
	InitEXRHeader(&hdr);

	EXRImage image;
	InitEXRImage(&image);

	IMS_SCOPE([&] {
		FreeEXRErrorMessage(err);
		if (mem)free(mem);

		//not necessary, because we manage the memory ourselves
		//FreeEXRImage(&image);
		//FreeEXRHeader(&hdr);
	});

	////////////////////////////////////////////////////////////////////////////
	
	let nc = m_img.size();
	image.num_channels = (int)nc;


	image.width = (int)w;
	image.height = (int)h;

	hdr.num_channels = (int)nc;

	std::vector<EXRChannelInfo> vec_ch(nc);
	std::vector<int> vec_pt(nc);
	std::vector<int> vec_rp(nc);

	hdr.channels = vec_ch.data();
	hdr.pixel_types = vec_pt.data();
	hdr.requested_pixel_types = vec_rp.data();

	hdr.compression_type = TINYEXR_COMPRESSIONTYPE_NONE;

#if 0 // example to write custom attribute
	int version_minor = 3;
	hdr.num_custom_attributes = 1;
	hdr.custom_attributes = reinterpret_cast<EXRAttribute *>
		(malloc(sizeof(EXRAttribute) * exr_header.custom_attributes));
	hdr.custom_attributes[0].name = strdup("tinyexr_version_minor");
	hdr.custom_attributes[0].type = strdup("int");
	hdr.custom_attributes[0].size = sizeof(int);
	hdr.custom_attributes[0].value = (unsigned char*)malloc(sizeof(int));
	memcpy(hdr.custom_attributes[0].value, &version_minor, sizeof(int));
#endif

	std::vector<void*> ptrs(nc);

	for (size_t i = 0; i < nc; ++i) {
		auto& c = m_img[i];
		assert(c.name.length() < 255);
		memset(&hdr.channels[i], 0, sizeof(EXRChannelInfo));
		strcpy(hdr.channels[i].name, c.name.c_str());

		int px_type = c.is_float ? 
			TINYEXR_PIXELTYPE_FLOAT : TINYEXR_PIXELTYPE_UINT;

		hdr.pixel_types[i] = px_type; // pixel type of input image
		// pixel type of output image to be stored in .EXR
		hdr.requested_pixel_types[i] = px_type; 


		ptrs[i] = c.image;
	}

	

	image.images = (unsigned char**)ptrs.data();

	
	size_t mem_size = SaveEXRImageToMemory(&image, &hdr, &mem, &err);
	
	if (mem_size == 0) {
		ims_error("Error: {}", err);
		return false;
	}	

	fs.write((const char*)mem, mem_size);

	return true;
}