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

//platform-dependent code
namespace platform
{

	//path where we will store the settings
	std::string_view getPathPref();

	//path to the user folder
	std::string_view getPathHome();


	//simple message
	void message(const char* msg, const char* title=nullptr);


	bool is_folder_exists(const std::string& path);
	bool create_directory(const char* fn);
	bool remove_empty_directory(const char* fn);
	bool remove_file(const char* fn);


	struct window_disabler
	{
		window_disabler();
		~window_disabler();
	};


	void open_url(const std::string& url);
	void ims_to_clipboard(const std::string& str);


	constexpr bool isDesktop()
	{
#if defined(_MSC_VER)
        return true;
#elif defined(__ANDROID__)
		return false;
#elif defined(__APPLE__)
#if TARGET_OS_IPHONE
        return false;
#else
        return true;
#endif //TARGET_OS_IPHONE
#else 
		return true;
#endif //_MSC_VER
	}

}
