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

#ifdef _MSC_VER

#include "pch.h" 
#include "windows.h"
#include "shellapi.h"
#include "version.h"
#include "utf8.h"

//#include "Uxtheme.h"
//#pragma comment(lib, "UxTheme.lib")


void SDL_set_high_DPI()
{
	//Try Windows 8.1+ version
	auto* shcoreDLL = SDL_LoadObject("shcore.dll");
	if (shcoreDLL) {
		typedef enum PROCESS_DPI_AWARENESS {
			PROCESS_DPI_UNAWARE = 0,
			PROCESS_SYSTEM_DPI_AWARE = 1,
			PROCESS_PER_MONITOR_DPI_AWARE = 2
		} PROCESS_DPI_AWARENESS;
		HRESULT(WINAPI *SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);
		SetProcessDpiAwareness = (HRESULT(WINAPI *)(PROCESS_DPI_AWARENESS)) 
			SDL_LoadFunction(shcoreDLL, "SetProcessDpiAwareness");
		if (SetProcessDpiAwareness) {
			SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
			return;
		}
	}

	//Try Vista - Windows 8 version.
	//This has a constant scale factor for all monitors.
	auto* userDLL = SDL_LoadObject("user32.dll");
	if (userDLL) {
		BOOL(WINAPI *SetProcessDPIAware)(void);
		SetProcessDPIAware = (BOOL(WINAPI *)(void))
			SDL_LoadFunction(userDLL, "SetProcessDPIAware");
		if (SetProcessDPIAware) {	
			SetProcessDPIAware();
			return;
		}
	}
}


HWND SDLWindows_getHWND(SDL_Window* sdlWindow) 
{
	return (HWND)SDL_GetPointerProperty(
		SDL_GetWindowProperties(sdlWindow),
		SDL_PROP_WINDOW_WIN32_HWND_POINTER, 
		nullptr);
};

typedef HRESULT(WINAPI* PFN_DwmSetWindowAttribute)(HWND hwnd,
	DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);

static PFN_DwmSetWindowAttribute DwmSetWindowAttributePtr = nullptr;

static void Loaddwmapi()
{
	static HMODULE dwmapiModule = nullptr;
	static bool IsLoaded = false;
	if (IsLoaded)return;
	IsLoaded = true;
	dwmapiModule = LoadLibraryA("dwmapi.dll");
	if (!dwmapiModule)return;
	
	DwmSetWindowAttributePtr = 
		reinterpret_cast<PFN_DwmSetWindowAttribute>(
			GetProcAddress(dwmapiModule, "DwmSetWindowAttribute"));
}

static HRESULT DwmSetWindowAttribute(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute)
{
	Loaddwmapi();
	if (!DwmSetWindowAttributePtr)return E_FAIL;
	return DwmSetWindowAttributePtr(hwnd, dwAttribute, pvAttribute, cbAttribute);
}

//https://stackoverflow.com/questions/57124243/winforms-dark-title-bar-on-windows-10


void set_dark_mode(SDL_Window* wnd, bool dark)
{
	
	auto* hWnd = SDLWindows_getHWND(wnd);
	

	//SetWindowTheme(hWnd, L"DarkMode_Explorer", NULL);

	const BOOL d = dark?1:0;
	if (S_OK != DwmSetWindowAttribute(hWnd, 20, &d, sizeof(d)))return;


	//haven't found another way to immediately redraw the TitleBar yet
	if (0 == (SDL_GetWindowFlags(wnd) & SDL_WINDOW_HIDDEN)) {
		
		//using SDL
		
		//int w, h;
		//SDL_GetWindowSize(wnd, &w, &h);
		//SDL_SetWindowSize(wnd, w + 1, h);
		//SDL_SetWindowSize(wnd, w, h);


		//using WinAPI

		RECT rc;
		if (GetWindowRect(hWnd, &rc))
		{
			let w = rc.right - rc.left;
			let h = rc.bottom - rc.top;
			let f = SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOREDRAW;
			SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, w + 1, h, f);
			SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, w, h, f);

		};
	}


	//the very first method
	//if (SDL_GetWindowFlags(wnd) & SDL_WINDOW_SHOWN) {
	//	SDL_MinimizeWindow(wnd);
	//	SDL_RestoreWindow(wnd);
	//}


	//RedrawWindow(hWnd, nullptr, nullptr, RDW_UPDATENOW| RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
	
	
	//SetWindowPos(hWnd,
	//	nullptr, 0, 0, 0, 0,
	//	SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
	//
	//InvalidateRect(hWnd,nullptr,TRUE);
	//
	//SendMessageW(hWnd, WM_NCPAINT,1, NULL);
	

	
}

void SDL_EnableWindow(SDL_Window* sdlWindow, bool enable)
{
	auto* h = SDLWindows_getHWND(sdlWindow);
	if (h)EnableWindow(h, enable?TRUE:FALSE);
};


////////////////////////////////////////////////////////////////////////////////
void set_thread_name(const char* name)
{
#ifndef NDEBUG
	const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.
		LPCSTR szName; // Pointer to name (in user addr space).
		DWORD dwThreadID; // Thread ID (-1=caller thread).
		DWORD dwFlags; // Reserved for future use, must be zero.
	} THREADNAME_INFO;
#pragma pack(pop)
	
	//auto info_ptr = thread.get_thread_info();
	//DWORD dwThreadID=info_ptr->id;
	const DWORD dwThreadID = (DWORD)-1;

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try{
		RaiseException(	MS_VC_EXCEPTION, 
						0, 
						sizeof(info) / sizeof(ULONG_PTR), 
						(ULONG_PTR*)&info);
	}__except (EXCEPTION_EXECUTE_HANDLER){
	}
#else 
	(void)name;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//prevents the computer from going to sleep completely
void ext_prevent_sleep_mode(bool prevent)
{
	EXECUTION_STATE f = ES_CONTINUOUS;

	if (prevent)f |= ES_SYSTEM_REQUIRED;// | ES_AWAYMODE_REQUIRED;
	
	SetThreadExecutionState(f);
}

////////////////////////////////////////////////////////////////////////////////

void setup_exception_handler() {};

#endif //_MSC_VER