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

#ifdef __APPLE__

#include "pch.h" 
#include "version.h"

#import <IOKit/pwr_mgt/IOPMLib.h>


void set_thread_name(const char* ) 
{
	//[[NSThread currentThread] setName:@"My thread name"];
}

void ext_prevent_sleep_mode(bool prevent) 
{
	ims_func_static IOPMAssertionID assertionID;
	ims_func_static bool s_prevented = false;

	IOReturn success;

	if (prevent) {
		if (s_prevented)return;//already 
		success = IOPMAssertionCreateWithName(
			kIOPMAssertionTypeNoIdleSleep,
			kIOPMAssertionLevelOn,
			CFSTR("Calculation in progress"),
			&assertionID);
		
	} else {
		if (!s_prevented)return;//already
		success = IOPMAssertionRelease(assertionID);
	}

	if (success == kIOReturnSuccess) {
		s_prevented = !s_prevented;
	}
}

void SDL_EnableWindow(SDL_Window*, bool)
{
	/*
	#include <AppKit/NSWindow.h>

	void enableWindow(NSWindow* window, bool enable)
	{
	[window setMovable : enable];
	}
	*/
};

#endif //__APPLE__
