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
#include "utf8.h"

#if defined( _MSC_VER)

#include <windows.h>

std::wstring utf8_to_native(std::string_view s)
{
	auto sz =
	MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
	std::wstring r;
	if (sz == 0)return r;
	r.resize((size_t)sz);
	MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), r.data(), sz);
	return r;
}
std::string native_to_utf8(std::wstring_view s)
{
	auto sz =
	WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0, nullptr, nullptr);
	std::string r;
	if (sz == 0)return r;
	r.resize((size_t)sz);
	WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), r.data(), sz, nullptr, nullptr);
	return r;
}

#endif//_MSC_VER