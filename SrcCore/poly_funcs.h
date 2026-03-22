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
#include "math_helpers.h"//gcd_arr

namespace poly_func 
{

template<typename Vector, typename Matrix>
void do_vec(Vector& V, const Matrix& A) 
{
	let n = (size_t)A.rows();
	for (size_t c = 0; c < n; ++c) {
		V.block(c*n, 0, n, 1) = A.col(c);
	}
}

//print the polynomial, n is the length of the poly array
template<typename Number, typename Stream>
void print(
	const Number* poly, 
	size_t n, 
	bool monic,  
	Stream& str, 
	const char* var="x")
{
	
	if (monic) {
		if (n > 0) {
			str << "x";
		}
		if (n > 1) {
			str << "^" << n;
		}
	}
	
	for (size_t i = 0; i < n; ++i) {
		let p = n - 1 - i;
		let& c = poly[p];
		if (c == 0)continue;
		
		if (monic || i > 0 || c < 0) {
			str << (c > 0 ? "+" : "-");
		}
		
		let ac = abs(c);
		if (ac != 1 || p == 0) {
			str << ac;
			if(p>0)str << "*";
		}

		if (p > 0) {
			str << var;
			if (p > 1) {
				str << "^" << p;
			}
		}
	};
};

//make the highest degree positive, divide by the GCD of the coefficients
template<typename Integer>
Integer adjust(std::vector<Integer>& p)
{
	if (p.empty())return 0;
	
	auto n = gcd_arr(p.data(), p.size());
	if (p.back() < 0)n *= -1;

	if (n == 0 || n == 1)return n;

	//split branches, otherwise integer overflow occurs
	if (n == -1)
		for (auto& a : p)a = -a;
	else
		for (auto& a : p)a /= n;
	return n;
}

//find the GCD of two polynomials (with correction at each step)
//returns a LINK, invalidates the arguments
template < typename Integer >
ims_polynomial<Integer>& pseudo_gcd
(
	ims_polynomial<Integer>& a,
	ims_polynomial<Integer>& b
)
{
	for (;;) {
		if (a.size()<1)return b;
		b %= a;	adjust(b.data());

		if (b.size()<1)return a;
		a %= b;	adjust(a.data());
	}
};


//partial factorization of a polynomial - Yun's algorithm
//fills in a pair: array of factors + array of multiplicities
//factors are relatively prime and do not contain multiple roots
template<typename Integer>
struct square_free_factor
{
	using Poly = ims_polynomial<Integer>;

	std::vector<Poly> ret;		//factors
	std::vector<size_t> mul;	//multiplicities
	

	void factor(const ims_polynomial<Integer>& P) 
	{
		ret.clear();
		mul.clear();

		size_t pw = 1;

		let n = P.size();
		
		h.data().resize(n - 1);
		for (size_t i = 1; i < n; ++i) {
			h[i - 1] = i*P[i];
		}

		g1 = P; g2 = h;
		g = pseudo_gcd(g1, g2);

		h = P / g;
		adjust(h.data());

		while (h.degree() > 0) {

			g1 = g; g2 = h;
			hn = pseudo_gcd(g1, g2);

			g = g / hn;
			adjust(g.data());

			ret.emplace_back(h / hn);
			adjust(ret.back().data());

			if (ret.back().degree() > 0) {
				mul.emplace_back(pw);
			} else {
				ret.pop_back();
			}
			pw++;
			std::swap(h, hn);
		};
	}

private:

	Poly h;//derivative
	Poly g1, g2;//temporary for GCD
	Poly g;
	Poly hn;
};



//find integer roots from the range [-R,R]
template<typename Integer>
void get_int_roots(
	std::vector<int>& dst,
	const ims_polynomial<Integer>& src, 
	const size_t R)
{
	dst.clear();

	let& h = src.data();

	let a0 = h[0] >= 0 ? h[0] : -h[0];

	let n = (a0 < static_cast<Integer>(R)) ?
		static_cast<int>(a0) :
		static_cast<int>(R);

	for (int k = -n; k <= n; ++k) {
		let ak = std::abs(k);
		if ((ak <= 1 || a0 % ak == 0) && src.evaluate(k) == 0) {
			dst.push_back(k);
		}
	}
};

//remove zero factors, returns the multiplicity of zero
template<typename Integer>
size_t remove_zero(ims_polynomial<Integer>& src) 
{
	auto& h = src.data();
	//remove zeros
	for (size_t i = 0; i < h.size(); ++i) {
		if (h[i] == 0)continue;
		h.erase(h.begin(), h.begin() + i);
		return i;
	}
	return 0;
};


//remove all roots of unity up to and including degree N
template<typename Integer>
void remove_units(ims_polynomial<Integer>& src, const size_t N)
{
	ims_polynomial<Integer> dv,tmp;

	auto& h = dv.data();
	h.reserve(N + 1);

	for (size_t n = 1; n <= N; ++n) {
		for (int a = -1; a < 1; a += 2) {
			h.resize(n + 1);
			std::fill(h.begin(), h.end(), 0);
			h[n] = 1;
			h[0] = a;

			tmp = src;
			let& res = poly_func::pseudo_gcd(tmp, dv);
			if (res.size() > 1) {
				src /= res;
				poly_func::adjust(src.data());
			}
		}
	}
};


//makes a substitution x->-x so that the maximum coefficient with the degree of the inverse of the parity
//of the polynomial itself is negative
//returns true if the polynomial has been modified
template<typename Integer>
bool adjust_signs(ims_polynomial<Integer>& poly, bool plus)
{
	assert(poly.size() > 0);

	let n = poly.degree();

	for (size_t k = 0; k < n; ++k) {
		let i = n - 1 - k;//from greater to lesser degrees

		if ((i + n) % 2 == 0)continue;

		let& q = poly[i];

		if (q == 0)continue;//skip zeros

		if ((plus && q > 0) || (!plus && q < 0))break;//OK

		//change the sign of the powers to the opposite
		for (size_t j = 0; j < n; ++j) {
			if ((j + n) % 2 == 0)continue;
			poly[j] *= -1;
		}

		return true;
	}

	return false;
};

//substitute y=L/R*x
template<typename Integer>
Integer replace_var(
	std::vector<Integer>& dst,
	const Integer& L,
	const Integer& R)
{
	//x^n+ax^(n-1)+bx^(n-2)+ ... q
	//R^n*y^n+ ay^(n-1)*R^(n-1)*L + by^(n-2)R^(n-2)L^2+ ...q*L^n

	Integer mL = 1;
	Integer mR = 1;
	let sz = dst.size();
	for (size_t i = 0; i < sz; ++i) {
		dst[i] *= mR;
		dst[sz - i - 1] *= mL;
		mR *= R;
		mL *= L;
	};

	return adjust(dst);
};

/*
a(s)=a0+a1*s+....a[n]*s^n,  a(s)=0, a[n]>0
g=b(s)=b0+b1*s+....b[n-1]*s^(n-1)
s*g=b0*s+b1*s^2+....+b[n-1]*s^n
s^n=-1/a[n]*(a0+a1*s+....a[n-1]*s^(n-1))
b[n-1]*s^n=-b[n-1]/an*(a0+a1*s+....a[n-1]*s^(n-1))
s*b(s)=b0*s+b1*s^2+....-b[n-1]/an*(a0+a1*s+....a[n-1]*s^(n-1))
s*b(s)=1/a[n]
(
a[n]	*(0 + b0*s + b1*s^2+... + b[n-2]*s^(n-1))
-b(n-1)	*(a0+ a1*s +...         + a[n-1]*s^(n-1))
)
*/

//b=s*b(s) if a(s)=0
//returns the denominator
template<typename Integer>
Integer mul_var(std::vector<Integer>& b, std::span<const Integer> a)
{
	let n = a.size() - 1;
	assert(a[n] > 0);
	assert(b.size()<=n);

	b.resize(n,0);

	let bx = b[n - 1];
	for (size_t i = n - 1; i > 0; --i) {
		b[i] = b[i - 1] * a[n];
	}
	b[0] = 0;

	for (size_t i = 0; i < n; ++i) {
		b[i] -= a[i] * bx;
	}

	for (size_t i = n; i > 0; --i) {
		if (b[i-1] != 0) {
			b.resize(i);
			break;
		}
	}

	
	return a[n];
};




}


