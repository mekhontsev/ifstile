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
#include "dyn_mat_vec.h"

struct ims_val_b
{
	using Integer = ims_integer;
	using Rational = ims_rational;
	using Real = double;
	using BigRational = ims_rational_big;
	//using BigReal = float_number_boost;

	using MMatInt = Eigen::Map<DynMat<Rational>>;
	using MMatBigRational = Eigen::Map<DynMat<BigRational>>;
	using MMatReal = Eigen::Map<DynMat<Real>>;

	using MVecReal = Eigen::Map<DynVec<Real>>;
	using MVecInt = Eigen::Map<DynVec<Rational>>;
	using MVecBigRational = Eigen::Map<DynVec<BigRational>>;


	static_assert(std::is_same_v<BigRational::value_type, boost::multiprecision::cpp_int>);
	static_assert(std::is_same_v<Rational::int_type, Integer>);

	static_assert(std::numeric_limits<BigRational>::is_specialized);
	static_assert(!std::numeric_limits<Rational>::is_specialized);

	static_assert(ims_get<BigRational>::is_big);
	static_assert(!ims_get<Rational>::is_big);
	


	enum class ETP : uint8_t
	{
		number,
		ast_ptr,	//ast_context
		string,		//array of chars
		inversion,	//mobius, relative to the unit ball

		style2,		//index in the palette - real number
		thickness,	//real number

		////////////////////////////////////////////////////////////////////////
		//vector types start here (use the m_dim field)
		vector,	
		_first_vec_type_ = vector,//for natvis only
		compos,	//composition of other ims_val
		uni,	//union other ims_vals

		matrix,	//rectangular matrix, column-major as in Eigen
		csg,	//$csg - Other vector of 4 elements
		mobius,	//size is (m_dim + 2)^2, column-major as in Eigen
		

		//specifies a point that belongs to the basin of attraction of the set
		//when iterating through any loop containing this set
		//geometrically identity,
		attractor,

		num_types,
	};

	
	enum class EST : uint8_t
	{
		//order is important
		rational,
		real,
		pod,	//POD subtype, does not require special processing
		other,	//starting from here, non-trivial creation and deletion is required
		big_rational,
	};
};
