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
#include "operator_ptr.h"

struct ims_operator;
struct oper_block;

//an object for iterating over the AST
//in-place value substitutions can be made during iteration
//but oper_block::m_ops cannot be extended
struct ast_stack
{
	struct value
	{
		//pointers must survive the loop iteration
		oper_block* b = nullptr;
		ims_operator* h  = nullptr;
		size_t call_offset = 0;

		value()
		{
			b = nullptr;
			h = nullptr;
			call_offset = 0;
		};

		ast_context get() const
		{
			return { { *h, b}, call_offset };
		}

		value(const ast_context& p);

		value(oper_block* b_, ims_operator* h_, size_t call_offset_ = 0) :
			b(b_), h(h_), call_offset(call_offset_) {};
	};

	ast_stack() = default;
	ast_stack(value v);

	//add a subtree to check
	void add_branch(value v);

	//all over again
	ast_stack& reset3(value v);

	////////////////////////////////////////////////////////////////////////////

	//an iterator that allows you to traverse an AST subtree without visiting links
	//traverses the parent tree after it has traversed all the child trees
	class iterator :
		public boost::iterator_facade<iterator, value,
		boost::forward_traversal_tag, value&>
	{

	public:

		iterator(ast_stack* t) :m_ai(t) {};

		//for comparison with end()
		bool operator==(int) { return  m_ai->m_stack.empty(); }
	private:

		friend class boost::iterator_core_access;

		//if we point at someone, it means we've already gone deeper than everyone else
		void increment() {
			m_ai->increment();
		}

		value& dereference() const {
			return m_ai->dereference();
		}

		//we'll be changing it as we go
		ast_stack* m_ai;
	};

	iterator begin() { return this; }
	int end() { return 0; }

private:

	struct stack_elem
	{
		value v;
		ims_operator* e;
		stack_elem()
		{
			v.b = nullptr;
			v.h = nullptr;
			e = nullptr;
		};
	};
	
#if 0 //#ifndef NDEBUG
	std::vector<stack_elem> m_stack;
#else
	boost::container::small_vector<stack_elem, 32> m_stack;
#endif // !NDEBUG

	//if we point at someone, it means we've already gone deeper than everyone else
	void increment();

	value& dereference() { return m_stack.back().v; }

	void append(value v);
	//add children for checking
	void push();
};
