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

using ims_integer = int64_t;
using ims_rational = boost::rational<ims_integer>;
using ims_integer_big = boost::multiprecision::cpp_int;
using ims_rational_big = boost::multiprecision::cpp_rational;

template<typename T>
using ims_polynomial = boost::math::tools::polynomial<T>;

template<typename Rational> struct ims_get {};
template<> struct ims_get<ims_rational>
{
	using int_type = ims_rational::int_type;
	using abs_max_t = std::make_unsigned_t<int_type>;
	static constexpr bool is_big = false;
};

template<> struct ims_get<ims_rational_big>
{
	using int_type = ims_rational_big::value_type;
	using abs_max_t = int_type;
	static constexpr bool is_big = true;
};

template<> struct ims_get<double>
{
	using abs_max_t = double;
};

inline ims_integer numerator(const ims_rational& v) 
{
	return v.numerator(); 
};
inline ims_integer denominator(const ims_rational& v) 
{
	return v.denominator(); 
};

//specializing the formatter for any boost::multiprecision number
template <typename Backend, boost::multiprecision::expression_template_option ExpressionTemplates>
struct fmt::formatter<boost::multiprecision::number<Backend, ExpressionTemplates>> : fmt::formatter<std::string> {
	auto format(const boost::multiprecision::number<Backend, ExpressionTemplates>& n, format_context& ctx) const {
		return fmt::formatter<std::string>::format(n.str(), ctx);
	}
};

namespace boost
{
	inline std::size_t hash_value(const ims_rational& v)
	{
		size_t seed = 0;
		hash_combine(seed, v.numerator());
		hash_combine(seed, v.denominator());
		return seed;
	}


	inline std::size_t hash_value(std::span<const ims_rational> s)
	{
		size_t seed = 0;
		hash_range(seed, s.begin(), s.end());
		return seed;
	}
}

