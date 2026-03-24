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

#ifdef _MSC_VER
#include "windows.h"
#endif

#include "platform.h"
#include "version.h"
#include "utf8.h"
#include <sys/stat.h>


#if defined(__EMSCRIPTEN__)
void Emscripten_window_open(std::string_view url);
void Emscripten_set_clipboard(std::string_view str);
#endif


void SDL_EnableWindow(SDL_Window* sdlWindow, bool enable);
SDL_Window* MainWindow_get();


namespace platform
{

std::string_view getPathPref()
{
	static std::string ret;
	if (!ret.empty())return ret;
#if defined(__EMSCRIPTEN__)
	ret = "/IFStile/"; //don't use what SDL provides('/libsdl/...')
#else
	auto* path = SDL_GetPrefPath(nullptr, APPLICATION_TITLE);
	if (path) {
		ret = path;
		SDL_free(path);
	} else {
		ret = "~/";
	}
#endif
	return ret;
}


std::string_view getPathHome()
{
	static std::string ret;
	if (!ret.empty()) return ret;

#if defined(__EMSCRIPTEN__)
	ret = getPathPref();
#else
	const char* path = SDL_GetUserFolder(SDL_FOLDER_HOME);
	if (path) {
		ret = path;
		//unlike SDL_GetPrefPath, the caller must NOT free the memory
	} else {
		ret = getPathPref();
	}
#endif

	return ret;
};

void open_url(const std::string& url)
{
#if defined(__EMSCRIPTEN__)
	Emscripten_window_open(url);
#else
	SDL_OpenURL(url.c_str());
#endif
}

void ims_to_clipboard(const std::string& str)
{
	if (str.empty())return;
#ifdef __EMSCRIPTEN__	
	Emscripten_set_clipboard(str);
#else
	SDL_SetClipboardText(str.c_str());
#endif
}

void set_app_icon_rgba(void* pixels, int w, int h)
{
	const int pitch = w * 4;

	auto* icon = SDL_CreateSurfaceFrom((int)w, (int)h,
		SDL_PIXELFORMAT_RGBA32, pixels, pitch);

	IMS_SCOPE([&] { SDL_DestroySurface(icon); });

	assert(icon);

	SDL_SetWindowIcon(MainWindow_get(), icon);
}

bool remove_empty_directory(const char* fn)
{
#if defined( _MSC_VER)
	if (RemoveDirectoryW(utf8_to_native(fn).c_str())) {
		return true;
	}
#else //UNIX
	if (rmdir(fn) == 0) {
		return true;
	}
#endif

	return false;
}

bool remove_file(const char* fn)
{
#if defined( _MSC_VER)
	if (DeleteFileW(utf8_to_native(fn).c_str())) {
		return true;
	}
#else //UNIX
	if (unlink(fn) == 0) {
		return true;
	}
#endif

	return false;
}

bool create_directory(const char* fn)
{
#if defined( _MSC_VER)
	if (CreateDirectoryW(utf8_to_native(fn).c_str(), nullptr)) {
		return true;
	}
#else //UNIX
	if (mkdir(fn, 0700) == 0) {
		return true;
	}
#endif
	return false;
}

window_disabler::window_disabler() 
{
	::SDL_EnableWindow(MainWindow_get(), false); 
}
window_disabler::~window_disabler() 
{
	::SDL_EnableWindow(MainWindow_get(), true); 
};


void message(const char* msg, const char* title)
{
	window_disabler wd;
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
		title ? title : APPLICATION_TITLE, msg, MainWindow_get());
};


bool is_folder_exists(const std::string& path)
{
	struct stat info{};

	if (stat(path.c_str(), &info) != 0) {
		return false;
	}

	return info.st_mode & S_IFDIR;
}


}
