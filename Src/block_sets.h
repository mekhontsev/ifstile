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

struct build_data;

struct block_sets
{
	//returns true if it was just initialized
	bool init8(const build_data& bi);

	//returns ims_max on failure
	size_t change_set2(const build_data& bi, int step, bool cycle);


	//to trim huge lists
	std::vector<size_t> m_all_refs;

	//ID of the current block_info
	size_t m_cur_bid = 0;
};
