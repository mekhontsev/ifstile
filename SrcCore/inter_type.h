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

#include "pool_ptr.h"

enum class inter_type : uint8_t
{
	unknown,	//haven't tested yet
	empty,		//don't intersect, no point in further subdivision
	overlapped, //complete overlap, identity map

	//non-trivial intersection is possible
	left,		//subdivide the left one (it's larger)
	right,		//subdivide the right one (it's larger)
	both,		//subdivide the left one (but the dimensions are the same)
	both_neg,	//subdivide the left one (but the dimensions are the same) - the determinant is negative
};

enum class intersect_mode : uint8_t
{
	rational,
	big_rational,
	real,
};



struct inter_elem
{
	using vertex_type = uint32_t;

	vertex_type s0, s1;

	pool_ptr m;

	//actual maximum bit length of matrix elements
	size_t bits = 0;

	//very important for hashing in real arithmetic
	//maps cannot be considered equal
	//if they have different intersection types inter_type
	//this disrupts the operation of other subsystems
	//in particular, cycles arise from inter_type::right
	inter_type res2 = inter_type::unknown;

	inter_type res = inter_type::unknown;

	void set_vers(size_t s0_, size_t s1_)
	{
		s0 = static_cast<vertex_type>(s0_);
		s1 = static_cast<vertex_type>(s1_);
	}

	//true for overlapped, left, both
	bool inter_type_left() const
	{
		assert(res != inter_type::unknown);
		return res != inter_type::right && res != inter_type::empty;
	};

	uint32_t div_ver() const
	{
		assert(res != inter_type::empty);
		return inter_type_left() ? s0 : s1;
	};


	static bool is_eq(const inter_elem& a, const inter_elem& b, double prec);
	static size_t get_hash(const inter_elem& a, double mul_prec);

	size_t get_dim() const;
	size_t get_sz() const;

};

struct inter_result
{
	size_t m_gcx{};		//how many intersections were checked
	size_t m_depth{};	//depth reached
	size_t m_bits{};	//how many bits were used

	//minimum depth where an exact overlap was found
	//0 - no overlap was found
	uint32_t m_over_depth{};

	bool m_completed{}; //intersections are fully created
	bool m_overflowed{};//there was a rational overflow

	//the mode in which the calculations were performed
	intersect_mode m_mode{};
};
