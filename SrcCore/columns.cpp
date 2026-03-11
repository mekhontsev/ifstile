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
#include "columns.h"
#include "data_column.h"
#include "oper_block.h"
#include "ifs_list.h"
#include "ims_num_traits.h"

template<typename F>
void sort_cols_by_func(F* f, bool asc, bool nsearch, ifs_list& lst)
{
	auto& arr = lst.m_blocks;
	if (asc) {
		std::stable_sort(arr.begin(), arr.end(),
			[f, nsearch, &lst](let id1, let id2)
			{
				let* e1 = lst.get_block(id1);
				let* e2 = lst.get_block(id2);
				if (nsearch) {
					if (!e1->m_calc_data)return false;
					if (!e2->m_calc_data)return true;
					//there are both, let's continue the usual comparison
				}
				return f(*e1, *e1->m_calc_data) < f(*e2, *e2->m_calc_data);
			});
	} else {
		std::stable_sort(arr.begin(), arr.end(),
			[f, nsearch, &lst](let id1, let id2)
			{
				let* e1 = lst.get_block(id1);
				let* e2 = lst.get_block(id2);
				if (nsearch) {
					if (!e1->m_calc_data)return false;
					if (!e2->m_calc_data)return true;
					//there are both, let's continue the usual comparison
				}
				return f(*e1, *e1->m_calc_data) > f(*e2, *e2->m_calc_data);
			});
	};
};

template<typename F>
void sort_cols_by_string(F* f, bool asc, bool nsearch, ifs_list& lst)
{
	auto& arr = lst.m_blocks;
	std::string s1, s2;
	if (asc) {
		std::stable_sort(arr.begin(), arr.end(),
			[f, nsearch, &s1, &s2, &lst](let id1, let id2)
			{
				let* e1 = lst.get_block(id1);
				let* e2 = lst.get_block(id2);

				if (nsearch) {
					if (!e1->m_calc_data)return false;
					if (!e2->m_calc_data)return true;
					//there are both, let's continue the usual comparison
				}
				s1.clear();
				s2.clear();
				f(*e1, *e1->m_calc_data, std::back_inserter(s1));
				f(*e2, *e2->m_calc_data, std::back_inserter(s2));
				return s1 < s2;
			});
	} else {
		std::stable_sort(arr.begin(), arr.end(),
			[f, nsearch, &s1, &s2, &lst](let id1, let id2)
			{
				let* e1 = lst.get_block(id1);
				let* e2 = lst.get_block(id2);

				if (nsearch) {
					if (!e1->m_calc_data)return false;
					if (!e2->m_calc_data)return true;
					//there are both, let's continue the usual comparison
				}

				s1.clear();
				s2.clear();
				f(*e1, *e1->m_calc_data, std::back_inserter(s1));
				f(*e2, *e2->m_calc_data, std::back_inserter(s2));
				return s1 > s2;
			});
	};
};




bool columns::is_visible_c(
	const oper_block* b, 
	const search_info* u, 
	size_t ridx, 
	size_t column)  const
{
	let& c = data_column::g_cols[column];
	
	let& qi = m_rule[ridx][column];
	if (!qi.filter)return true;


	if (c.is_need_search() && !u)return true;//cannot hide if there is no information

	if (column == column_id::ECID::GCX && ridx > ERULE::view) {
		return true;//checked in advance using a complex formula
	}

	if (column == column_id::ECID::CH || column == column_id::ECID::HD) {
		let req_val = qi.bval ? 1 : 0;
		let val = c.get_int(*b, *u);
		return val == req_val;
	} else if (c.get_float) {
		let v = c.get_float(*b, *u);
		let eps = ims_num_traits<double>::almost_zero();
		return v >= qi.lim[0] - eps && v <= qi.lim[1] + eps;
	} else if (c.get_int) {
		let v = c.get_int(*b, *u);
		return v >= qi.ilim[0] && v <= qi.ilim[1];
	}
	return true;
}

bool columns::accepted(const oper_block* b, const search_info* u, bool for_view)  const
{
	size_t idx0, idx1;
	if (for_view) {
		idx0 = ERULE::view;
		idx1 = ERULE::search_first;
	} else {
		idx0 = ERULE::search_first;
		idx1 = m_rule.size();
	}

	for (size_t r = idx0; r < idx1; ++r) {
		bool rule_accepted = true;

		for (size_t v = 0; v < column_id::NUM_COLS; ++v) {
			if (!is_visible_c(b, u, r, v)) {
				rule_accepted = false;
				break;
			}
		}

		if (rule_accepted)return true;//met all the criteria of the rule
		
	}
	return false;//didn't match any of the rules
}

void columns::sort_by_col(
	size_t idx, 
	ifs_list& lst, 
	bool order)
{
	m_last_sorted_idx = idx;

	let& c = data_column::g_cols[idx];

	let nsearch = c.is_need_search();

	if (c.get_int) {
		sort_cols_by_func(c.get_int, order, nsearch, lst);
	}  else if (c.get_float) {
		auto& arr = lst.m_blocks;

		std::stable_sort(arr.begin(), arr.end(),
			[&c, order, nsearch, &lst](let id1, let id2)
			{
				let* e1 = lst.get_block(id1);
				let* e2 = lst.get_block(id2);

				if (nsearch) {
					if (!e1->m_calc_data)return false;
					if (!e2->m_calc_data)return true;
					//there are both, let's continue the usual comparison
				}

				let d1 = c.get_float(*e1, *e1->m_calc_data);
				let d2 = c.get_float(*e2, *e2->m_calc_data);

				if (std::abs(d1 - d2) <
					ims_num_traits<data_column::Real>::almost_zero()) {
					return false;//the same
				}

				if (order) {
					return d1 < d2;
				} else {
					return d1 > d2;
				}
			});
	} else if (c.to_string) {
		sort_cols_by_string(c.to_string, order, nsearch, lst);
	}
}


void columns::sort_cols_by_vis(std::vector<size_t>& vcol)
{
	auto check = [this](size_t i1, size_t i2) ->bool {

		let fl1 = m_col_visible[i1];
		let fl2 = m_col_visible[i2];
		if (fl1 == fl2)return i1 < i2;
		return fl1 && !fl2;
	};

	/////////////////////////////////////////////////////////////////////
	bool ordered = true;
	for (size_t i = 1; i < vcol.size(); ++i) {
		if (!check(i - 1, i)) {
			ordered = false;
			break;
		}
	}
	if (ordered)return;

	/////////////////////////////////////////////////////////////////////
	std::stable_sort(vcol.begin(), vcol.end(), check);
}

void columns::sort_cols(std::vector<size_t>& vcol, size_t ridx)
{
	auto check = [this, ridx](size_t i1, size_t i2) ->bool {

		let& ra = m_rule[ridx];
		let fl1 = ra[i1].filter;
		let fl2 = ra[i2].filter;
		return fl1 && !fl2;
	};

	/////////////////////////////////////////////////////////////////////
	bool ordered = true;
	for (size_t i = 1; i < vcol.size(); ++i) {
		if (!check(i - 1, i)) {
			ordered = false;
			break;
		}
	}
	if (ordered)return;

	/////////////////////////////////////////////////////////////////////
	std::stable_sort(vcol.begin(), vcol.end(), check);
}

bool columns::only_osc() const
{
	for (size_t i = ERULE::search_first; i < m_rule.size(); ++i) {
		if (m_rule[i][column_id::OVL].filter) {
			return false;
		}
	}
	return true;
}


columns& columns::get()
{
	ims_func_static columns g_col;
	if (g_col.m_str2id.empty()) {
		for (size_t i = 0; i < column_id::NUM_COLS; ++i) {
			let& src = data_column::g_cols[i];
			g_col.m_str2id[src.title] = column_id::ECID(i);
		}
	}
	return g_col;
}


bool columns::used(size_t column) const
{
	if (m_col_visible[column])return true;

	for (let& ra : m_rule) {
		if (ra[column].filter)return true;
	}

	return false;
}

void columns::init_search_rules(size_t from)
{
	for (size_t i = from; i < m_rule.size(); ++i) {
		auto& rs = m_rule[i];

		for (size_t c = 0; c < column_id::NUM_COLS; ++c) {
			let& src = data_column::g_cols[c];

			auto& r = rs[c];
			r.filter = false;
			r.lim[0] = src.limit.rdef[0];
			r.lim[1] = src.limit.rdef[1];
			r.ilim[0] = src.limit.idef[0];
			r.ilim[1] = src.limit.idef[1];
		}

		
	}
};

void columns::resize_rules(size_t num)
{
	num = ims_clamp(num, 2, 16);
	size_t old_size = m_rule.size();
	m_rule.resize(num);
	if (num <= old_size) {
		return;
	}

	init_search_rules(old_size);
}


void columns::adjust_vis()
{
	for (size_t i = 0; i < column_id::NUM_COLS; ++i) {
		if (m_col_visible[i] && !data_column::g_cols[i].is_need_search()) {
			return;//everything is fine, at least one visible column
		}
	}
	for (size_t i = 0; i < column_id::NUM_COLS; ++i) {
		let& src = data_column::g_cols[i];
		if (!src.is_need_search() && src.is_def_vis()) {
			m_col_visible[i] = true;
		}
	}
};


void columns::init_columns(size_t num_rules, bool clear)
{
	assert(num_rules >= 2);
	m_rule.resize(num_rules);

	auto& rv = m_rule[ERULE::view];

	for (size_t i = 0; i < column_id::NUM_COLS; ++i) {

		let& src = data_column::g_cols[i];
		m_col_visible[i] = src.is_def_vis();
		
		auto& r= rv[i];
		r.filter = false;
		r.lim[0] = src.limit.rdef[0];
		r.lim[1] = src.limit.rdef[1];
		r.ilim[0] = src.limit.idef[0];
		r.ilim[1] = src.limit.idef[1];
	}

	if (!clear) {
		auto& r = rv[column_id::HD];
		r.filter = true;
		r.bval = false;
	}

	//////////////////////////////////////////////////

	init_search_rules(ERULE::search_first);

	if (!clear) {
		auto& r = m_rule[ERULE::search_first];
		r[column_id::GCX].filter = true;
		r[column_id::DIM2].filter = true;
		r[column_id::CT].filter = true;
	}
}

size_t columns::get_complexity() const
{
	int64_t ret = 0;

	for (size_t i = ERULE::search_first; i < m_rule.size(); ++i) {
		let& c = m_rule[i][column_id::GCX];
		//limit even if filtering is disabled
		if (i > ERULE::search_first && !c.filter)continue;
		ret = std::max(ret, c.ilim[1]);
	}

	return static_cast<size_t>(ret);
}

size_t columns::get_max_search_depth() const
{
	size_t ret = 0;

	for (size_t i = ERULE::search_first; i < m_rule.size(); ++i) {
		let& c = m_rule[i][column_id::DPT];
		if (!c.filter)return ims_max;
		ret = std::max(ret, static_cast<size_t>(c.ilim[1]));
	}
	return ret;
}

bool columns::is_connectedness_ok(int64_t con) const
{
	for (size_t i = ERULE::search_first; i < m_rule.size(); ++i) {
		let& c = m_rule[i][column_id::CT];
		if (!c.filter)return true;
		if (con >= c.ilim[0] && con <= c.ilim[1]) return true;
	}
	return false;//didn't match any filters
}

bool columns::is_bdim_ok(double d) const
{
	let& eps = ims_num_traits<double>::almost_zero();
	for (size_t i = ERULE::search_first; i < m_rule.size(); ++i) {
		let& c = m_rule[i][column_id::DIM2];
		if (!c.filter)return true;
		if (d + eps > c.lim[0] && d - eps < c.lim[1]) return true;
	}
	return false;//didn't match any filters
}


