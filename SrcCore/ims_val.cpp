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
	size_t sz = 1;
	switch (gt()) {
	case ETP::number:
		break;
	case ETP::vector:
		sz = get_size();
		break;
	case ETP::uni:
		assert(get_size() == 0);//empty set
		return false;
	case ETP::compos:
		assert(get_size() == 0);//empty set
		return true;
	case ETP::string:
		return get_size()>0;
	default:
		return true;
	};

	for (size_t i = 0; i < sz; ++i) {
		switch (gs())
		{
		case EST::rational:
			if (numerator(p_i(i)) <= 0)return false;
			break;
		case EST::big_rational:
			if (numerator(p_b(i)) <= 0)return false;
			break;
		case EST::real:
			if (p_r(i) <= 0)return false;
			break;
		case EST::other:
			if (!p_v(i) || !p_v(i)->is_true()) {
				return false;
			}
			break;
		default:
			break;
		}
	}
	return true;
}

int ims_val::compare(const ims_val* a, const ims_val* b)
{

	if (a == b) {
		return 0;
	}
	if (!a)return -1;
	if (!b)return 1;

	if (a->gt() != b->gt()) {
		return a->gt() < b->gt() ? -1 : 1;
	}

	let na = a->num_el();
	let nb = b->num_el();
	if (na != nb) {
		return na < nb ? -1 : 1;
	}

	if (a->gt() == ETP::string) {
		int res = a->get_string().compare(b->get_string());
		if (res == 0)return res;
		return res > 0 ? 1 : -1;
	}

	if (na == 1 && a->gs() > EST::nan) {
		return a < b ? -1 : 1;//compare by pointers
	}

	if (a->gs() == EST::other || b->gs() == EST::other) {
		if (a->gs() != b->gs()) {
			return a->gs() == EST::other ? 1 : -1;
		}
		for (size_t i = 0; i < na; ++i) {
			int res = compare(a->p_v(i), b->p_v(i));
			if (res != 0)return res;
		}
		return 0;
	}

	let s = a->common_subtype(b->gs());
	for (size_t i = 0; i < na; ++i) {
		switch (s)
		{
		case EST::rational:
		{
			let d = a->p_i(i) - b->p_i(i);
			if (d != 0) {
				return d < 0 ? -1 : 1;
			}
			break;
		}
		case EST::big_rational:
		{
			BigRational xa{}, xb{};
			a->to_big_rational(xa);
			b->to_big_rational(xb);
			let d = xa - xb;
			if (d != 0) {
				return d < 0 ? -1 : 1;
			}
			break;
		}
		case ims_val_b::EST::real:
		{
			Real ra{}, rb{};
			a->to_real(ra);
			b->to_real(rb);
			let d = ra - rb;
			if (d != 0) {
				return d < 0 ? -1 : 1;
			}
			break;
		}
		default:
			assert(false);
			break;
		}
	}
	return 0;
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


static void ensure_indexable(
	[[maybe_unused]] const ims_val* v,
	[[maybe_unused]] size_t idx)
{
	//it should be checked outside
	assert(v->is(ims_val_b::ETP::ptr) || v->is(ims_val_b::ETP::number) || v->is(ims_val_b::ETP::vector));
	assert(idx < (v->is(ims_val_b::ETP::vector) ? v->get_size() : 1));
}

bool ims_val::set_i64(int64_t src, size_t idx)
{
	ensure_indexable(this, idx);

	switch (gs()) {
	case ims_val_b::EST::rational:
	{
		ps_i()[idx] = src;
		return true;
	}
	case ims_val_b::EST::real:
	{
		let dv = static_cast<ims_val_b::Real>(src);
		if (src != static_cast<int64_t>(dv)) {
			return false;
		}
		ps_r()[idx] = dv;
		return true;
	}
	case ims_val_b::EST::big_rational:
	{
		ps_b()[idx] = src;
		return true;
	}
	case ims_val_b::EST::other:
	{
		auto* q = gp<ims_val*>();
		auto* v = q[idx];
		if (v && v->is(ims_val_b::ETP::number) && v->set_i64(src)) {
			return true;
		}
		return false;
	}
	default:
	{
		return false;
	}
	}
}

bool ims_val::get_i64(int64_t& dst, size_t idx) const
{
	ensure_indexable(this, idx);

	switch (gs()) {
		case ims_val_b::EST::rational:
		{
			let& di = ps_i()[idx];
			if (denominator(di) != 1)return false;
			dst = numerator(di);
			return true;
		}
		case ims_val_b::EST::real:
		{
			let v = ps_r()[idx];
			let iv = static_cast<int64_t>(v);
			if (v != iv) {
				return false;
			}
			dst = iv;
			return true;
		}
		case ims_val_b::EST::big_rational:
		{
			let& di = ps_b()[idx];
			if (denominator(di) != 1)return false;
			let& n = numerator(di);

			static constexpr auto min_val = std::numeric_limits<int64_t>::min();
			static constexpr auto max_val = std::numeric_limits<int64_t>::max();
			if (n < min_val || n > max_val) {
				return false;
			}
			dst = n.convert_to<int64_t>();
			return true;
		}
		case ims_val_b::EST::other:
		{
			let* v = p_v()[idx];
			if (v && v->is(ims_val_b::ETP::number)) {
				return v->get_i64(dst);
			}
			return false;
		}
		default:
		{
			return false;
		}
	}
}


bool ims_val::get_f64(double& dst, size_t idx) const
{
	ensure_indexable(this, idx);

	switch (gs()) {
	case ims_val_b::EST::rational:
	{
		dst = boost::rational_cast<double>(ps_i()[idx]);
		return true;
	}
	case ims_val_b::EST::real:
	{
		dst = ps_r()[idx];
		return true;
	}
	case ims_val_b::EST::big_rational:
	{
		dst = ps_b()[idx].convert_to<double>();
		return true;
	}
	case ims_val_b::EST::other:
	{
		let* v = p_v()[idx];
		if (v && v->is(ims_val_b::ETP::number)) {
			return v->get_f64(dst);
		}
		return false;
	}
	default:
	{
		return false;
	}
	}
}

bool ims_val::set_f64(double src, size_t idx)
{
	ensure_indexable(this, idx);

	switch (gs()) {
	case ims_val_b::EST::rational:
	{
		let iv = static_cast<int64_t>(src);
		if (static_cast<double>(iv) != src) {
			return false;
		}
		ps_i()[idx] = iv;
		return true;
	}
	case ims_val_b::EST::real:
	{
		ps_r()[idx] = src;
		return true;
	}
	case ims_val_b::EST::big_rational:
	{
		let iv = static_cast<ims_integer_big>(src);
		if (static_cast<double>(iv) != src) {
			return false;
		}
		ps_b()[idx] = iv;
		return true;
	}
	case ims_val_b::EST::other:
	{
		auto* v = gp<ims_val*>()[idx];
		if (v && v->is(ims_val_b::ETP::number)) {
			return v->set_f64(src);
		}
		return false;
	}
	default:
	{
		return false;
	}
	}
}
bool ims_val::to_real(Real& v) const
{
	if (!is(ETP::number))return false;
	return get_f64(v);
}

ims_val::Real ims_val::get_real() const
{
	return get_v<EST::real>();
}

ims_val::Rational ims_val::get_int() const
{
	return get_v<EST::rational>();
}

ims_val::BigRational& ims_val::get_big_rational() const
{
	return get_v<EST::big_rational>();
}
