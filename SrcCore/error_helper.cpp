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
#include "error_helper.h"


using src_location_key = std::pair<size_t, size_t>;

struct pair_hash {
	std::size_t operator () (const std::pair<size_t, size_t>& p) const {
		return p.first ^ p.second;
	}
};

ims_static ankerl::unordered_dense::set<src_location_key, pair_hash> g_err_exists;

void ims_err_reset()
{
	g_err_exists.clear();
}

////////////////////////////////////////////////////////////////////////////////

struct error_location
{
	//block variable with ordinal number idx
	std::string_view var_name;
	//current line of file if block is not specified
	size_t line_in_file = 0;

	bool m_print_to_buf = false;
	std::string m_buf;
};

ims_static thread_local error_location g_location;


std::string error_helper::get_buf()
{
	auto ret = std::move(g_location.m_buf);
	g_location.m_buf.clear();
	return ret;
};

void error_helper::set_to_buf(bool r)
{
	g_location.m_buf.clear();//in any case
	g_location.m_print_to_buf = r;
};

void error_helper::set_var(std::string_view name)
{
	g_location.var_name = name;
};

void error_helper::set_line(size_t line)
{
	g_location.line_in_file = line;
}


////////////////////////////////////////////////////////////////////////////////

void ims_err_print(
	bool is_warning,
	size_t file,
	size_t line,
	fmt::string_view fmt,
	fmt::format_args args)
{
	auto res = g_err_exists.emplace(file, line);
	if (!res.second) {//already printed
		return;
	}

	auto& g = g_location;

	std::string loc;

	if (g.line_in_file > 0) {
		fmt::format_to(std::back_inserter(loc),
			"Block at line: {}\n", g.line_in_file);
	}

	if (!g.var_name.empty()) {
		fmt::format_to(std::back_inserter(loc),
			"Var: {}\n", g.var_name);
	}

	if (!g.m_print_to_buf || is_warning) {
		std::cout << loc;
		fmt::vprint(std::cout, fmt, args);
		std::cout << std::endl;
	} else {
		g.m_buf += loc;
		fmt::vformat_to_n(std::back_inserter(g.m_buf), ims_max, fmt, args);
	}
};

