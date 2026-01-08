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


struct combinator
{
	void init(size_t n, size_t k);
	void get();

	//returns false if there are no more combinations
	bool next();

	std::vector<size_t> idxs;

private:

	std::vector<bool> mask;
};

struct combinator1
{
	bool next();

	struct elem
	{
		size_t v, b, e;// b <= v < e
	};

	std::vector<elem> m_state;
};



struct combinator2
{
	bool next();
	bool init(size_t num);

	struct elem
	{
		size_t max_cur; //maximum available in this group
		size_t max_other; //how many can be distributed among subsequent groups

		size_t v; //how many were placed in the current group
		size_t num; //how many MUST be placed starting from this group
	};

	std::vector<elem> m_state;
};


//generates a non-decreasing sequence of indices for each group
struct combinator3 
{
	void init_group(size_t idx);
	bool next_group(size_t idx);
	void init();
	//go to the next state starting changes from the idx group
	bool next_from(size_t idx);

	struct elem
	{
		size_t idx;		//start in m_arr
		size_t num;		//how many "digits" are in this group (> 0)
		size_t max;		//number system >0
	};

	std::vector<elem> m_settings;
	std::vector<size_t> m_state;

};