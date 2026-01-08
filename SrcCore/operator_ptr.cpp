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

#include "operator_ptr.h"
#include "oper_block.h"


ast_context ast_context::index(size_t idx) const
{
	return ast_context{ index_base(idx), call_offset };
}

operator_ptr operator_ptr::index_base(size_t idx) const
{
	return  a->get_ptr(h.u32 + idx);
}

template<typename T>
bool cmp_int(T v1, T v2, intptr_t& res)
{
	res = ((intptr_t)v1) - ((intptr_t)v2);
	return res != 0;
}

intptr_t ast_context::lexic_compare(ast_context t1, ast_context t2)
{
	intptr_t v;

	v = ims_operator::lexic_compare(t1.h, t2.h);
	if (v != 0)return v;

	if (t1.h.tt == ETYPE::reference) {
		if (cmp_int(t1.call_offset, t2.call_offset, v))	return v;
	
	}

	let n1d = t1.h.data_args();
	let n2d = t2.h.data_args();

	let n1o = t1.h.oper_args();
	let n2o = t2.h.oper_args();

	//there must be the same number of arguments

	if (cmp_int(n1d, n2d, v)) return v;
	if (cmp_int(n1o, n2o, v)) return v;

	if (n1d == 0 && n1o == 0)return 0;//no additional, finished

	let* p1 = t1.a->m_ops.data() + t1.h.get_offset();
	let* p2 = t2.a->m_ops.data() + t2.h.get_offset();

	if (p1 == p2) {
		return 0;//refer to the same data
	}

	for (size_t i = 0; i < n1d; ++i) {
		if (cmp_int(p1[i].u64, p2[i].u64, v)) return v;
	}

	//even if the non-operator data is the same,
	//they can be interpreted differently
	//due to different dimensions and projectors
	if (cmp_int(t1.a->m_block_id, t2.a->m_block_id, v)) return v;

	if (n1o == 0) {
		return 0;
	}


	for (size_t i = 0; i < n1o; ++i) {
		t1.h = p1[i].hdr;
		t2.h = p2[i].hdr;

		v = lexic_compare(t1, t2);//recursion
		if (v != 0)return v;
	}

	return 0;//equal
}
void ast_context::hash_combine(ast_context t, size_t& ret)
{
	
	t.h.hash_combine(ret);

	if (t.h.tt == ETYPE::reference) {
		boost::hash_combine(ret, t.call_offset);	
	}

	let nd = t.h.data_args();	boost::hash_combine(ret, nd);
	let no = t.h.oper_args();	boost::hash_combine(ret, no);

	if (no == 0 && nd == 0) {
		return;
	}

	let* p = t.a->m_ops.data() + t.h.get_offset();

	for (size_t i = 0; i < nd; ++i) {
		boost::hash_combine(ret, p[i].u64);
	}

	//even if the non-operator data is the same,
	//they can be interpreted differently
	//due to different dimensions and projectors
	boost::hash_combine(ret, t.a->m_block_id);

	if (no == 0) {
		return;
	}

	for (size_t i = 0; i < no; ++i) {
		t.h = p[i].hdr;
		hash_combine(t, ret);//recursion
	}
}


