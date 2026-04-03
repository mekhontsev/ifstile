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
#include "ifs_renderer.h"

#include <emscripten.h>


void ext_console_clear(){};
bool ims_need_stop(){return false;}

// Stub out Emscripten's memory-growth notification.
// Eliminates the "env" import from the WASM binary.
extern "C" void emscripten_notify_memory_growth(int) {}

// Global renderer
static ifs_renderer g_renderer;

// Global output bitmap: reused across render calls to avoid allocations.
static ims_bitmap g_bitmap;

extern "C" {

// Initialize renderer with an AIFS fractal definition.
// aifs_text — null-terminated UTF-8 string.
// Returns 1 on success, 0 on failure.
EMSCRIPTEN_KEEPALIVE
int ifslib_init(const char* aifs_text)
{
	return g_renderer.init(aifs_text) ? 1 : 0;
}

// Render into an internal RGBA bitmap of size width x height.
// Returns pointer to raw RGBA pixel data (width * height * 4 bytes),
// or NULL on failure. Valid until the next ifslib_render call.
EMSCRIPTEN_KEEPALIVE
const uint8_t* ifslib_render(int width, int height, float quality, float thickness)
{

	g_bitmap.recreate(static_cast<size_t>(width), static_cast<size_t>(height));
	if (g_bitmap.empty())
		return nullptr;

	if (!g_renderer.render(g_bitmap, quality, thickness))
		return nullptr;

	return reinterpret_cast<const uint8_t*>(g_bitmap.data());
}

} // extern "C"