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
#include <wasi/api.h>


void ext_console_clear(){};
bool ims_need_stop(){return false;}

// Stub out Emscripten's memory-growth notification.
// Eliminates the "env" import from the WASM binary.
extern "C" void emscripten_notify_memory_growth(int) {}

// WASI Preview 1 stubs — eliminates wasi_snapshot_preview1 imports.
// Exact set derived from: WebAssembly.Module.imports(IFSlib.wasm)
extern "C" {

__wasi_errno_t __wasi_environ_get(uint8_t**, uint8_t*)                            { return __WASI_ERRNO_SUCCESS; }
__wasi_errno_t __wasi_environ_sizes_get(__wasi_size_t* c, __wasi_size_t* s)       { *c = 0; *s = 0; return __WASI_ERRNO_SUCCESS; }

__wasi_errno_t __wasi_clock_time_get(__wasi_clockid_t, __wasi_timestamp_t, __wasi_timestamp_t* t) {
    *t = 0; return __WASI_ERRNO_SUCCESS;
}

__wasi_errno_t __wasi_fd_close(__wasi_fd_t)                                       { return __WASI_ERRNO_SUCCESS; }
__wasi_errno_t __wasi_fd_read(__wasi_fd_t, const __wasi_iovec_t*, size_t, __wasi_size_t* n) {
    *n = 0; return __WASI_ERRNO_BADF;
}
__wasi_errno_t __wasi_fd_write(__wasi_fd_t, const __wasi_ciovec_t*, size_t, __wasi_size_t* n) {
    *n = 0; return __WASI_ERRNO_SUCCESS;
}
__wasi_errno_t __wasi_fd_seek(__wasi_fd_t, __wasi_filedelta_t, __wasi_whence_t, __wasi_filesize_t* p) {
    *p = 0; return __WASI_ERRNO_SUCCESS;
}

__wasi_errno_t __wasi_random_get(uint8_t* buf, __wasi_size_t len) {
    uint32_t s = 0xDEADBEEFu;
    for (__wasi_size_t i = 0; i < len; ++i) {
        s = s * 1664525u + 1013904223u;
        buf[i] = static_cast<uint8_t>(s >> 24);
    }
    return __WASI_ERRNO_SUCCESS;
}

} // extern "C" (WASI stubs)

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