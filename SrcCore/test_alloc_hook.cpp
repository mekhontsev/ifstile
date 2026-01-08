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
ims_static std::atomic<int> g_num_hooks = 0;

//is it enabled for the current thread
static thread_local bool s_enabled = false;

static int s_hook(int allocType, void* userData, size_t size, 
	int blockType, long requestNumber, 
	const unsigned char* filename, int lineNumber)
{
	assert(
		blockType == _CRT_BLOCK ||
		!s_enabled
	);

	auto* old = s_old;

	if (!old) {
		return 1;	
	}
	
	return old(allocType, userData, size, blockType, requestNumber, filename, lineNumber);
}


test_alloc_hook::test_alloc_hook(bool enabled /*= true*/): m_enabled(enabled)
{
	assert(!s_enabled);
	if (!m_enabled)return;
	
	//////////////////////////////////
	s_enabled = true;
	if (++g_num_hooks == 1) {
		s_old = _CrtSetAllocHook(s_hook);
	}
}

test_alloc_hook::~test_alloc_hook()
{
	s_enabled = false;
	if (!m_enabled)return;
	//////////////////////////////////

	if (--g_num_hooks == 0) {
		_CrtSetAllocHook(s_old);
		s_old = nullptr;
	}
	
}

#endif// TEST_ALLOC_HOOK_READY