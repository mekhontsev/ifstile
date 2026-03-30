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
#include "math_helpers.h"
#include "ims_stage.h"

template<typename Map>
size_t is_zero(const Map& m)
{
	for (let& x : m.reshaped()){
		if (x != 0)return false;
	}
	return true;
}

//maximum absolute value of the coefficients
template<typename Map>
typename Map::Scalar norm_max(const Map& m)
{
	typename Map::Scalar ret = 0;
	for (let& q : m.reshaped()) {
		ret = std::max(ret, q > 0 ? q : -q);
	}
	return ret;
}

template<typename Map>
size_t get_hash(const Map& m)
{
	size_t seed = 0;
	for (let& v : m.reshaped()) {
		boost::hash_combine(seed, v);
	}
	return seed;
}

template<typename Map>
size_t is_id(const Map& m)
{
	using Integer = typename Map::Scalar;
	static_assert(std::numeric_limits<Integer>::is_exact);
	for (size_t r = 0; r < (size_t)m.rows(); ++r) {
		for (size_t c = 0; c < (size_t)m.cols(); ++c) {
			let& v = m(r, c);
			if (r == c) {
				if (v != 1)return false;
			} else {
				if (v != 0)return false;
			}
		}
	}
	return true;
}

//reduce by GCD and make the last non-zero coefficient > 0
//return the factor that was factored out
template<typename Map>
typename Map::Scalar to_normal_form_d(Map& m)
{
	return arr_to_normal_form(m.data(), m.size());
};

//multiply left and right, put the result in dst
template<typename Map1, typename Map2, typename Map>
void mul_x(Map& dst, const Map1& left, const Map2& right)
{
	using Integer = typename Map::Scalar;
	
	assert((void*)&left != (void*)&dst);
	assert((void*)&right != (void*)&dst);

	assert(right.rows()== left.cols());


	//TODO: remove the condition after Eigen's fix
	if constexpr (std::numeric_limits<Integer>::is_bounded) {
		dst.noalias() = left * right;
	} else {
		let nr = (size_t)left.rows();
		let nc = (size_t)right.cols();
		dst.resize(nr, nc);

		let nk = (size_t)left.cols();

		for (size_t i = 0; i < nr; ++i) {
			for (size_t j = 0; j < nc; ++j) {
				dst(i, j) = Integer(0);
				for (size_t k = 0; k < nk; ++k) {
					dst(i, j) += left(i, k) * right(k, j);
				}
			}
		};
	}

}


template<typename Map>
bool less(const Map& left, const Map& right)
{
	let sz1 = left.size();
	let sz2 = right.size();
	if (sz1 != sz2)return sz1 < sz2;
	
	let* d1 = left.data();
	let* d2 = right.data();
	for (size_t i = 0; i < (size_t)sz1; ++i) {
		if (d1[i] != d2[i])return d1[i] < d2[i];
	}
	return false;//equal
}

template<typename Map>
bool can_mul_matrix_new(const Map& left, const Map& right)
{
	using Integer = typename Map::Scalar;

	static_assert(
		std::numeric_limits<Integer>::is_exact &&
		std::numeric_limits<Integer>::is_bounded);
	
	let ml = max_abs_array_entry(left.data(), left.size());
	if (ml == 0)return true;
	let mr = max_abs_array_entry(right.data(), right.size());
	if (mr == 0)return true;
	
	let bl = number_of_bits(ml);
	let br = number_of_bits(mr);

	let d1 = std::max(left.rows(), left.cols());
	let d2 = std::max(right.rows(), right.cols());

	let bd = number_of_bits((int)std::max(d1,d2));

	return bl + br + bd <= std::numeric_limits<Integer>::digits;
}

//multiply homogeneous matrices
template<typename Map>
bool mul_checked(
	Map& dst,
	const Map& left,
	const Map& right)
{
	assert(left.size() == right.size());
	if (!can_mul_matrix_new(left, right)) {
		return false;
	}
	mul_x(dst, left, right);
	to_normal_form_d(dst);
	return true;
}


template<typename Map, typename Integer>
void add_scalar(Map& A, Integer p)
{
	assert(A.rows() == A.cols());
	let d = (size_t)A.rows();
	for (size_t i = 0; i < d; ++i) {
		A(i, i) += p;
	}
}

//get the characteristic polynomial and the adjoint matrix
//https://en.wikipedia.org/wiki/Adjugate_matrix
//poly = characteristic polynomial (A) without the leading coefficient
//returns det(A), B = A^-1 * det(A)
//T - temporary matrix
//uses Fadeev's algorithm
template<typename Matrix, typename Map>
typename Map::Scalar char_poly(
	const Map& A,
	size_t dim,
	Matrix& B,
	Matrix& T,
	typename Map::Scalar* poly = nullptr,
	ims_stage* ri = nullptr)
{
	assert(dim > 0);

	let n = static_cast<int>(dim);//important to convert to signed

	assert(n == A.rows() && n == A.cols());

	using Scalar = typename Map::Scalar;


	//first iteration
	auto p = A.trace();
	if (poly)poly[n - 1] = -p;

	B.resize(n,n);
	assert(n == B.rows() && n == B.cols());

	if (n == 1) {
		B.setIdentity();
		return p;
	}

	B = A;

	T.resize(n,n);
	assert(n == T.rows() && n == T.cols());

	let w = 1.0 / (n - 1);

	//intermediate iterations
	for (int k = 2; k < n; ++k) {
		add_scalar(B, -(Scalar)p);
		mul_x(T, A, B);
		B = T;
		p = B.trace();
		p /= k;
		if (poly)poly[n - k] = -p;

		if (ri) {
			if (ims_need_stop()) {
				return Scalar(0);
			}
			ri->work_add(w);
		}
	}

	//last iteration
	add_scalar(B, -(Scalar)p);
	mul_x(T, A, B);

	if (ri) {
		ri->work_add(w);
	}

	p = T.trace();
	p /= n;
	if (poly)poly[0] = -p;

	if (n & 1)return p;//odd dimension

	B *= Scalar(-1);
	return -p;
}


template<typename Map>
void get_exchange(Map& m, size_t n)
{
	m.resize(n,n);
	m.setZero();

	for (size_t i = 0; i < n; ++i) {
		m(i, n - 1 - i) = 1;
	};
}


//companion matrix, p is a polynomial (not necessarily monic)
template<typename Map>
void get_companion(Map& M,
	std::span<const typename Map::Scalar> p)
{
	let n = p.size() - 1;

	M.resize(n,n);
	M.setZero();

	for (size_t i = 0; i < n; ++i) {
		if (i + 1 < n) {
			M(i + 1, i) = p[n];
		}
		M(i, n - 1) = -p[i];
	};
}

template<typename Map>
bool is_diag(const Map& M)
{
	if (M.rows() != M.cols())return false;
	
	for (size_t r = 0; r < (size_t)M.rows(); ++r) {
		for (size_t c = 0; c < (size_t)M.cols(); ++c) {
			if (r != c && M(r,c) != 0) {
				return false;
			}
		}
	}
	return true;
}

//returns a positive GCD, for zero it returns 0
template<typename Map>
typename Map::Scalar gcd(const Map& m)
{
	return gcd_arr(m.data(), m.size());
};



//A = A * B^p
//p >= 0
//t is temporary, B is invalidated
template<typename Map>
void mulpow_mat(Map& A, Map& B, uint8_t p, Map& t)
{
	for (;;) {
		if (p & 1) {//A=A*B
			mul_x(t, A, B);
			A = t;
		}
		p >>= 1;
		if (!p)return;

		//B=B^2
		mul_x(t, B, B);
		B = t;
	}
};


