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

////////////////////////////////////////////////////////////////////////////////
using mprec_backend_boost= boost::multiprecision::backends::cpp_bin_float<50>;
using raw_number_boost = boost::multiprecision::number<mprec_backend_boost> ;

using float_number_boost =
#ifdef NDEBUG
raw_number_boost;
#else
boost::multiprecision::number<boost::multiprecision::
	debug_adaptor<mprec_backend_boost>>;
#endif

////////////////////////////////////////////////////////////////////////////////
using mprec_int_backend_boost = boost::multiprecision::backends::cpp_int_backend<>;

using raw_int_number_boost= boost::multiprecision::number<mprec_int_backend_boost>;

using big_int_number_boost=
#ifdef NDEBUG
raw_int_number_boost;
#else
boost::multiprecision::number<boost::multiprecision::
	debug_adaptor<mprec_int_backend_boost>>;
#endif

using BigPoly = boost::math::tools::polynomial<big_int_number_boost>;

////////////////////////////////////////////////////////////////////////////////


template<typename T>
inline std::complex<T> sqrt_generic(const std::complex<T>& a)
{
	if (a.real() == 0 && a.imag() == 0) {
		return std::complex<T>(0, 0);
	}

	T b, c, d, e;

	b = abs(a.real());
	c = abs(a.imag());

	if (b > c) {
		d = c / b;
		e = sqrt(b) * sqrt(0.5 * (1 + sqrt(1 + d*d)));
	} else {
		d = b / c;
		e = sqrt(c) * sqrt(0.5 * (d + sqrt(1 + d*d)));
	}

	std::complex<T>  result;
	if (a.real() >= 0) {
		result.real(e);
		result.imag(a.imag() / (2 * e));
	} else {
		result.imag(a.imag() >= 0 ? e : -e);
		result.real(a.real() / (2 * result.imag()));
	}
	return result;
};


////////////////////////////////////////////////////////////////////////////////
//complex functions
namespace std{

inline float_number_boost norm(const std::complex<float_number_boost>& z)
{
	return z.real()*z.real() + z.imag()*z.imag();
};

inline float_number_boost abs(const std::complex<float_number_boost>& z)
{
	return sqrt(norm(z));
};

inline std::complex<float_number_boost> sqrt(const std::complex<float_number_boost>& z)
{
	return sqrt_generic(z);
};

}


namespace Eigen {

////////////////////////////////////////////////////////////////////////////
template<> struct NumTraits<float_number_boost>
	: GenericNumTraits<raw_number_boost>
{
	typedef float_number_boost Real;
	typedef float_number_boost NonInteger;
	typedef float_number_boost Nested;


	enum {
		IsComplex = 0,
		IsInteger = 0,
		RequireInitialization = 1,
		ReadCost = 20,
		AddCost = 30,
		MulCost = 40
	};

	static inline Real lowest()
	{
		static bool f = false;
		static Real s;
		if (!f) {
			f = true;
			s = -highest();
		}
		return s;
	}

	static inline Real dummy_precision()
	{
		static bool f = false;
		static Real s;
		if (!f) {
			f = true;
			const auto pw = ::log(NumTraits<double>::dummy_precision()) /
				::log(NumTraits<double>::epsilon());
			s = pow(epsilon(), pw);
		}
		return s;
	}
};


};//namespace Eigen


