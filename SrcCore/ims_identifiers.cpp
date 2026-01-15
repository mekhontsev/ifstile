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
#include "ims_identifiers.h"

ims_identifiers::data& ims_identifiers::get_data(std::string_view v)
{
	let new_id = m_idx2unknown.size();
	auto it = m_unknown2idx.emplace(v, data{ new_id, block_id_max });
	auto* ret = &*it.first;
	if (it.second) {
		m_idx2unknown.emplace_back(ret);
	}
	assert(m_unknown2idx.size() == m_idx2unknown.size());
	return ret->second;
}

const ims_identifiers::data* 
ims_identifiers::find_data(std::string_view str_id) const
{
	auto it = m_unknown2idx.find(str_id);
	if (it == m_unknown2idx.end()) {
		return nullptr;
	}
	return &it->second;
}

block_id_t ims_identifiers::find_block_id(std::string_view str_id) const
{
	let* d = find_data(str_id);
	return d ? d->block_id: block_id_max;
}

std::string
ims_identifiers::gen_unique_block_id(std::string_view prefix, size_t* suffix) const
{
	size_t sidx = 0;//not static!
	if (!suffix)suffix = &sidx;

	std::string ret(prefix);
	for (;;) {
		ret += std::to_string(*suffix);
		let* d = find_data(ret);
		if (!d || !d->has_block()) {
			return ret;
		}
		++(*suffix);
		ret.resize(prefix.size());//cut back at each iteration
	}
}

size_t ims_identifiers::create_unique_identifier(std::string_view prefix)
{
	std::string ret(prefix);
	size_t num = 0;
	for (;;) {
		if (!find_data(ret)) break;
		ret.resize(prefix.size());//cut back at each iteration
		ret += std::to_string(num++);
	}

	return get_data(ret).unk_id;
}
