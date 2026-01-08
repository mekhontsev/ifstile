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

//dst = l_mat * r_mat
template<typename T>
void mul_mat_mat(T* dst, const T* l_mat, const T* r_mat, size_t n)
{
	assert(l_mat != dst);
	assert(r_mat != dst);

	for (size_t i = 0; i < n; ++i) {//rows of the result
		for (size_t j = 0; j < n; ++j) {//columns
			let jn = j * n;
			dst[i + jn] = T(0);
			for (size_t k = 0; k < n; ++k) {
				dst[i + jn] += l_mat[i + k * n] * r_mat[k + jn];
			}
		}
	};
}

//dst = mat * vec
template<typename T>
void mul_mat_vec(T* dst, const T* mat, const T* vec, size_t n)
{
	assert(vec != dst);

	for (size_t i = 0; i < n; ++i) {
		dst[i] = T(0);
		for (size_t k = 0; k < n; ++k) {
			dst[i] += mat[i + k * n] * vec[k];
		}
	};
}

//dst = l_vec + r_vec
template<typename T>
void add_vec(T* dst, const T* l_vec, const T* r_vec, size_t n)
{
	for (size_t i = 0; i < n; ++i) {
		dst[i] = l_vec[i] + r_vec[i];
	};
}


//dst += vec * s
template<typename T>
void add_vec_mul(T* dst, const T* vec, const T& s,  size_t n)
{
	for (size_t i = 0; i < n; ++i) {
		dst[i] += vec[i]* s;
	};
}

//dst = vec  *s
template<typename T>
void mul_vec_scalar(T* dst, const T& s, const T* vec, size_t n)
{
	for (size_t i = 0; i < n; ++i) {
		dst[i] = vec[i] * s;
	};
}


//dst = aff * vec
template<typename T>
void mul_aff_vec(T* dst, const T* aff, const T* vec, size_t n)
{
	assert(vec != dst);

	let* tr = aff + n * n;

	for (size_t i = 0; i < n; ++i) {
		dst[i] = tr[i];
		for (size_t k = 0; k < n; ++k) {
			dst[i] += aff[i + k * n] * vec[k];
		}
	};
}


template<typename T>
T vec_norm2(const T* p, size_t n)
{
	T ret = 0;
	for (size_t i = 0; i < n; ++i) {
		ret += p[i] * p[i];
	}
	return ret;
}

template<typename T>
T vec_norm(const T* p, size_t n)
{
	return sqrt(vec_norm2(p, n));
}

