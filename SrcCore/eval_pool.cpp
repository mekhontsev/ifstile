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


ims_val* eval_pool::alloc(ETP t, EST s, size_t data_bytes, size_t sz)
{
	let idx = ims_pool::get_idx(sizeof(ims_val) + data_bytes);
	return new (m_pool.alloc_by_idx(idx)) ims_val(sz, idx, t, s);
}


void eval_pool::release(const ims_val* v)
{
	assert(v);//should be checked outside

	if (v->dec_ref() > 1) {
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

size_t eval_pool::get_data_capacity(const ims_val* v)
{
	return ims_pool::get_size(v->get_bucket()) - sizeof(ims_val);
}

template<ims_val_b::EST s>
ims_val* update_vec_size_ex(ims_val* v, size_t new_sz)
{
	assert(v && v->is(ims_val_b::ETP::vector) && v->is(s));

	using T = typename ims_val::get_t<s>::t;

	//current capacity and size, in elements
	let	cap = eval_pool::get_data_capacity(v) / sizeof(T);
	let sz = v->get_size();

	if (cap < new_sz) {//copy old elements into a new vector
		auto* nv = eval_pool::ep.get_vector_ex<s>(new_sz);
		std::copy(v->template gps<s>(), v->template gps<s>() + sz, nv->template gps<s>());
		std::fill(nv->template gps<s>() + sz, nv->template gps<s>() + new_sz, T());

		if constexpr (s == ims_val_b::EST::other) {
			for (size_t i = 0; i < sz; ++i) {
				let* a = nv->p_v(i);
				if (a)a->add_ref();
			}
		}
		return nv;
	}

	//make all changes in-place
	v->set_size(new_sz);
	if (sz < new_sz) {//extend by zeros
		std::fill(v->template gps<s>() + sz, v->template gps<s>() + new_sz, T());
	} else {//shrink
		if constexpr (s == ims_val_b::EST::big_rational) {
			auto* d = v->p_b();
			for (size_t i = new_sz; i < sz; ++i) {
				d[i].~ims_rational_big();
			}
		}
		if constexpr (s == ims_val_b::EST::other) {
			auto* d = v->p_v();
			for (size_t i = new_sz; i < sz; ++i) {
				let* a = d[i];
				if (!a)continue;
				d[i] = nullptr;
				eval_pool::ep.release(a);
			}
		}
	}
	v->add_ref();
	return v;
}

ims_val* eval_pool::update_vec_size(ims_val* v, size_t new_sz)
{
	assert(v && v->is(ims_val_b::ETP::vector));
	switch (v->gs())
	{
		case ims_val_b::EST::rational:
			return update_vec_size_ex<ims_val_b::EST::rational>(v, new_sz);
		case ims_val_b::EST::real:
			return update_vec_size_ex<ims_val_b::EST::real>(v, new_sz);
		case ims_val_b::EST::big_rational:
			return update_vec_size_ex<ims_val_b::EST::big_rational>(v, new_sz);
		case ims_val_b::EST::other:
			return update_vec_size_ex<ims_val_b::EST::other>(v, new_sz);
		default:
			return nullptr;
	}
}


ims_val* eval_pool::get_pointer(void* p, EST s)
{
	auto* ret = alloc_scalar(ETP::ptr, s, 0);
	ret->m_ptr = p;
	return ret;
}

const ims_val* eval_pool::get_pointer(ims_val* v, size_t idx)
{
	assert(idx <= v->get_size());
	void* ptr;
	switch (v->gs()) {
	case EST::other:
	{
		auto* ret = v->p_v()[idx];
		if (ret)ret->add_ref();
		return ret;
	}
	case EST::rational:		ptr = v->p_i() + idx; break;
	case EST::real:			ptr = v->p_r() + idx; break;
	case EST::big_rational:	ptr = v->p_b() + idx; break;
	default:				ptr = nullptr; break;
	}
	return get_pointer(ptr, v->m_s);
}

ims_val* eval_pool::update_string(ims_val* v, std::string_view src)
{
	if (v && !v->is(ims_val_b::ETP::string)) {
		v = nullptr;
	}

	let cap_chars = v ? get_data_capacity(v) : 0;
	if (!v || cap_chars < src.size()) {
		return get_string(src);
	}

	std::copy(src.data(), src.data() + src.size(), v->gp<char>());
	v->set_size(src.size());
	v->add_ref();
	return v;
}

ims_val* eval_pool::get_string(std::string_view s)
{
	auto* ret = alloc(ETP::string, EST::pod, s.length(), s.length());
	std::copy(s.begin(), s.end(), ret->gp<char>());
	return ret;
}

ims_val* eval_pool::get_indexed_object(size_t idx, ETP t)
{
	return alloc(t, EST::pod, 0, idx);
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

ims_val* eval_pool::get_scalar_big_rational()
{
	auto* ret = alloc_scalar(ETP::number, EST::big_rational, sizeof(ims_val_b::BigRational));
	new (ret->p_b()) ims_val_b::BigRational();
	return ret;
}

ims_val* eval_pool::get_scalar_real(Real v, ETP t)
{
	auto* ret = alloc_scalar(t, EST::real, sizeof(Real));
	*ret->p_r() = v;
	return ret;
}


ims_val* eval_pool::get_vector_int(size_t sz)
{
	return get_vector_ex<EST::rational>(sz);
}

static void init_big_rational_vec(ims_val* v, size_t sz)
{
	auto* d = v->p_b();
	for (let* de = d + sz; d < de; ++d) {
		new (d) ims_val_b::BigRational();
	}
}

ims_val* eval_pool::get_vector_big_rational(size_t sz)
{
	auto* ret = get_vector_ex<EST::big_rational>(sz);
	init_big_rational_vec(ret, sz);
	return ret;
}

ims_val* eval_pool::get_vector_real(size_t sz)
{
	return get_vector_ex<EST::real>(sz);
}

ims_val* eval_pool::get_vector(size_t sz, ETP t)
{
	auto* ret = get_vector_ex<EST::other>(sz, t);
	auto* v = ret->p_v();
	std::fill(v, v + sz, nullptr);
	return ret;
}


template<ims_val_b::EST s>
ims_val* eval_pool::get_matrix_ex(size_t r, size_t c)
{
	assert(r > 0 && c > 0);
	return alloc(ETP::matrix, s,
		sizeof(typename ims_val_b::get_t<s>::t) * r * c,
		ims_val::get_dim_field(r, c));
};

ims_val* eval_pool::get_matrix_int(size_t rows, size_t cols)
{
	return get_matrix_ex<EST::rational>(rows, cols);
}

ims_val* eval_pool::get_matrix_real(size_t rows, size_t cols)
{
	return get_matrix_ex<EST::real>(rows, cols);
}

ims_val* eval_pool::get_matrix_big_rational(size_t rows, size_t cols)
{
	auto* ret = get_matrix_ex<EST::big_rational>(rows, cols);
	init_big_rational_vec(ret, rows * cols);
	return ret;
}

const ims_val* eval_pool::adjust_vec_type(const ims_val* vec)
{
	assert(vec && vec->is(ims_val_b::EST::other));
	//try to narrow the type
	auto common_sbt = ims_val::EST::rational;

	vec->visit_childs([&](const ims_val* v) {
		if (!v || !v->is(ims_val::ETP::number)) {
			common_sbt = ims_val::EST::other;
			return false;
		}
		common_sbt = std::max(v->gs(), common_sbt);
		if (common_sbt == ims_val::EST::other) {
			return false;
		}
		return true;//continue
	});

	if (common_sbt == ims_val::EST::other) {
		vec->add_ref();
		return vec;
	}

	ims_val* ret = nullptr;
	let vsz = vec->get_size();

	//conversion from Other, numeric scalars
	if (common_sbt == ims_val::EST::rational) {
		ret = get_vector_int(vsz);
		auto* d = ret->p_i();
		for (size_t j = 0; j < vsz; ++j) {
			d[j] = vec->p_v(j)->get_int();
		};
	} else if (common_sbt == ims_val::EST::big_rational) {
		ret = get_vector_big_rational(vsz);
		auto* d = ret->p_b();
		for (size_t j = 0; j < vsz; ++j) {
			d[j] = vec->p_v(j)->get_big_rational();
		};
	} else {
		assert(common_sbt == ims_val::EST::real);
		ret = get_vector_real(vsz);
		auto* d = ret->p_r();
		for (size_t j = 0; j < vsz; ++j) {
			vec->p_v(j)->to_real(d[j]);
		};
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