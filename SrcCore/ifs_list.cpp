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
#include "ifs_list.h"
#include "oper_block.h"


void ifs_list::data::clear()
{
	b.reset();
	m_str_id = ims_max;
}


block_id_t ifs_list::alloc_block_entry()
{
	block_id_t ret;

	if (!m_free_blocks.empty()) {
		ret = m_free_blocks.back();
		m_free_blocks.pop_back();
	} else {
		ret = (block_id_t)m_id2data.size();
		m_id2data.emplace_back();
	}

	return ret;
}

void ifs_list::release_block_entry(block_id_t id)
{
	auto& d = m_id2data[id];
	if (d.m_str_id != ims_max) {
		m_idf.m_idx2unknown[d.m_str_id]->second.block_id = block_id_max;
	}
	d.clear();
	if (m_id2data.size() == id + 1) {//minor optimization
		m_id2data.pop_back();
	} else {
		m_free_blocks.emplace_back(id);
	}
}

block_id_t ifs_list::insert_by_str_id(std::string_view str_id)
{
	assert(!str_id.empty());
	auto& d = m_idf.get_data(str_id);

	if (!d.has_block()) {
		let id = alloc_block_entry();
		d.block_id = id;
		m_id2data[id].m_str_id = d.unk_id;
	}
	return d.block_id;
}


block_id_t ifs_list::move_block(
	std::unique_ptr<oper_block>& src, std::string_view str_id)
{
	block_id_t id;
	if (!str_id.empty()) {
		id = insert_by_str_id(str_id);
	} else {
		id = alloc_block_entry();
	}
	src->m_block_id = id;
	auto& v = m_id2data[id];
	v.b = std::move(src);

	m_blocks.emplace_back(id);

	return id;
}

oper_block* ifs_list::get_block(block_id_t id) const
{
	assert(id != block_id_max);//let them sort it out above
	return m_id2data[id].b.get();
}

oper_block* ifs_list::get_block_from_unk(size_t unk_id) const
{
	let* v = m_idf.m_idx2unknown[unk_id];
	let block_id = v->second.block_id;
	if (block_id == block_id_max) {
		return nullptr;
	}
	return get_block(block_id);
}

std::string_view ifs_list::get_str(block_id_t id) const
{
	assert(id != block_id_max);//let them sort it out above
	let str_id = m_id2data[id].m_str_id;
	if (str_id == ims_max) {
		return {};
	}
	return m_idf.get_str_from_unk(str_id);
}

oper_block* ifs_list::get_block_by_idx(size_t idx) const
{
	return m_id2data[m_blocks[idx]].b.get();
}

oper_block* ifs_list::add_block(std::string_view str_id)
{
	auto ptr = std::make_unique<oper_block>();
	auto* ret = ptr.get();
	move_block(ptr, str_id);
	return ret;
}

oper_block* ifs_list::find_block(std::string_view str_id) const
{
	let id = m_idf.find_block_id(str_id);
	if (id == block_id_max) {
		return nullptr;
	}
	return get_block(id);
}

oper_block* ifs_list::find_block2(std::string_view str_id, std::string_view name) const
{
	//TODO: may be this function should be in a different class?
	if (!str_id.empty()) {
		auto* b = find_block(std::string(str_id));
		if (b)return b;
	}

	//O(n)
	if (!name.empty()) {
		for (let id : m_blocks) {//loop through live blocks
			auto* b = get_block(id);
			if (b->m_name == name) {
				return b;
			}
		}
	}
	return nullptr;
}


