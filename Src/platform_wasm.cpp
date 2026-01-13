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

#ifdef __EMSCRIPTEN__

#include "pch.h" 
#include "version.h"
#include <emscripten/threading.h>


void set_thread_name(const char* name)
{
#if !defined(NDEBUG) || defined(DEVELOPER_VERSION)
	emscripten_set_thread_name(pthread_self(), name);
#else 
	(void)name;
#endif
}

void ext_prevent_sleep_mode(bool) 
{
	
}

void SDL_EnableWindow(SDL_Window*, bool)
{
	
};


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"

void Emscripten_download(std::string_view fn)
{
	EM_ASM_({ Module.dfile(UTF8ToString($0, $1)); }, fn.data(), fn.size());
};

//will cause the user to see a file "send" dialog box and,
//after selecting, an "open file" command will be sent to the main thread.
void Emscripten_upload()
{
	EM_ASM_({ Module.ufile('.aifs, .ifs, .png, .gpl'); });
};

void Emscripten_window_open(std::string_view url)
{
	EM_ASM_({ window.open(UTF8ToString($0, $1)); }, url.data(), url.size());
};

void Emscripten_set_clipboard(std::string_view str)
{
	EM_ASM_({ Module.copyToClipbard(UTF8ToString($0, $1)); }, str.data(), str.size());
};

#pragma GCC diagnostic pop


#endif //__EMSCRIPTEN__

