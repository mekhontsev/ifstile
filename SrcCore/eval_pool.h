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

#include "ims_val_b.h"
#include "ims_pool.h"

struct ims_val;

//can allocate values
struct eval_pool: public boost::noncopyable
{
	using ETP = ims_val_b::ETP;
	using EST = ims_val_b::EST;
	using Rational = ims_val_b::Rational;
	using Real = ims_val_b::Real;
	using BigRational = ims_val_b::BigRational;

	void release(const ims_val* v);

	
	ims_val* alloc_scalar(ETP t, EST s, size_t sz = 0);

	ims_val* get_empty_val();
	ims_val* get_id_val();

	static size_t get_data_capacity(const ims_val* v);

	ims_val* update_string(ims_val* v, std::string_view src);
	ims_val* get_string(std::string_view s);
	ims_val* get_indexed_object(size_t idx, ETP t);

	ims_val* update_vec_size(ims_val* v, size_t new_sz);

	ims_val* get_scalar_int(Rational v);
	ims_val* get_scalar_real(Real v, ETP t = ETP::number);
	//allocates a scalar value of type BigRational initialized to zero.
	ims_val* get_scalar_big_rational();

	template<typename T>
	ims_val* get_affine(size_t dim) 
	{
		if constexpr (std::is_same_v<T, Rational>)
			return get_affine_int(dim);
		else if constexpr (std::is_same_v<T, Real>)
			return get_affine_real(dim);
		else if constexpr (std::is_same_v<T, BigRational>)
			return get_affine_big_rational(dim);
		else {
			static_assert(false);
		}
	}

	ims_val* get_affine_int(size_t dim);
	ims_val* get_affine_real(size_t dim);
	ims_val* get_affine_big_rational(size_t dim);

	ims_val* get_vector_int(size_t sz);
	ims_val* get_vector_real(size_t sz);
	ims_val* get_vector(size_t sz, ETP t = ETP::vector);

	ims_val* get_matrix_int(size_t rows, size_t cols);
	ims_val* get_matrix_real(size_t rows, size_t cols);
	ims_val* get_matrix_big_rational(size_t rows, size_t cols);


	ims_pool& get_pool() { return m_pool; }

	static void add_ref(const ims_val* v);

	static thread_local eval_pool ep;

private:

	ims_pool m_pool;

	//returns with one reference, removed via release
	ims_val* alloc(ETP t, EST s, size_t data_bytes = 0,	size_t sz = 0);
};


//using alloc_type = eval_pool_allocator<boost::multiprecision::limb_type>;
//using my_backend = boost::multiprecision::cpp_int_backend<0, 0, boost::multiprecision::signed_magnitude, boost::multiprecision::unchecked, alloc_type>;
//using my_cpp_rational_backend = boost::multiprecision::rational_adaptor<my_backend>;
//using my_cpp_rational = boost::multiprecision::number<my_cpp_rational_backend>;
//my_cpp_rational p = 1;

template<class T>
struct eval_pool_allocator
{
	using value_type = T;

	eval_pool_allocator() = default;

	template<class U>
	eval_pool_allocator(const eval_pool_allocator <U>&){}

	T* allocate(size_t n) noexcept
	{
		return static_cast<T*>(eval_pool::ep.get_pool().allocate(n * sizeof(T)));
	}

	void deallocate(T* p, size_t n) noexcept
	{
		eval_pool::ep.get_pool().deallocate(p, n * sizeof(T));
	}
};

template<class T, class U>
bool operator==(const eval_pool_allocator <T>&, const eval_pool_allocator <U>&) { return true; }

template<class T, class U>
bool operator!=(const eval_pool_allocator <T>&, const eval_pool_allocator <U>&) { return false; }


