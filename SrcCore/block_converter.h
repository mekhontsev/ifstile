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
#include "oper_block.h"
#include "eval_context.h"
#include "ims_val_b.h"
#include "pool_ptr.h"

struct val_recognizer
{
	using Integer = ims_val_b::Rational;
	using Matrix = DynMat<Integer>;
	using Map = Eigen::Map<Matrix>;

	ast_context get_ptr()
	{
		return { block.get_ptr(0), 0 };
	};

	bool recognize(const ims_val* src);

	void init(const oper_block* to);

	using Rational = ims_val_b::Rational;

	struct Val 
	{
		size_t ref;
		size_t idx;
	};

	using Key = std::span < const Rational>;

	struct hasher
	{
		bool operator()(Key m1, Key m2) const
		{
			if (m1.size() != m2.size())return false;
			return std::equal(m1.begin(), m1.end(), m2.begin());
		};	
		size_t operator()(Key m) const
		{
			return boost::hash_value(m);
		};
	};

	/////////////////////////////////////////////////////////////

	ankerl::unordered_dense::map<Key, Val, hasher, hasher> m_map;
	
	std::vector<pool_ptr> m_vals;

	const oper_block* in_block = nullptr;

private:

	
	eval_context ctx;
	oper_block block;

};



struct block_converter
{
	void init(const oper_block& conv, const oper_block* to);

	bool convert(oper_block& dst, const oper_block& src);

private:
	
	oper_block tmp;
	eval_context ctx;
	
	val_recognizer rec;
	std::vector<size_t> conv_refs;
};
