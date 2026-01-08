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
#include "mprec.h"

namespace Detail{
template<typename Real>
Real consteval sqrtNewtonRaphson(Real x, Real curr, Real prev)
{
	return curr == prev
		? curr
		: sqrtNewtonRaphson(x, Real(0.5) * (curr + x / curr), curr);
}
template<typename Real>
Real consteval consteval_sqrt(Real x)
{
	return Detail::sqrtNewtonRaphson(x, x, Real(0));
}
}


template<typename Real>
struct ims_num_traits
{
	static consteval Real epsilon()
	{
		return std::numeric_limits<Real>::epsilon();
	}

	static consteval Real almost_zero()
	{
		return Detail::consteval_sqrt(epsilon());
	}

};


template<>
struct ims_num_traits<float_number_boost>
{
	static float_number_boost epsilon()
	{
		return m_epsilon;
	}

	static float_number_boost almost_zero()
	{
		return m_almost_zero;
	}

	static float_number_boost m_epsilon;
	static float_number_boost m_almost_zero;

};

