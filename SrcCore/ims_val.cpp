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
#include "ims_val.h"


size_t ims_val::vec_length() const
{
	return is(ETP::vector) ? get_size() : 0;
}

size_t ims_val::num_vec_length() const
{
	return is(EST::other) ? 0 : vec_length();
}

bool ims_val::is_true() const
{
	switch (gt()) {
	case ETP::number:
		switch (gs())
		{
		case EST::rational:
			return get_int().numerator() > 0;
		case EST::real:
			return get_real() > 0;
		default:
			return true;
		}
	case ETP::uni:
		assert(get_size() == 0);//empty set
		return false;
	default:
		return true;
	}
}

bool ims_val::to_big_rational(ims_val_b::BigRational& v) const
{
	switch (gs())
	{
	case EST::rational:
	{
		let& rv = get_int();
		v = ims_val_b::BigRational(rv.numerator(), rv.denominator());
		return true;
	}
	case EST::big_rational:
		v = get_big_rational();
		return true;
	default:
		return false;
		break;
	}
}


bool ims_val::to_int(int64_t& v) const
{
	if (!is(ETP::number, EST::rational)) return false;
	
	let& d = get_int();
	if (d.denominator() != 1) {
		return false;
	}
	v = d.numerator();
	return true;
}

bool ims_val::to_real(Real& v) const
{
	if (!is(ETP::number))return false;

	switch (gs()) {
	case EST::rational:
	{
		let& ival = get_int();
		v = static_cast<Real>(ival.numerator()) /
			static_cast<Real>(ival.denominator());
		return true;
	}
	case EST::big_rational:
	{
		let& bval = get_big_rational();
		v = bval.convert_to<double>();
		return true;
	}
	case EST::real:
		v = get_real();
		return true;
	default:
		assert(false);
		return false;
	}
}

ims_val::ETP ims_val::common_affine_type(ETP t, size_t dim) const
{
	if (is(t)) {
		switch (t) {
		case ETP::number:
			return ETP::number;
		case ETP::vector:
			if (get_size() !=  dim * dim) {//cannot be converted to a matrix
				return ETP::vector;
			}
			break;
		default:
			break;
		}
	}

	//this is the best guess from the numeric types
	return ETP::matrix;
}

ims_val::EST ims_val::common_subtype(EST t) const
{
	return std::max(t, gs());
}


const ims_val* ims_val::elevate_compos() const
{
	assert(is(ims_val::ETP::compos));
	let sz = get_size();
	if (sz != 1)return nullptr;

	assert(sz == 1);
	let* ret = p_v()[0];
	ret->add_ref();
	return ret;
}

bool ims_val::is_normal() const
{
	if (!is(ETP::matrix, EST::real))return true;
	let sz = extent(0) * extent(1);
	for (size_t i = 0; i < sz; ++i) {
		let v = p_r()[i];
		if (v > 0 && v < std::numeric_limits<Real>::min()) {
			return false;
		}
		//if (std::fpclassify() != FP_SUBNORMAL) {
	}
	return true;
}

size_t ims_val::num_el() const
{
	switch (m_t)
	{
	case ETP::number:
	case ETP::ast_ptr:
	case ETP::string:
	case ETP::style2:
	case ETP::thickness:
		return 1;
	case ETP::vector:
	case ETP::compos:
	case ETP::uni:
	case ETP::csg:
		return get_size();
	case ETP::matrix:
		return size_t(m_ex[0]) * size_t(m_ex[1]);
	case ETP::mobius:
	case ETP::attractor:
	case ETP::inversion:
		return 0;
	default:
		return 0;
	}
}


ims_val::Real ims_val::get_real() const
{
	assert(is(EST::real));
	return *p_r();
}

ims_val::Rational ims_val::get_int() const
{
	assert(is(EST::rational));
	return *p_i();
}

ims_val::BigRational& ims_val::get_big_rational() const
{
	assert(is(EST::big_rational));
	return *p_b();
}



