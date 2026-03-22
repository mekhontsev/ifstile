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
#include "ims_file.h"
#include "utf8.h"

#if defined( _MSC_VER)

#include <cstdio>//_wfopen_s

extern "C" {
FILE* fopen_utf8(const char* filename, const char* mode)
{
	FILE* ret = nullptr;
	let err = _wfopen_s(&ret,
		utf8_to_native(filename).c_str(),
		utf8_to_native(mode).c_str());
	return err ? nullptr : ret;
};
}

#endif


namespace ims_file {

FILE* open_exclusive_write(const std::string& filename)
{
	//'x' - if the file already exists, fopen() fails
	static const char* mode = "wxb";
#if defined( _MSC_VER)
	return fopen_utf8(filename.c_str(), mode);
#else 
	return fopen(filename.c_str(), mode);
#endif
}

template<typename Stream>
void open_t(Stream& fs,
	const std::string& filename,
	std::ios_base::openmode mode)
{
#ifdef _MSC_VER
	fs.open(utf8_to_native(filename), mode);
#else
	fs.open(filename, mode);
#endif//_MSC_VER
}

void open(std::ofstream& fs,
	const std::string& filename,
	std::ios_base::openmode mode)
{
	return open_t(fs, filename, mode);
}

void open(std::ifstream& fs,
	const std::string& filename)
{
	return open_t(fs, filename, std::ios_base::binary);
}


bool is_exists(const std::string& filename)
{
	std::ifstream fs;
	open(fs, filename);
	return fs.good();
};

bool get_contents(const std::string& filename, std::string& contents)
{
	std::ifstream fs;
	open(fs, filename);
	if (!fs.is_open())return false;
	using fi = std::istreambuf_iterator<char>;
	contents.assign(fi(fs), fi());
	return true;
}

std::string get_extension(std::string_view path)
{
	for (size_t i = path.length(); i > 0; --i) {
		let c = path[i - 1];
		if (c == '\\' || c == '/')break;

		if (c == '.') {
			return
				boost::algorithm::to_lower_copy(std::string(path.substr(i)));
		}
	}
	return {};
};

std::string_view remove_extension(std::string_view path)
{
	for (size_t i = path.length(); i > 0; --i) {
		let c = path[i - 1];
		if (c == '\\' || c == '/')break;

		if (c == '.') {
			return path.substr(0, i - 1);
		}
	}
	return path;
};



std::string get_parent(const std::string& fn, std::string* name)
{
	if (name)(*name) = "";

	auto ret = adjust(fn);

	if (is_root(ret)) {	
		return "";
	}
	if (ret.back() == '/') {
		ret.pop_back();
	}
	let pos = ret.rfind('/');
	if (pos == std::string::npos) {
		//everything was processed, it was just a file name
		if (name)(*name) = ret;
		return "";
	}

	if (name)(*name) = ret.substr(pos + 1);

	return ret.substr(0, pos + 1);
};

bool is_root(const std::string& fn)
{
	return fn.empty() || fn == "/";
}

std::string adjust(std::string_view path)
{
	std::vector<std::string> arr;
	boost::split(arr, path, boost::is_any_of("/\\"));

	for (size_t i = 1; i < arr.size();) {
		if (i>0 && arr[i - 1] != ".." && arr[i] == "..") {
			arr.erase(arr.begin() + i - 1, arr.begin() + i + 1);
			--i;
		} else {
			++i;
		}
	}

	std::string ret;
	for (size_t i = 0; i < arr.size(); ++i) {
		ret += arr[i];
		if (i + 1 < arr.size())ret += "/";
	}

	return ret;
};


}
