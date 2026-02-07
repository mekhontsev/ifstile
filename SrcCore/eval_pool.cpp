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

#include "pch.h"
#include "eval_pool.h"
#include "ims_val.h"

thread_local eval_pool eval_pool::ep;


ims_val* eval_pool::alloc(ETP t, EST s, size_t data_bytes, size_t dim)
{
	let idx = ims_pool::get_idx(sizeof(ims_val) + data_bytes);
	return new (m_pool.alloc_by_idx(idx)) ims_val(dim, idx, t, s);
}


void eval_pool::release(const ims_val* v)
{
	assert(v);//should be checked outside

	if (v->dec_ref() > 0) {
		return;
	}

	switch (v->gs()) {
	case ims_val_b::EST::other:
	{
		//here are only vector-like
		auto* p = v->p_v();
		for (let* pe = p + v->get_size(); p < pe; ++p) {
			if (*p)release(*p);
		}
		break;
	}
	case ims_val_b::EST::big_rational:
	{
		//matrices and numbers
		auto* p = v->p_b();
		for (let* pe = p + v->num_el(); p < pe; ++p) {
			p->~BigRational();
		}
		break;
	}
	default:
		break;
	}
	
	//remove the element itself
	m_pool.dealloc_by_idx(v->get_bucket(), const_cast<ims_val*>(v));
}

////////////////////////////////////////////////////////////////////////////////

void eval_pool::add_ref(const ims_val* v)
{
	if (v)v->add_ref();
}

ims_val* eval_pool::get_empty_val()
{
	//empty union
	return get_vector(0, ETP::uni);
}

ims_val* eval_pool::get_id_val()
{
	//empty composition
	return get_vector(0, ETP::compos);
}

ims_val* eval_pool::get_string(std::string_view s)
{
	auto* ret = alloc(ETP::string, EST::pod, s.length(), s.length());
	std::copy(s.begin(), s.end(), ret->gp<char>());
	return ret;
}

ims_val* eval_pool::alloc_scalar(ETP t, EST s, size_t sz)
{
	return alloc(t, s, sz, 0);
}

ims_val* eval_pool::get_scalar_int(Rational v)
{
	auto* ret = alloc_scalar(ETP::number, EST::rational, sizeof(Rational));
	*ret->p_i() = v;
	return ret;
}

ims_val* eval_pool::get_scalar_real(Real v, ETP t)
{
	auto* ret = alloc_scalar(t, EST::real, sizeof(Real));
	*ret->p_r() = v;
	return ret;
}

ims_val* eval_pool::get_vector(size_t sz, ETP t)
{
	auto* ret = alloc(t, EST::other, sizeof(ims_val*) * sz, sz);
	auto* v = ret->p_v();
	std::fill(v, v + sz, nullptr);
	return ret;
}

ims_val* eval_pool::get_vector_int(size_t sz)
{
	return alloc(ETP::vector, EST::rational, sizeof(Rational) * sz, sz);
}

ims_val* eval_pool::get_vector_real(size_t sz)
{
	return alloc(ETP::vector, EST::real, sizeof(Real) * sz, sz);
}

ims_val* eval_pool::get_matrix_int(size_t rows, size_t cols)
{
	assert(rows > 0 && cols > 0);
	return alloc(ETP::matrix, EST::rational, sizeof(Rational) * rows * cols,
		ims_val::get_dim_field(rows, cols));
}

ims_val* eval_pool::get_matrix_real(size_t rows, size_t cols)
{
	if (rows == 0 || cols == 0) {
		assert(false);
	}
	assert(rows > 0 && cols > 0);
	return alloc(ETP::matrix, EST::real, sizeof(Real) * rows * cols,
		ims_val::get_dim_field(rows, cols));
}



ims_val* eval_pool::get_matrix_big_rational(size_t rows, size_t cols)
{
	let sz = rows * cols;
	assert(sz > 0);
	auto* ret = alloc(ETP::matrix, EST::big_rational,
		sizeof(ims_val_b::BigRational) * sz,
		ims_val::get_dim_field(rows, cols));

	auto* v = ret->p_b();
	for (let* ve = v + sz; v < ve; ++v) {
		new (v) ims_val_b::BigRational();
	}
	return ret;
}


ims_val* eval_pool::get_affine_int(size_t dim)
{
	return get_matrix_int(dim, dim + 1);
}

ims_val* eval_pool::get_affine_real(size_t dim)
{
	return get_matrix_real(dim, dim + 1);
}

ims_val* eval_pool::get_affine_big_rational(size_t dim)
{
	return get_matrix_big_rational(dim, dim + 1);
}