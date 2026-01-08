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
#include "ast_stack.h"
#include "ims_operator.h"
#include "oper_block.h"


ast_stack::ast_stack(value v)
{
	append(v);
}

void ast_stack::add_branch(value v)
{
	increment();//artificially perform the next iteration
	append(v);
	//hack: a little later after this there will be an increment, and we will get to the right one
	--m_stack.back().v.h;
}

ast_stack& ast_stack::reset3(value v)
{
	m_stack.clear();
	append(v);
	return *this;
}

void ast_stack::append(value v)
{
	auto& nv = m_stack.emplace_back();
	nv.v = v;
	nv.e = v.h + 1;
	push();//immediately go into the depths
}

void ast_stack::push()
{
	for (;;) {

		auto el = m_stack.back();//copy
		auto* h = el.v.h;

		let na = h->oper_args();

		if (na == 0) {
			break;
		}

		//continue to go deeper along the left side in the same block
		el.v.h = &el.v.b->m_ops[h->u32].hdr;
		el.e = el.v.h + na;

		m_stack.emplace_back(el);
	}
}

void ast_stack::increment()
{
	auto& s = m_stack;
	auto& b = s.back();

	//immediately move on to the next one
	if (++b.v.h == b.e) {
		s.pop_back();//child traversal completed
	} else {
		push();//trying to go deeper into the next child
	}

	//interesting...
	//assert(m_stack.size() <= m_stack.static_capacity);
}

ast_stack::value::value(const ast_context& p)
{
	b = const_cast<oper_block*>(p.a);
	h = const_cast<ims_operator*>(&p.h);
	call_offset = p.call_offset;
}
