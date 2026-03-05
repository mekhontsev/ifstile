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


//generalized value for computations, usually immutable
//changes can only be made by the owner when the reference count == 1
struct alignas(16) ims_val : public ims_val_b
{
private:

	//can be temporarily used for other purposes if reference counting is not needed
	mutable std::atomic<size_t> m_use_count;

	//for scalars it can be used for other purposes
	union {
		//for vectors: number of elements
		//for strings: length
		//for jsval - index in the js_engine
		uint32_t m_size;
		//for matrices - dimensions: rows, cols
		std::array<uint16_t, 2> m_ex;
	};
	
	//32-bit block
	uint8_t m_reserved;	//if necessary, m_size can be expanded to 40 bits
	uint8_t m_bucket:5;	//bucket index in the pool, capacity for data
	uint8_t m_flags:3;	//for various purposes
	ETP	m_t;
	EST	m_s;

	//uint64_t m_data[];//https://en.wikipedia.org/wiki/Flexible_array_member

public:

	static size_t get_dim_field(size_t rows, size_t cols)
	{
		assert(rows <= 0xFFFF);
		assert(cols <= 0xFFFF);
		return rows | (cols << 16);
	};

	template<typename T>
	static consteval EST get_subtype()
	{
		if constexpr (std::is_same_v<T, Rational>)
			return EST::rational;
		else if constexpr (std::is_same_v<T, Real>)
			return EST::real;
		else if constexpr (std::is_same_v<T, BigRational>)
			return EST::big_rational;
		else
			return EST::other;
	}

	//returns whether the ball can be modified under its action
	//cutting is not a change!
	static bool is_geom(ETP t)
	{
		constexpr auto get_arr = []()
		{
			auto lst = {
				ETP::number, 
				ETP::vector,
				ETP::inversion,
				ETP::mobius,
				ETP::matrix,
				ETP::compos,
				ETP::uni,//empty union
				ETP::attractor,
			};

			std::array<bool, (size_t)ETP::num_types> ret{ false };
			for (auto q : lst) {
				ret[(size_t)q] = true;
			}
			return ret;
		};
		
		static constexpr auto arr{ get_arr() };
		static_assert(arr[(size_t)ETP::number] == true);
		static_assert(arr[(size_t)ETP::style2] == false);
		return arr[(size_t)t];
	}

	

	template<typename T> auto* gp() const { return (T*)(this + 1); }

	auto* p_r() const { assert(is(EST::real));	return gp<Real>(); };
	auto* p_i()	const { assert(is(EST::rational));	return gp<Rational>(); };
	auto* p_b() const { assert(is(EST::big_rational));	return gp<BigRational>(); };
	auto* p_v() const { assert(is(EST::other));	return gp<const ims_val*>(); };
	
	auto p_r(size_t idx) const { return p_r()[idx]; };
	auto p_i(size_t idx) const { return p_i()[idx]; };
	auto p_b(size_t idx) const { return p_b()[idx]; };
	let* p_v(size_t idx) const { return p_v()[idx]; };
	

	ETP	gt() const { return m_t; };
	EST	gs() const { return m_s; };

	bool is(ETP t, EST sbt) const { return gt() == t && gs() == sbt; };
	bool is(ETP t) const { return gt() == t; }
	bool is(EST s) const { return gs() == s; }

	////////////////////////////////////////////////////////////////////////////

	//for example, we can immediately compare both matrix sizes
	size_t raw_size() const
	{
		return m_size;
	}

	Real* get_affine() const
	{
		assert(is(ims_val::EST::real));
		assert(is_affine());
		return p_r();
	}

	void set_flags(uint8_t f) { m_flags = f; };
	uint8_t get_flags() const { return m_flags; };

	//for debugging purposes
	bool is_normal() const;

	static size_t affine_num_elems(size_t dim) 
	{
		return dim * (dim + 1);
	}

	size_t affine_num_elems() const 
	{
		if (!is_affine())return 0;
		return affine_num_elems(extent(0));
	}


	size_t extent(size_t idx) const
	{
		assert(is(ETP::matrix));
		return m_ex[idx];
	}

	size_t rows() const
	{
		return extent(0);
	};

	size_t cols() const
	{
		return extent(1);
	};

	size_t get_size() const
	{
		return m_size;
	};

	std::string_view get_string() const
	{
		assert(is(ETP::string));
		return { gp<char>(), m_size };
	}

	bool is_empty() const
	{
		return is(ims_val::ETP::uni) && get_size() == 0;
	}

	size_t num_el() const;

	bool is_id() const
	{
		return is(ims_val::ETP::compos) && get_size() == 0;
	}
	
	//use only for shrinking!
	void shrink(size_t new_size) 
	{
		assert(new_size <= m_size);
		m_size = static_cast<uint32_t>(new_size);
	}

	Real get_real() const;
	Rational get_int() const;
	BigRational& get_big_rational() const;


	size_t vec_length() const;

	size_t num_vec_length() const;

	bool is_true() const;//for the ternary operator


	bool to_big_rational(ims_val_b::BigRational& v) const;
	bool to_int(int64_t& v) const;
	bool to_real(Real& v) const;

	//dim - dimension of the affine space
	ETP common_affine_type(ETP t, size_t dim) const;
	EST common_subtype(EST t) const;

	
	const ims_val* elevate_compos() const;


	bool is_affine() const 
	{
		return is(ETP::matrix) && extent(0) + 1 == extent(1);
	}

	size_t get_affine_dim() const 
	{
		assert(is_affine());
		return extent(0);
	}

	////////////////////////////////////////////////////////
	size_t affine_index(size_t row, size_t col) const
	{
		assert(is(ETP::matrix));
		return row + col * extent(0);
	};

	Rational& affine_int_get_elem(size_t row, size_t col) const
	{
		assert(is(EST::rational));
		return p_i()[affine_index(row, col)];
	}

	Real& affine_real_get_elem(size_t row, size_t col) const
	{
		assert(is(EST::real));
		return p_r()[affine_index(row, col)];
	}

	////////////////////////////////////////////////////////////////////////////

	MMatInt MI() const
	{
		assert(is(ETP::matrix, EST::rational));
		return { p_i(), m_ex[0], m_ex[1] };
	}

	MMatBigRational MB() const
	{
		assert(is(ETP::matrix, EST::big_rational));
		return { p_b(), m_ex[0], m_ex[1] };
	}

	MMatReal MR() const
	{
		assert(is(ETP::matrix, EST::real));
		return { p_r(), m_ex[0], m_ex[1] };
	}

	////////////////////////////////////////////////////////////////////////////
	
	MMatInt MatI() const
	{
		assert(is(EST::rational));
		let d = (int)get_affine_dim();
		return { p_i(), d, d };
	}


	MMatBigRational MatB() const
	{
		assert(is(EST::big_rational));
		let d = (int)get_affine_dim();
		return { p_b(), d, d };
	}

	MMatReal MatR() const
	{
		assert(is(EST::real));
		let d = (int)get_affine_dim();
		return { p_r(), d, d };
	}

	////////////////////////////////////////////////////////////////////////////

	MVecInt TrI() const
	{
		assert(is(EST::rational));
		let d = (int)get_affine_dim();
		return { p_i() + d * d, d };
	}

	MVecBigRational TrB() const
	{
		assert(is(EST::big_rational));
		let d = (int)get_affine_dim();
		return { p_b() + d * d, d };
	}


	MVecReal TrR() const
	{
		assert(is(EST::real));
		let d = (int)get_affine_dim();
		return { p_r() + d * d, d };
	}

	MVecReal VecR() const
	{
		assert(is(ETP::vector, EST::real));
		return { p_r(), (int)get_size() };
	}

	////////////////////////////////////////////////////////

	template<typename F>
	void visit_compos(F f) const
	{
		if (!is(ETP::compos)) {
			f(this);
			return;
		}
		auto** a = p_v();
		for (let** ae = a + get_size(); a < ae; ++a) {
			(*a)->visit_compos(f);
		}
	}

	template<typename F>
	void visit_childs(F f) const
	{
		assert(gs()==EST::other);
		auto** a = p_v();
		for (let** ae = a + get_size(); a < ae; ++a) {
			if (!f(*a)) break;
		}
	}

	void set_size(size_t sz)
	{
		assert(is(ETP::string) || is(ETP::vector));
		m_size = static_cast<uint32_t>(sz);
	}

	//for pool only
	ims_val(size_t size, uint8_t bucket, ims_val::ETP t, ims_val::EST s) :
		m_use_count{ 1 },
		m_size{ static_cast<uint32_t>(size) },
		m_reserved{ 0 },
		m_bucket{ bucket },
		m_flags{ 0 },
		m_t{ t },
		m_s{ s } {};

	uint8_t get_bucket() const
	{
		return m_bucket;
	};

	size_t dec_ref() const
	{
		check_ref();
		return --m_use_count;
	};

	void add_ref() const
	{
		++m_use_count;
	}

	void check_ref() const
	{	
		assert(m_use_count < 0xFFFFFF);//something wrong happens
		assert(m_use_count > 0);
	}

	//for .natvis only
	using ETP = ims_val_b::ETP;
	using EST = ims_val_b::EST;
	using Rational = ims_val_b::Rational;
	using Real = ims_val_b::Real;
};


////////////////////////////////////////////////////////////////////////////////
static_assert(sizeof(ims_val) % 8 == 0);//required for data alignment
#ifdef _IMS_64_
static_assert(sizeof(ims_val) <= (2* sizeof(void*)));
#else 
static_assert(sizeof(ims_val) <= (4 * sizeof(void*)));
#endif
