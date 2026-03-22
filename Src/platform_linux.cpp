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

#if defined(__GNUC__) && !defined(__clang__)


#include "pch.h" 

#include "version.h"


////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
void ext_heap_init() 
{
	
}

////////////////////////////////////////////////////////////////////////////////
void set_thread_name(const char*)
{

}

////////////////////////////////////////////////////////////////////////////////
//prevents the computer from going to sleep completely
void ext_prevent_sleep_mode(bool /*prevent*/)
{

}

void SDL_EnableWindow(SDL_Window*, bool)
{

};


void setup_exception_handler()
{

}

#endif //
