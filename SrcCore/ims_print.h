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

template <typename... T>
void ims_print(fmt::format_string<T...> fmt, T&&... args) 
{
	fmt::vargs<T...> vargs = { {args...} };
	fmt::vprint(std::cout, fmt.str, vargs);
}

void ims_err_print(
	bool is_warning,
	size_t file, 
	size_t line, 
	fmt::string_view str, 
	fmt::format_args args);


template <typename... T>
void ims_err_print_ex(
	fmt::format_string<T...> fmt, 
	bool is_warning,
	size_t file,
	size_t line, 
	T&&... args)
{
	fmt::vargs<T...> vargs = { {args...} };
	ims_err_print(is_warning, file, line, fmt.str, vargs);
}

//restart showing all errors
void ims_err_reset();

consteval size_t ims_str_hash(const char* str) 
{	
	size_t h = 0;

	if constexpr (sizeof(size_t) == 8) {
		h = 1125899906842597L; // prime
	} else {
		h = 4294967291L;
	}

	int i = 0;
	while (str[i] != 0) {
		h = 31 * h + str[i++];
	}

	return h;
}


//print an error if it hasn't already printed
#define ims_error(format, ...) ims_err_print_ex (format, false, ims_str_hash(__FILE__), __LINE__, ##__VA_ARGS__)
#define ims_warning(format, ...) ims_err_print_ex (format, true, ims_str_hash(__FILE__), __LINE__, ##__VA_ARGS__)
