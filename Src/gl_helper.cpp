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
#include "gl_helper.h"

#ifdef IMS_USE_DX11

#include "dx_helper.h"
dx11_helper& get_d3d_device();

#else

static void upload_texture_RGBA_GL(uintptr_t texID, size_t w, size_t h, void* data)
{
	assert(GL_NO_ERROR == glGetError());

	glBindTexture(GL_TEXTURE_2D, (GLuint)texID);

	/////////////////////////

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
#ifdef GL_UNPACK_ROW_LENGTH
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	/////////////////////////

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		(GLuint)w,
		(GLuint)h,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		data
	);
	glBindTexture(GL_TEXTURE_2D, 0);
	assert(GL_NO_ERROR == glGetError());
}

#endif


//get the nearest power of two
static size_t nearest_pow2(size_t v)
{
	size_t p = 1;
	while (p < v)p *= 2;
	return p;
};


void gl_helper::delete_screen_texture() 
{
#ifdef IMS_USE_DX11
	if (m_screen_texture) {
		auto* tex = (ID3D11ShaderResourceView*)m_screen_texture;
		tex->Release();
		m_screen_texture = 0;
	}
#else	
	auto id = (GLuint)m_screen_texture;
	glDeleteTextures(1, &id);
	m_screen_texture = 0;
#endif

};

void gl_helper::init()
{
#ifdef IMS_USE_DX11

#else
	GLuint id;
	glGenTextures(1, &id);
	m_screen_texture = id;
#endif


}

void gl_helper::deinit()
{
	delete_screen_texture();
}

bool gl_helper::prepare_screen_bitmap(const ims_bitmap& bmp, size_t gw, size_t gh) 
{
	assert(gw > 0 && gh > 0);

	let w = bmp.w();
	let h = bmp.h();

	auto icx = m_cx * w / gw;
	auto icy = m_cy * h / gh;

	auto isx = m_sx * w / gw;
	auto isy = m_sy * h / gh;


	assert(icx + isx <= w);
	assert(icy + isy <= h);

	let tx = nearest_pow2(isx);
	let ty = nearest_pow2(isy);

	if (tx == 0 || ty == 0) {
		return false;
	}

	m_scale[0] = float(w) / float(gw * tx);
	m_scale[1] = float(h) / float(gh * ty);

	m_tm_bmp.recreate(tx, ty);
	clear_color(m_tm_bmp);

	let sw = bmp.w();
	let* src = bmp.row_begin(icy) + icx;
	auto* dst = m_tm_bmp.row_begin(0);
	for (size_t y = 0; y < isy; ++y) {
		std::copy(src, src + isx, dst);
		src += sw;
		dst += tx;
	}

	return true;

}

void gl_helper::upload_screen_bitmap()
{
	/////////////////////////////////////////////////////////////////////////////
#if defined(IMS_USE_DX11)
	delete_screen_texture();
	m_screen_texture = get_d3d_device().upload_texture_RGBA(
		m_tm_bmp.w(), m_tm_bmp.h(), m_tm_bmp.row_begin(0));
#else
	//no need to delete_screen_texture because we're reusing it
	upload_texture_RGBA_GL(m_screen_texture, 
		m_tm_bmp.w(), m_tm_bmp.h(), m_tm_bmp.row_begin(0));
#endif
};




bool gl_helper::set_region(size_t cx, size_t cy, size_t sw, size_t sh)
{
	if (m_cx == cx && m_cy == cy && m_sx == sw && m_sy == sh) {
		return false;
	}

	m_cx = cx;
	m_cy = cy;
	m_sx = sw;
	m_sy = sh;

	return true;
}

uintptr_t gl_helper::get_screen_texture() const
{
	return m_screen_texture;
}


void gl_helper::clear_tmp_bmp()
{
	m_tm_bmp.recreate(0, 0);
}

float gl_helper::get_scale(uint8_t idx) const
{
	return m_scale[idx];
}

