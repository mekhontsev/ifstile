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
#include "math_helpers.h"
#include "poly_funcs.h"
#include "mprec.h"
#include "ims_random.h"

namespace poly_roots
{
	//the polynomial is defined through coefficients, the leading coefficient is 1 and is not stored
template <typename Real>
using Poly = std::vector<Real>;

template <typename Real>
using RootList = std::vector<std::complex<Real>>;


//find the approximation bias using Newton's method (monic polynomial of degree n)
template<typename Integer, typename Real>
inline Real newthon_step(const Integer* a, size_t n, const Real& x)
{
	Real p = 1;//function value
	Real q = 0;//the value of the derivative
	for (size_t i = 0; i < n; ++i) {
		q = p + x*q;
		p = static_cast<Real>(a[n - 1 - i]) + x*p;
	};
	return -p / q;
};

//find the approximation shift using Newton's method (monic polynomial of degree n)
template<typename Integer, typename Real>
inline std::complex<Real> newthon_step_c(const Integer* a, size_t n, const std::complex<Real>& x)
{
	using Complex=std::complex<Real>;
	Complex p(1,0);//function value
	Complex q(0,0);//the value of the derivative
	for (size_t i = 0; i < n; ++i) {
		q = p + x*q;
		p = Complex(static_cast<Real>(a[n - 1 - i]),0) + x*p;
	};

	return -p / q;
};

//finds all roots of a complex polynomial, if there are no multiple roots and not all roots are zero
//returns the maximum relative error over all roots
//uses the Abert-Ehrlich method
// https://en.wikipedia.org/wiki/Aberth_method
//the initial approximation of the roots must be calculated from the outside
template<typename Real>
inline Real compute_ex(RootList<Real>& roots, const Poly<Real> & a)
{
	using Complex=std::complex<Real>;

	const size_t n = a.size();//degree of polynomial

	assert(n > 0);

	assert(roots.size() == n);

	//first we reach the square root of this error, then we continue until it decreases
	let e2 = ims_num_traits<Real>::epsilon();

	Real prevs2 = 2 * e2;//the square of the maximum error of the previous iteration

	Real maxz2;//maximum square of the modulus of roots
	Real maxs2;//maximum square of the modulus of offsets

	Complex s, p, q;
	for (;;) {//iterations

		maxs2 = 0;
		maxz2 = 0;

		for (size_t k = 0; k < n; ++k) {
			auto& z = roots[k];

			//Horner's scheme for simultaneously calculating the value and derivative
			p = Complex(1, 0);//function value
			q = Complex(0, 0);//the value of the derivative
			for (size_t i = 0; i < n; ++i) {
				q = p + z*q;
				p = a[n - 1 - i] + z*p;
			};

			s = Complex(0, 0);
			for (size_t i = 0; i < n; ++i) {
				if (i != k) {
					s += Complex(1, 0) / (roots[k] - roots[i]);
				}
			}

			s = p / (p*s - q);//offset value

			z += s;

			maxz2 = std::max(std::norm(z), maxz2);
			maxs2 = std::max(std::norm(s), maxs2);

		};
		
		if (maxz2 > 0) {
			maxs2 /= maxz2;//relative error
		}
	
		if (maxs2 < e2 && maxs2 >= prevs2*0.99) {
			return maxs2;//the error is small, but has been greatly reduced
		}
		prevs2 = maxs2;
	}
};

//find roots in one step
template<typename Real>
inline Real compute_direct(RootList<Real>& roots, const Poly<Real>& poly)
{
	const size_t n = poly.size();//degree of polynomial
	
	
	
	//initial approximation
	roots.resize(n);


	auto& rng = ims_random::getR().rng;
	std::uniform_real_distribution<Real> distr(-1, 1);

	for (size_t i = 0; i < n; ++i) {
		roots[i] = std::complex<Real>(distr(rng), distr(rng));
	};

	return compute_ex<Real>(roots, poly);
};


//first finds the roots using double, then refines
template<typename Real>
inline Real compute(RootList<Real>& roots, const Poly<Real>& poly)
{
	static_assert(!std::is_fundamental<Real>::value, "error");

	const size_t n = poly.size();//degree of polynomial

	//convert the polynomial to double precision
	Poly<double> dpoly(n);
	for (size_t i = 0; i < n; ++i) {
		dpoly[i] = static_cast<double>(poly[i]);
	}

	RootList<double> droots;
	compute_direct(droots, dpoly);

	//convert roots from double precision
	roots.resize(n);
	for (size_t i = 0; i < n; ++i) {
		roots[i] = std::complex<Real>(
			static_cast<Real>(droots[i].real()),
			static_cast<Real>(droots[i].imag())
			);
	};

	return compute_ex(roots, poly);
};


inline double compute(RootList<double>& roots, const Poly<double>& poly)
{
	return compute_direct(roots, poly);
};

inline double compute(RootList<float>& roots, const Poly<float>& poly)
{
	return compute_direct(roots, poly);
};

////////////////////////////////////////////////////////////////////////////////
//find the multiplicity of each root in the roots array
//input poly - the original polynomial (without the leading coefficient)
//output poly - the derivative (with the leading coefficient) that is not equal to zero at any root
template<typename Real>
inline void roots_multiplicity(std::vector<size_t>& mul, Poly<Real>& poly, const RootList<Real>& roots)
{
	mul.resize(roots.size());
	std::fill(mul.begin(), mul.end(), 1);//multiplicity is at least 1

	size_t n = poly.size();
	for (size_t i = 1; i < n; ++i) {
		poly[i - 1] = poly[i] * static_cast<Real>(i);
	}
	poly[n - 1] = static_cast<Real>(n);//the leading coefficient of the derivative
	n -= 1;

	std::complex<Real> s;

	//TODO: can be optimized by not counting the value of dropped roots
	while (n > 0) {//take derivatives sequentially

		size_t numz = 0;//number of zeros in the current derivative

		Real maxc2 = 0;//the square of the modulus of the maximum coefficient
		for (size_t i = 0; i <= n; ++i) {
			maxc2 = std::max(maxc2, std::norm(poly[i]));
		}
		let e2 = maxc2 * ims_num_traits<Real>::epsilon();

		for (size_t k = 0; k < roots.size(); ++k) {
			let& z = roots[k];

			s = poly[n];//Horner's scheme
			for (size_t i = 0; i < n; ++i) {
				s = s*z + poly[n - 1 - i];
			}

			if (std::norm(s) < e2) {
				mul[k] += 1;
				numz += 1;
			}
		}
		if (numz == 0) {
			break;//no roots
		}

		for (size_t i = 1; i <= n; ++i) {
			poly[i - 1] = poly[i] * static_cast<Real>(i);
		}
		poly.resize(n);
		n -= 1;//new degree
	}
}

//returns integer 1<=n<max_n for which a=b^n, otherwise 0
template<typename Real>
size_t is_pow(
	const std::complex<Real>& a, 
	const std::complex<Real>& b, 
	size_t max_n)
{
	auto x=b;
	let& e2 = ims_num_traits<Real>::epsilon();
	for (size_t n = 1; n < max_n; ++n) {
		if (std::norm(x - a) < e2) {
			return n;
		}
		x *= b;
	}
	return 0;
}


//returns integer 1<=n<max_n for which a=b^n or a=-b^n or a=b'^n or a=-b'^n, otherwise 0
template<typename Real>
size_t is_pow_group(
	const std::complex<Real>& a,
	const std::complex<Real>& b,
	size_t max_n)
{
	auto x = b;
	let& e = ims_num_traits<Real>::almost_zero();

	for (size_t n = 1; n < max_n; ++n) {
		if (std::abs(std::abs(x.real()) - std::abs(a.real()))<e &&
			std::abs(std::abs(x.imag()) - std::abs(a.imag()))<e) {
			return n;
		}
		x *= b;
	}
	return 0;
}

//for polynomials with real coefficients
//retains only one root of a pair of conjugates (im>0)
//for real coefficients, makes the imaginary part exactly equal to zero
//sorts by descending absolute value; if the absolute value is the same, then by descending real part
template<typename Real>
inline void adjust(RootList<Real>& roots)
{
	let e2 = ims_num_traits<Real>::epsilon();
	for (auto& z : roots) {
		if (z.imag()*z.imag() < e2)z.imag(0);
		if (z.real()*z.real() < e2)z.real(0);
	};

	std::erase_if(roots, [](let& z) {return z.imag() < 0;});
	

	std::sort(roots.begin(), roots.end(), [](let& z1, let& z2) {
		let m1 = std::norm(z1);
		let m2 = std::norm(z2);
		let m12 = m1 - m2;
		if (m12*m12 < ims_num_traits<Real>::epsilon()) {//same module
			return z1.real() > z2.real();
		}
		return m12 > 0;
	});
};

template<typename Real>
struct root_elem
{
	std::complex<Real> r;	//root
	Real ar;				//root module
	size_t pw;				//multiplicity>0
	size_t sq;				//from which square-free factor was taken

	size_t degree() const 
	{
		if (r.imag() == 0) {
			return pw;
		} else {
			return pw*2;
		}
	}

	void set(const Real& x, const Real& y, size_t _pw=0, size_t _sq=0)
	{
		r.real(x);
		r.imag(y);
		ar= sqrt(r.real()*r.real() + r.imag()*r.imag());
		pw = _pw;
		sq = _sq;
	}

	//apply a polynomial
	template<typename Number>
	void action(const ims_polynomial<Number>& p, Number d=1)
	{
		let n = p.degree();
		
		//Horner's scheme
		std::complex<Real> s = static_cast<Real>(p[n]);
		for (size_t j = 0; j < n; ++j) {
			s = s*r + static_cast<Real>(p[n - 1 - j]);
		}

		s /= static_cast<Real>(d);

		let as = std::abs(s);

		//if the complex number has become real, then it is necessary to duplicate the multiplicity
		if (std::abs(std::imag(s)) < ims_num_traits<Real>::almost_zero()*as) {
			s.imag(0);
			if (std::abs(std::imag(r)) > ims_num_traits<Real>::almost_zero()*ar) {
				pw *= 2;
			}
		}

		r = s;
		ar = as;
	}

	//sorting by modulo + real part
	static void sort(std::vector<root_elem>& vec){
		
		std::sort(vec.begin(), vec.end(), [](let& e1, let& e2) {
			Real m12 = e1.ar - e2.ar;
			if (std::abs(m12) < ims_num_traits<Real>::almost_zero()) {//identical modules
				return e1.r.real() > e2.r.real();//in descending of the real part
			}
			return m12 > 0;//in descending of module
		});
	}

};

template<typename Real>
using root_list = std::vector<root_elem<Real>>;


template<typename Integer, typename Real>
struct root_finder 
{
	using Poly = ims_polynomial<Integer>;
	using RootList = root_list<Real>;
	using ElemType = root_elem<Real>;



	void divide(const Real& a1, const Real& a2, std::vector<size_t>& dst)
	{
		dst.clear();

		let num = m_relem.size();
		let eps = ims_num_traits<Real>::almost_zero();
		
		
		for(size_t rstart = 0; rstart < num; ++rstart){
	
			let& rs = m_relem[rstart];

			if (rs.degree() != 2)continue;

			if (rs.ar < a1 - eps || rs.ar > a2 + eps) {
				continue;//coefficient out of range
			}

			dst.emplace_back(rstart);
		}

	};

	void find(const Poly& p)
	{
		m_relem.clear();

		m_sqf.factor(p);
	

		for (size_t i = 0; i < m_sqf.ret.size(); ++i) {//loop through polynomials
			let& h = m_sqf.ret[i];
			let n = h.degree();
			m_rpoly.resize(n);

			for (size_t j = 0; j < n; ++j) {
				div_to_real(m_rpoly[j], h[j], h[n]);
			}

			m_roots.clear();
			compute(m_roots, m_rpoly);

			////////////////////////////////////////////////////////////////////////
			let& e2 = ims_num_traits<Real>::epsilon();
			for (auto& q : m_roots) {
				if (q.imag()*q.imag() < e2)q.imag(0);
				if (q.real()*q.real() < e2)q.real(0);

				if (q.imag() >= 0) {
					//std::abs boost::multiprecision
					auto& v = m_relem.emplace_back();
					v.set(q.real(), q.imag(), m_sqf.mul[i], i);
				}
			};
			////////////////////////////////////////////////////////////////////////

			
		}
		root_elem<Real>::sort(m_relem);
	};

	
	//found roots, in decreasing order (modulus + real part)
	RootList m_relem;
private:
	//square-free decomposition
	poly_func::square_free_factor<Integer> m_sqf;
	//polynomial in real form
	std::vector<Real> m_rpoly;
	//temporary storage of roots
	std::vector<std::complex<Real>> m_roots;
};
    
   
//returns the maximum positive real root of the polynomial
//if there is none, returns 0
//also returns the multiplier polynomial that contained this root
template<typename Real>
Real max_positive_root(BigPoly& poly)
{
    poly_func::remove_zero(poly);
    
    poly_func::square_free_factor<big_int_number_boost> F;
    F.factor(poly);
    
	//index of the polynomial with the maximum real root
    size_t max_poly_idx = ims_max;
    Real x = 0;
    root_finder<big_int_number_boost, Real> rf;
    
	//according to the Perron-Frobenius theorem, a real root with the maximum absolute value is needed
    for (size_t i = 0; i < F.ret.size(); ++i) {//run through the polynomials and look for a suitable one
        rf.find(F.ret[i]);
        let& q = rf.m_relem.front().r;
        if (q.imag() != 0 || q.real() <= 0) {
            continue;
        }
        if (q.real() > x) {
            x = q.real();
            max_poly_idx = i;
        }
    }
    
    if (max_poly_idx == ims_max) {
        return 0;
    }
    
    poly = F.ret[max_poly_idx];
    
    ////////////////////////////////////////////////////////////////////////
    poly_func::remove_units(poly, 6);//remove some roots of unity
    
    
    std::vector<int> int_roots;
    
    poly_func::get_int_roots(int_roots, poly, 1000);
    
    const int* fr = nullptr;
    for (let& q : int_roots) {
        let r = static_cast<Real>(q);
        if (std::abs(x - r) < ims_num_traits<Real>::almost_zero()) {
            fr = &q;
            break;
        }
    }
    
    if (fr) {
        poly = { -(*fr), 1 };
    } else {
		//use fact that the polynomial is square-free, so we divide once
        BigPoly de = { 0,1 };
        for (let& q : int_roots) {
            de[0] = -q;
            poly /= de;
        };
    }
    
    return x;
}


}
