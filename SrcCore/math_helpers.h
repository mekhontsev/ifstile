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
#include "ims_num_traits.h"


//v*=a^p, p>=0
template<typename Number>
void mulpow(Number& v, Number a, uint8_t p)
{
	for (;;) {
		if (p & 1)v *= a;
		p >>= 1;
		if (!p)return;
		a *= a;
	}
};

//returns a^p
template<typename Number>
Number ipow(Number a, uint8_t p)
{
	Number r = 1;
	mulpow(r, a, p);
	return r;
}

//closest integer such that v^n <= |a|
template<typename Integer>
Integer root(Integer a, size_t n)
{
	static_assert(std::numeric_limits<Integer>::is_exact);

	if (a < 0)a = -a;
	if (a <= 1)return a;
	if (n == 0) return 0; //error: zeroth root is indeterminate!
	if (n == 1) return a;

	assert(a >= 2);

	Integer v = 2;
	auto tp = ipow(v, n);// first power of two such that v**n >= a
	while (tp < a) {    
		v <<= 1;
		tp = ipow(v, n);
	}
	if (tp == a) return v;  // answer is a power of two
	v >>= 1;
	Integer bit = v >> 1u;
	tp = ipow(v, n);    // v is highest power of two such that v**n < a
	Integer t;
	while (a > tp) {
		v += bit;       // add bit to value
		t = ipow(v, n);
		if (t > a) v -= bit;    // did we add too much?
		else tp = t;
		bit >>= 1;
		if (bit == 0) break;
	}
	return v;
};


//represents numbers a as x^p, returns p
template<typename Integer>
size_t iLogx(const Integer& a, const Integer& x, bool& res)
{
	static_assert(std::numeric_limits<Integer>::is_exact);

	assert(a > 0);
	assert(x > 0);


	if (x == 1) {
		if (a == 1) {
			res = true;
			return 1;
		} else {
			res = false;
			return 0;
		}
	}

	size_t p = 0;
	Integer xp=1;
	for (;;) {
		if (xp == a) {
			res = true;
			return p;
		}
		if (xp > a) {
			res = false;
			return 0;
		}

		xp *= x;
		++p;
	}
}



//find a rational approximation of a real number from 0 to 1 with an accuracy of eps
template<typename Integer, typename Real>
void farey(const Real x, const Real eps, Integer& r, Integer& h)
{
	static_assert(std::numeric_limits<Integer>::is_exact);

	assert(x >= 0 && x <= 1 && eps>0);

	if (x < eps) {
		r = 0;	h = 1;
		return;
	} 
	
	if (x + eps > 1) {
		r = 1;	h = 1;
		return;
	}

	Integer a, b, c, d;
	a = 0; b = 1;
	c = 1; d = 1;
	
	Real e;
	for(;;){	
		r = a + c;
		h = b + d;

		e = x - Real(r) / h;

		if (e > 0) {
			if ( e < eps)return;
			a = r;	b = h;
		} else {
			if (-e < eps)return;
			c = r;	d = h;
		}
	}
};


//finds all prime factors in ascending order and adds them to the array
template<typename Integer>
void find_factors(Integer n, std::vector<Integer>& dst)
{
	static_assert(std::numeric_limits<Integer>::is_exact);

	if (n < 0)n = -n;
	
	if (n < 2) {
		return;
	}
	
	while (n % 2 == 0) {
		dst.emplace_back(2);
		n = n / 2;
	}
	
	Integer z = 3;
	while (z * z <= n) {
		while (n % z == 0) {
			dst.emplace_back(z);
			n = n / z;
		};
		z += 2;
	};

	if (n > 1) {
		dst.emplace_back(n);
	}
}



/* WARNING: undefined if a = 0 */
static inline int clz32(unsigned int a)
{
#if defined(_MSC_VER) && !defined(__clang__)
	unsigned long index;
	_BitScanReverse(&index, a);
	return 31 - index;
#else
	return __builtin_clz(a);
#endif
}

/* WARNING: undefined if a = 0 */
static inline int clz64(uint64_t a)
{
#if defined(_MSC_VER) && !defined(__clang__)
#if INTPTR_MAX == INT64_MAX
	unsigned long index;
	_BitScanReverse64(&index, a);
	return 63 - index;
#else
	if (a >> 32)
		return clz32((unsigned)(a >> 32));
	else
		return clz32((unsigned)a) + 32;
#endif
#else
	return __builtin_clzll(a);
#endif
}


//counts significant bits in O(log(log(v))) time
//suitable for any integer type
template<typename Integer>
size_t number_of_bits(const Integer& v)
{
	static_assert(std::numeric_limits<Integer>::is_exact);
	//static_assert(std::numeric_limits<Integer>::is_signed);

	assert(v >= 0);
	if (v == 0)return 0;

	//means that (v>>max_bits)==0
	size_t max_bits;
	//means that (v>>min_bits)>0
	size_t min_bits;


	if constexpr (std::numeric_limits<Integer>::is_bounded) {
		max_bits = std::numeric_limits<Integer>::digits;
		min_bits = 0;
	} else {
		max_bits = 1;
		while ((v >> max_bits) > 0) {
			max_bits *= 2;
		}
		min_bits = max_bits / 2;
	}

	//assert((v >> max_bits) == 0);
	assert((v >> min_bits) > 0);


	for (;;) {
		let dif = max_bits - min_bits;
		if (dif < 2) {
			break;
		}

		let avg_bits = min_bits + dif / 2;
		if ((v >> avg_bits) == 0) {
			max_bits = avg_bits;
		} else {
			min_bits = avg_bits;
		}
	}

	return max_bits;
};


//calculate the greatest common divisor of the array (>=0)
//can return the index of the last non-zero coefficient
//test - std::array < int64_t, 2 > arr= {-9223372036854775808, -9223372036854775808};
template<typename Integer>
static Integer gcd_arr(const Integer* arr, size_t n, size_t* last_non_zero=nullptr)
{
	static_assert(std::numeric_limits<Integer>::is_exact);

	//looking for the last non-zero
	for (;;) {
		if (n == 0)return 0;
		--n;
		if (arr[n] != 0)break;
	}


	if (last_non_zero) {
		*last_non_zero = n;
	}

	auto ret = arr[n];

	for (size_t i=0; i < n; ++i) {
		if (ret == 1)return 1;
		let& v = arr[i];
		if (v == 0)continue;
		if (ret != v) {
			ret = boost::integer::gcd(ret, v);
		}
	}

	return ret;
}

template<typename Number>
static size_t max_arr_elem_idx(const Number* ptr, size_t sz)
{
	
	Number max_val = 0;
	size_t ret = sz;
	for (size_t i = 0; i < sz; ++i) {
		auto v = ptr[i];
		if (v < 0)v = -v;
		if (v > max_val) {
			ret = i;
			max_val = v;
		}
	}
	return ret;
};

//divide the array by gcd, returns gcd
template<typename Integer>
Integer arr_div_gcd(Integer* ptr, size_t n)
{
	static_assert(std::numeric_limits<Integer>::is_exact);

	size_t fn;//the last non-zero coefficient
	let s = gcd_arr(ptr, n, &fn);
	if (s != 0 && s != 1) {
		for (size_t i = 0; i <= fn; ++i) {
			ptr[i] /= s;
		}
	}
	return s;
};

template<typename Integer>
Integer arr_to_normal_form(Integer* ptr, size_t n)
{
	static_assert(std::numeric_limits<Integer>::is_exact);

	size_t fn;//index of the last non-zero coefficient
	let s = gcd_arr(ptr, n, &fn);
	if (s == 0)return s;//zero matrix

	if (s != 1) {//divide
		for (size_t i = 0; i <=fn; ++i) {
			ptr[i] /= s;
		}
	}
	if (ptr[fn] > 0) {
		return s;
	}

	for (size_t i = 0; i <= fn; ++i) {
		ptr[i] *= -1;
	}
	return -s;
}

template<typename Integer>
Integer checked_mul(const Integer& a1, const Integer& a2)
{
	static_assert(std::numeric_limits<Integer>::is_exact);

	Integer ret = a1*a2;
#ifndef NDEBUG
	let aret = ret>=0?ret:-ret;
	let b1 = a1 >= 0 ? a1 : -a1;
	let b2 = a2 >= 0 ? a2 : -a2;
	
	if (ret > 0) {
		assert(aret >= b1 && aret >= b2);
		assert(a1 > 0 && a2 > 0 || a1 < 0 && a2 < 0);
	}
	if (ret < 0) {
		assert(aret >= b1 && aret >= b2);
		assert(a1 > 0 && a2 < 0 || a1 < 0 && a2>0);
	}
#endif
	return ret;
}

template<typename Number>
Number max_abs_array_entry(const Number* src, size_t sz)
{
	Number ret = 0;
	for (let* last = src + sz; src < last; ++src) {
		ret = std::max(ret, std::abs(*src));
	}
	return ret;
};

template<typename Number>
bool dot_product_checked(Number& s, const Number* left, const Number* right, size_t dim)
{
	constexpr auto M = std::numeric_limits<Number>::max();
	constexpr auto N = std::numeric_limits<Number>::lowest();

	s = 0;

	for (size_t i = 0; i < dim; ++i) {
		let& ml = left[i];
		let& mr = right[i];

		if (ml == 0 || mr == 0)continue;

		if (std::abs(ml) > M / std::abs(mr)) {
			return false;
		}

		let p = ml * mr;

		if (p > 0) {
			if (s > M - p) {//s+p>M
				return false;
			}
		} else {
			if (s < N - p) {//s+p<N
				return false;
			}
		}
		s += p;
	}
}




////////////////////////////////////////////////////////////////////////////////
//execute r=a/b
//non-trivial actions are required because, for example,
//a very long integer may not fit into a double

//version for fundamental integer types
template<typename Real, typename Integer>
void div_to_real(Real& r, const Integer& a, const Integer& b)
{
	static_assert(
		std::numeric_limits<Integer>::is_exact &&
		std::numeric_limits<Integer>::is_bounded);
	static_assert(std::is_fundamental<Integer>::value,
		"this function is only for fundamental types");
	r = static_cast<Real>(a) / static_cast<Real>(b);
};

//if we use boost::multiprecision then it is possible to perform more accurately
template<typename Real, typename Backend>
void div_to_real(Real& r,
	const boost::multiprecision::number<Backend>& a,
	const boost::multiprecision::number<Backend>& b)
{
	using namespace boost::multiprecision;
	const number<rational_adaptor<Backend>> q(a, b);
	r = static_cast<Real>(q);
};



//finds a rational approximation using continued fractions
template<typename Real, typename Integer>
bool rational_approximation(
	Real x, //number being approximated
	Integer& p, //output numerator
	Integer& q, //output denominator
	Real eps, //desired error
	Integer max_val) //maximum permissible absolute values of the numerator and denominator
{
	Integer p0, q0;
	
	p0 = 0; q0 = 1;
	p = 1; q = 0;	

	Real last_error = 1;

	let x0 = x;

	for (;;) {
		let a = (Integer)std::floor(x);
		let p1 = p;
		let q1 = q;

		p = a * p + p0;
		q = a * q + q0;

		if (std::abs(p) > max_val || std::abs(q) > max_val) {
			return false;
		}

		let err = fabs(x0 - Real(p)/Real(q));
		if (err <= eps) {
			return true;
		}
		if (err >= last_error) {
			return false;
		}
		last_error = err;

		p0 = p1;
		q0 = q1;

		x = 1.0 / (x - a);
	}
}


//multiply pl and pr, put the result in dst, the space dimension is dim
//pl has stride equal to dim (col major)
template<typename Integer>
bool dot_prod_int_checked(Integer* dst, const Integer* pl, const Integer* pr, size_t dim)
{
	static_assert(std::numeric_limits<Integer>::is_exact);
	constexpr auto M = std::numeric_limits<Integer>::max();
	constexpr auto N = std::numeric_limits<Integer>::lowest();

	auto& s = *dst;
	s = Integer(0);

	let* pe = pr + dim;

	while (pr < pe) {

		let& ml = *pl;
		let& mr = *pr;

		if (ml != 0 && mr != 0) {
			if (std::abs(ml) > M / std::abs(mr)) {
				return false;
			}

			let p = ml * mr;

			if (p > 0) {
				if (s > M - p) {//s+p>M
					return false;
				}
			} else {
				if (s < N - p) {//s+p<N
					return false;
				}
			}
			s += p;
		}

		pl += dim;	//next column
		pr += 1;	//next row
	};

	return true;
};


//multiply left and right, put the result in dst, the space dimension is dim
//matrices are stored column-wise
template<typename Integer>
bool mul_matrix_checked_int(Integer* dst, const Integer* left, const Integer* right, size_t dim)
{

	assert(left != dst && right != dst);

	for (size_t c = 0; c < dim; ++c) {
		for (size_t r = 0; r < dim; ++r) {
			if (!dot_prod_int_checked(dst, left + r, right + c * dim, dim)) {
				return false;
			}
			++dst;
		}
	};

	return true;
};



template<typename Rational>
auto rational_magnitude(const Rational& q)
{
	if constexpr (ims_get<Rational>::is_big) {

		return std::max(
			ims_abs(numerator(q)), 
			denominator(q));

	} else {
		using Integer = Rational::int_type;
		using Unsigned = std::make_unsigned_t<Integer>;
		static_assert(std::numeric_limits<Integer>::is_exact);
		static_assert(std::numeric_limits<Integer>::is_bounded);
		constexpr auto N = std::numeric_limits<Integer>::lowest();
		constexpr auto M = static_cast<Unsigned>(std::numeric_limits<Integer>::max());

		auto safe_abs = [](Integer v)
			{
				//let u = static_cast<Unsigned>(v); // u === x mod 2^N
				//return v < 0? (~u + 1) : u;

				return v == N ? M + 1 : static_cast<Unsigned>(std::abs(v));
			};

		return std::max(
			safe_abs(q.numerator()),
			static_cast<Unsigned>(q.denominator())
		);
	}
}


template<typename Rational>
auto rational_vec_magnitude(const Rational* v, size_t sz)
{
	let* ve = v + sz;

	auto ret = rational_magnitude(*v++);
	while (v < ve) {
		ret = std::max(ret, rational_magnitude(*v++));
	}
	return ret;
}




//multiply left and right, put the result in dst, the space dimension is dim
//left has stride equal to dim (col major)
template<typename Rational>
void dot_prod_rational(
	Rational* dst, 
	const Rational* left, 
	const Rational* right, 
	size_t dim)
{
	let* pe = right + dim;
	let* pl = left;
	let* pr = right;

	if constexpr (!ims_get<Rational>::is_big) {

		//this optimization speeds up the search by more than 2 times
		//but is still 1.5 times slower than the integer one
		//because there are 5 multiplications instead of one
		
		auto ns = pl->numerator() * pr->numerator();
		auto ds = pl->denominator() * pr->denominator();

		pl += dim;	//next column
		pr += 1;	//next row

		while (pr < pe) {
		
			let n = pl->numerator() * pr->numerator();
			let d = pl->denominator() * pr->denominator();

			//ns/ds + n/d = (ns*d+ds*n)/(ds*d)
			ns = ns * d + ds * n;
			ds *= d;

			pl += dim;	//next column
			pr += 1;	//next row
		};

		*dst = { ns, ds };//only at the end we convert it to the rational

	}else{

		auto& s = *dst;
		s = Rational(0);
		while (pr < pe) {
			s += (*pl) * (*pr);
			pl += dim;	//next column
			pr += 1;	//next row
		};
	}

};


template<typename Rational>
constexpr auto rational_get_max()
{
	using Integer = Rational::int_type;
	static_assert(std::numeric_limits<Integer>::is_exact);
	static_assert(std::numeric_limits<Integer>::is_bounded);
	using Unsigned = std::make_unsigned_t<Integer>;
	return static_cast<Unsigned>(std::numeric_limits<Integer>::max());
}


//multiply left and right, put the result in dst, the space dimension is dim
//left and right have dim rows and dim + 1 columns
template<typename Rational>
void mul_affine_rational(
	Rational* dst, 
	const Rational* left, 
	const Rational* right, 
	size_t dim)
{
	assert(left != dst && right != dst);

	let sz = dim * (dim + 1);

	for(let* re = right + sz; right < re; right += dim){
		for (size_t r = 0; r < dim; ++r) {
			dot_prod_rational(dst++, left + r, right, dim);
		}
	};

	dst -= dim;
	left += dim * dim;

	for (size_t r = 0; r < dim; ++r) {
		*dst++ += *left++;
	}
};



//get the similarity coefficient or 0 if the map is not similarity
template<typename Real>
Real get_sim(const Real* A, size_t n)
{
	constexpr auto eps = ims_num_traits<Real>::almost_zero();

	let* Ae = A + n * n;

	auto maxCoeff = std::abs(A[0]);
	for (let* p = A + 1; p < Ae; ++p) {
		maxCoeff = std::max(maxCoeff, std::abs(*p));
	}
	let maxErr = maxCoeff * maxCoeff * eps;

	auto dotProduct = [](const Real* c1, const Real* c2, size_t n) {
		Real ret = 0;
		for (size_t i = 0; i < n; ++i) {
			ret += c1[i] * c2[i];
		}
		return ret;
	};

	const Real h2 = dotProduct(A, A, n);// first column

	Real s2 = h2;//accumulate the square of the Frobenius norm

	for (let* c1 = A + n; c1 < Ae; c1 += n) {//starting from the second column
		//the norms of all columns must be equal
		let d2 = dotProduct(c1, c1, n);
		if (std::abs(d2 - h2) > maxErr) {
			return 0;
		}
		s2 += d2;

		//the dot product with the previous columns must be equal to zero
		for (let* c2 = A; c2 < c1; c2 += n) {
			if (std::abs(dotProduct(c1, c2, n)) > maxErr) {
				return 0;
			}
		}
	};

	return sqrt(s2 / n);
};
