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
#include "test_alloc_hook.h"

#ifdef TEST_ALLOC_HOOK_READY

ims_static _CRT_ALLOC_HOOK s_old = nullptr;

//check depth for the current thread
static thread_local int s_depth = 0;

static int s_hook(int allocType, void* userData, size_t size, 
	int blockType, long requestNumber, 
	const unsigned char* filename, int lineNumber)
{
	assert(blockType == _CRT_BLOCK || s_depth == 0);

	if (!s_old) {
		return 1;	
	}
	
	return s_old(allocType, userData, size, blockType, requestNumber, filename, lineNumber);
}

test_alloc_hook::test_alloc_hook(bool enabled /*= true*/): m_enabled(enabled)
{
	if (!m_enabled)return;
	++s_depth;
}

test_alloc_hook::~test_alloc_hook()
{
	if (!m_enabled)return;
	--s_depth;
}

void test_alloc_hook::init()
{
	s_old = _CrtSetAllocHook(s_hook);
}

#endif// TEST_ALLOC_HOOK_READY
