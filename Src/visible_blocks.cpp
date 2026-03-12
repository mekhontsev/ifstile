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
#include "visible_blocks.h"
#include "oper_block.h"
#include "data_column.h"
#include "columns.h"
#include "ifs_list.h"


void visible_blocks::reset_vis_blocks(const ifs_list& lst)
{
	let& ba = lst.m_blocks;

	let* cur =	m_cur_block_pos < m_vis_blocks.size() ? 
		m_vis_blocks[m_cur_block_pos] : nullptr;

	m_cur_block_pos = ims_max;

	m_vis_blocks.clear();

	m_vis_blocks.reserve(ba.size());

	for (let id : ba) {

		let* b = lst.get_block(id);

		if (!columns::get().accepted(b, b->m_calc_data.get(), true)){
			continue;
		}

		if (cur == b) {
			m_cur_block_pos = m_vis_blocks.size();
		}
		m_vis_blocks.emplace_back(b);

	}
}

size_t visible_blocks::find_vis_block(const oper_block* b)
{
	for (size_t i = 0; i < m_vis_blocks.size(); ++i) {
		if (m_vis_blocks[i] == b) {
			return i;
		}
	}

	return ims_max;
}



oper_block* visible_blocks::get_vis(size_t idx) const
{
	if (idx >= m_vis_blocks.size())return nullptr;
	return const_cast<oper_block*>(m_vis_blocks[idx]);
}




size_t visible_blocks::get_vis_checked()
{
	return m_checked;
}

oper_block* visible_blocks::get_cur_block()
{
	return get_vis(m_cur_block_pos);
}

size_t visible_blocks::find_block_by_id(block_id_t id)
{
	for (size_t i = 0;; ++i) {
		let* sr = get_vis(i);
		if (!sr)break;
		if (sr->m_block_id == id) {
			return i;
		}
	}
	return ims_max;
}

void visible_blocks::set_checked(oper_block* b, bool c)
{
	auto& f = b->m_flags;
	if (f.checked && !c) {
		--m_checked;
	} else if (!f.checked && c) {
		++m_checked;
	}

	f.checked = c;
}

void visible_blocks::check_interval(size_t i1, size_t i2, bool val, bool hidden)
{
	size_t from = std::min(i1, i2);
	size_t to = std::max(i1, i2);

	for (size_t i = from; i <= to; ++i) {
		auto* sr = get_vis(i);
		if (!sr)break;
		if (hidden) {
			sr->m_flags.hidden = val;
		} else {
			set_checked(sr, val);
		}
	}
}

void visible_blocks::init9(ifs_list& ba)
{
	reset_vis_blocks(ba);

	m_cur_block_pos = ims_max;

	update_num_checked(ba);

}

void visible_blocks::update_num_checked(const ifs_list& lst)
{
	uint32_t num = 0;
	for (let id : lst.m_blocks) {
		let* q = lst.get_block(id);
		if (q->m_flags.checked)++num;
	}
	m_checked.store(num);
}

void visible_blocks::remove_marked(ifs_list& lst)
{

	let num_block = lst.size();
	ims_erase(lst.m_blocks, [&](let id) 
	{
		let* q = lst.get_block(id);
		if (!q->m_flags.marked) return false;
		lst.release_block_entry(id);
		return true; 
	});

	if (num_block != lst.size()) {
		reset_vis_blocks(lst);
	}

	update_num_checked(lst);
}

void visible_blocks::list_action(const data_column* dc_arr, e_action& action)
{
	switch (action)
	{
	case e_action::Invert://visible only
	{
		for (size_t i = 0;; ++i) {
			auto* sr = get_vis(i);
			if (!sr)break;
			set_checked(sr, !sr->m_flags.checked);
		};
		break;
	}
	case e_action::Unique:
	{
		let& col = dc_arr[columns::get().m_last_sorted_idx];

		const oper_block* prev = nullptr;
		for (size_t idx = 0;; ++idx) {
			auto* cur = get_vis(idx);
			if (!cur)break;

			if (!prev || !col.is_same(prev, cur)) {
				set_checked(cur, true);
			}
			prev = cur;
		}
		break;
	}
	case e_action::Hide:
	{
		for (size_t idx = 0;; ++idx) {
			auto* cur = get_vis(idx);
			if (!cur)break;
			if (cur->m_flags.checked) {
				cur->m_flags.hidden = true;
			}
		}
		break;
	}
	case e_action::Unhide:
	{
		for (size_t idx = 0;; ++idx) {
			auto* cur = get_vis(idx);
			if (!cur)break;
			if (cur->m_flags.checked) {
				cur->m_flags.hidden = false;
			}
		}
		break;
	}
	default:
		assert(false);
	}
}

bool visible_blocks::append_block(oper_block* sr)
{
	if (sr->m_flags.checked) {
		++m_checked;
	}

	if (!columns::get().accepted(sr, sr->m_calc_data.get(), true)){
		return false;//hidden
	}

	m_vis_blocks.emplace_back(sr);

	return true;
}


