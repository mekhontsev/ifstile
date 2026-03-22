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

struct oper_block;
struct ifs_list;

enum class e_action : int
{
	Uncheck = 0,//remove the flag from ALL
	Remove,
	Invert,//invert only visible
	Interval,
	Unique,
	Hide,
	Unhide,
	Convert,
};


struct data_column;

struct visible_blocks
{
	
	//the current block's serial number among the visible ones. This is not an ID
	size_t m_cur_block_pos = ims_max;

	std::vector<const oper_block*> m_vis_blocks;

	//number of marked
	std::atomic<uint32_t> m_checked{};

	void reset_vis_blocks(const ifs_list& lst);
	size_t find_vis_block(const oper_block* b);
	
	oper_block* get_vis(size_t idx) const;

	size_t get_vis_checked();

	size_t find_block_by_id(block_id_t id);

	void set_checked(oper_block* b, bool c);

	void check_interval(size_t i1, size_t i2, bool val, bool hidden);

	void init9(ifs_list& ba);

	void update_num_checked(const ifs_list& ba);

	void remove_marked(ifs_list& ba);

	void list_action(const data_column* dc_arr, e_action& action);

	bool append_block(oper_block* sr);
};
