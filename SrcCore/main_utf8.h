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

#if defined( _MSC_VER)
#include <vector>
#include <string>
#include "utf8.h"
#include <windows.h>
#include <shellapi.h>
#endif

int main(int argc, char** argv) 
{

#if defined( _MSC_VER)
	
	//Set console code page to UTF8 so console known how to interpret string data
	SetConsoleOutputCP(CP_UTF8);
	//Enable buffering to prevent VS from chopping up UTF8 byte sequences
	setvbuf(stdout, nullptr, _IOFBF, 1000);

	//https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/setlocale-wsetlocale?view=msvc-160#utf-8-support
	//For Windows<1803 to work, static linking of the runtime is required
	//en_US makes the decimal separator a period in print functions
	setlocale(LC_ALL, "en_US.UTF8");

	////////////////////////////////////////////////////////////////////////////

	std::vector<std::string> u8_argv;
	std::vector<char*> u8_ptrs;

	auto** wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if(wargv){
		u8_argv.resize(argc);
		for(int i = 0; i < argc; ++i){
			u8_argv[i] = native_to_utf8(wargv[i]);
		}
		LocalFree(wargv);
	}
	
	u8_ptrs.resize(u8_argv.size());
	for(size_t i = 0; i < u8_argv.size(); ++i){
		u8_ptrs[i] = u8_argv[i].data();//null-terminated
	}

	argv = u8_ptrs.data();

#endif

	int main_utf8(int argc, char** argv);//must be defined somewhere
	return main_utf8(argc, argv);
	
}