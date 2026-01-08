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

#if defined( _MSC_VER)
#include <windows.h>
#else
#include <pthread.h>
#endif

////////////////////////////////////////////////////////////////////////////////
void set_thread_low_priority()
{
#if defined(_MSC_VER)
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#else
	int policy;
	struct sched_param param;
	pthread_getschedparam(pthread_self(), &policy, &param);
	param.sched_priority = sched_get_priority_min(policy);
	pthread_setschedparam(pthread_self(), policy, &param);
#endif
};
