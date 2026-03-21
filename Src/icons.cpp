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
#include "embed/icon256.png.h"
#include "png_support.h"

namespace platform
{
	void set_app_icon_rgba(void* pixels, int w, int h);
}

void set_app_icon()
{
	size_t w = 0, h = 0;
	auto* mem = ims_png::rgba_from_mem(embed::ifstile256, boost::size(embed::ifstile256), w, h);
	IMS_SCOPE([&] {ims_png::rgba_free(mem); });
	assert(mem);
	platform::set_app_icon_rgba(mem, (int)w, (int)h);
}
