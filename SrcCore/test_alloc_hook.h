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

#if defined(_MSC_VER) && !defined(NDEBUG)
#define TEST_ALLOC_HOOK_READY
struct test_alloc_hook
{
	test_alloc_hook(bool enabled = true);
	~test_alloc_hook();

	bool m_enabled;
};
#define TEST_ALLOC_HOOK(v) test_alloc_hook tah(v)

#else

#define TEST_ALLOC_HOOK

#endif
