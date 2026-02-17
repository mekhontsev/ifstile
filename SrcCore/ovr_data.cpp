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
#include "ovr_data.h"
#include "oper_block.h"

void ovr_data::init(const oper_block& b, bool without_own_ctx /*= false*/)
{
	let sz = b.num_vars();

	m_arr.resize(sz);
	for (size_t i = 0; i < sz; ++i) {
		m_arr[i].p.h.set_xundef();
	};

	for (auto& q : m_builtins)q.h.clear();

	for (m_p = &b; m_p; m_p = m_p->get_parent()) {
		if (without_own_ctx && m_p->own_ctx()) {
			return;
		}
		for (let& q : *m_p) {
			let ptr = m_p->get_ptr(q.pos5);

			if (q.is_builtin()) {
				auto& x = m_builtins[(size_t)q.get_builtin()];
				if (!x.is_def()) {
					x = ptr;
				}
			} else {
				auto& d = m_arr[q.gr()];
				if (d.p.h.is_xundef()) {
					d.is_subs = q.is_subs();
					d.p = ptr;
				}
			}
		}
	}
}

void ovr_data::merge_from(oper_block& dst, const oper_block& src) const
{
	let& opod = *this;

	dst.clear();

	dst.m_name = src.m_name;
	dst.m_dim2 = src.get_dim();
	dst.m_subspace = src.m_subspace;
	dst.m_js_init = src.m_js_init;

	auto& f = dst.m_flags;
	f = src.m_flags;

	f.clear_attr();


	let sz = src.num_vars();


	auto& ovr = opod.m_arr;

	dst.clear_ops();
	uint32_t pos = 0;

	for (size_t i = 0; i < sz; ++i) {
		let& q = ovr[i];
		if (q.p.h.is_xundef())continue;

		let ds = dst.add_var(pos, i, q.is_subs);//not a substitution
		dst.insert_op_ex(ds, ast_context{ q.p,0 }, *src.ctx(), nullptr, false);
	}

	for (size_t i = 0; i < c_num_builtins; ++i) {
		ast_context q{ opod.m_builtins[i],0 };
		if (!q.is_def()) continue;

		let ds = dst.add_builtin(builtin_ids(i));
		dst.insert_op_ex(ds, q, *src.ctx(), nullptr, false);
	}

	dst.set_parent(opod.m_p);

	if (!dst.get_class()) {
		dst.create_copy(src.get_class());
	}

	dst.m_flags.ready = false;

	dst.set_own_dim();
}

