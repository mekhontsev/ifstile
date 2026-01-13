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
#include "block_class.h"
#include "ims_info.h"
#include "ims_operator.h"

IMS_DEFINE_OPAQUE_DELETER(block_class);

std::string_view block_class::get_var_name(size_t ref) const
{
	if (ref_is_builtin(ref)) {
		return get_builtin_name(ref2builtin(ref));
	}

	let unk_idx = m_refs[ref].unk_id;
	if (unk_idx == ims_max) {
		return {};
	}
	return m_nfo->m_list.m_idf.get_str_from_unk(unk_idx);
}

size_t block_class::find_var_by_name(std::string_view name) const
{
	let* d = m_nfo->m_list.m_idf.find_data(name);
	if (!d)return ims_max;
	return find_var(d->unk_id);
}

size_t block_class::find_var(size_t unk_id) const
{
	assert(unk_id != ims_max);
	auto it = m_unk2var.find(unk_id);
	return it == m_unk2var.end() ? ims_max : it->second;
}

size_t block_class::add_var(size_t unk_id)
{
	let ret = m_refs.size();

	auto& oi = m_refs.emplace_back();
	
	oi.unk_id = unk_id;

	if (unk_id != ims_max) {
		let res = m_unk2var.emplace(unk_id, ret);
		assert(res.second);
#ifndef NDEBUG
		oi.name2 = get_var_name(ret);
#endif	
	}

	return ret;
}
