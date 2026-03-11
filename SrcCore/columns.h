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
#include "column_id.h"

struct oper_block;
struct search_info;
struct ifs_list;

struct rule
{
	bool filter = false;

	//what booleans are allowed?
	bool bval = false;

	//what integers are allowed
	int64_t ilim[2] = { 0,0 };

	//what real values are allowed?
	double lim[2] = { 0,0 };

	using arr = std::array<rule, column_id::NUM_COLS>;

};

enum ERULE :size_t
{
	view = 0,
	search_first = 1,
};


struct columns
{
	void init_columns(
		size_t num_rules = ERULE::search_first + 1, 
		bool clear = false);

	void adjust_vis();
	
	bool accepted(const oper_block* b, const search_info* u, 
		bool for_view)  const;

	void sort_by_col(size_t idx, ifs_list& lst, bool order);

	void sort_cols_by_vis(std::vector<size_t>& vcol);
	//if it's already sorted, it does nothing.
	void sort_cols(std::vector<size_t>& vcol, size_t ridx);

	bool only_osc() const;

	static columns& get();

	size_t get_complexity() const;
	size_t get_max_search_depth() const;
	//optimization - early checking of connectedness
	bool is_connectedness_ok(int64_t con) const;
	//optimization - early checking of intersection dimensions
	bool is_bdim_ok(double d) const;

	void resize_rules(size_t num);

public:

	size_t m_last_sorted_idx = 0;


	bool used(size_t column) const;

	void init_search_rules(size_t from);
	std::array<bool, column_id::NUM_COLS> m_col_visible;

	std::vector<rule::arr> m_rule;

	ankerl::unordered_dense::map<std::string, column_id::ECID> m_str2id;

private:

	bool is_visible_c(const oper_block* b, const search_info* u, size_t ridx,
		size_t column) const;
};
