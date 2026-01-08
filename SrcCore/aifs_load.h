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

struct ims_info;
struct oper_block;
struct ifs_list;
struct read_state;

bool aifs_from_stream(
	ifs_list& lst,
	size_t& cur_line,
	read_state& rs,
	std::istreambuf_iterator<char>& it_beg,
	const std::istreambuf_iterator<char>& it_end);

//load from a fragment of an already open file
[[nodiscard]]
bool ims_load7(
	ims_info& nfo,
	std::string_view path,
	std::istreambuf_iterator<char>& nbeg,
	const std::istreambuf_iterator<char>& nend,
	bool bappend);

//create a copy of all blocks, replacing the definition of the given b
//if b == 0, then add a new block to the end
bool ims_apply_source(
	const ims_info& nfo,
	std::unique_ptr<ims_info>& dst,
	const oper_block** new_b,//who did 'b' turn into in the new file
	std::string_view src,
	const oper_block* b,
	bool js_mode);


