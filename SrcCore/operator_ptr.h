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
#include "ims_operator.h"


struct oper_block;

//reference to the operator
struct operator_ptr
{
	ims_operator h;
	const oper_block* a = nullptr;

	//check for complete equality
	static bool equal_raw(const operator_ptr& t1, const operator_ptr& t2)
	{
		return t1.h.as64() == t2.h.as64() && t1.a == t2.a;
	}

	bool is_def() const
	{
		return !h.is_xundef();
	}

	void set_undef()
	{
		h.clear();
	}

	//there's a subtle point here - we're creating a pointer to a number that doesn't exist
	//in the operator array, the number refers to the desired vector component
	operator_ptr index_imm(size_t idx) const
	{
		assert(h.tt == ETYPE::vector_imm);
		operator_ptr ret = *this;
		ret.h.tt = ETYPE::number;//save the subtype
		ret.h.u32 += (uint32_t)idx;
		return ret;
	}

	operator_ptr index_base(size_t idx) const;

	std::span<const uint64_t> get_permutation_params(size_t& dim) const;

	std::string_view get_string() const;
};


struct ast_context: public operator_ptr
{
	ast_context() = default;
	ast_context(operator_ptr p, size_t co) 
		: operator_ptr(p), call_offset(co){};
	size_t call_offset;

	ast_context index(size_t idx) const;

	size_t get_ref_idx() const
	{
		assert(h.tt == ETYPE::reference);
		return h.get_offset() + call_offset;
	}
	//usually performs fast because it exits on the first mismatch
	static intptr_t lexic_compare(ast_context t1, ast_context t2);

	static void hash_combine(ast_context t, size_t& ret);

	std::strong_ordering operator<=>(ast_context rhs) const {
		return lexic_compare(*this, rhs) <=> 0;
	}


	struct raw_hasher
	{
		//compare two elements
		bool operator()(const ast_context& t1, const ast_context& t2) const
		{
			return t1.a == t2.a && t1.h.as64() == t2.h.as64() && t1.call_offset == t2.call_offset;
		}

		//get the element's hash
		size_t operator()(const ast_context& t) const
		{
			size_t ret = t.call_offset;
			boost::hash_combine(ret, t.a);
			boost::hash_combine(ret, t.h.as64());
			return ret;
		}
	};
};



struct distrib_info
{
	ETYPE t;
	ESUBTYPE s;
	double d[2];
};

struct override_info
{
	size_t ref2;//index of the variable in the graph
	ast_context src;
	ast_context proto2;
};

struct control_values
{
	struct elem
	{
		size_t dst_idx9;
		ast_context src;//template from which we copy
	};
	boost::container::small_vector<elem, 2> data;
};




struct control_values2
{
	control_values cv;

	struct elem
	{
		//reference to the element that overrides
		ast_context ptr2;

		//the graph operator was overridden
		bool vis;

		//forced copy
		bool vis2;
	};

	std::vector<elem> a;
};


using builtin_arr = std::array<operator_ptr, c_num_builtins>;


