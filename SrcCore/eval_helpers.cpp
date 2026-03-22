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
#include "eval_helpers.h"
#include "matrix_helper.h"
#include "eval_pool.h"
#include "mat_operations.h"
#include "matrix_funcs.h" //charpoly
#include "ims_val.h"
#include "pool_ptr.h"


namespace eval_helpers {


const ims_val* create_ball(
	const Real* c, 
	Real r, 
	size_t dim)
{
	if (!c)return nullptr;
	auto* b = eval_pool::ep.get_vector_real(dim + 1);
	auto* d = b->p_r();
	d[0] = r;
	std::copy(c, c + dim, d + 1);
	return b;
}

const ims_val* mulpow_affine(const ims_val* A, const ims_val* B, uint8_t p)
{
	assert(A->is_affine());
	assert(B->is_affine());

	let dim = A->extent(0);
	assert(B->extent(0) == dim);

	let s = A->gs();
	assert(B->is(s));

	A->add_ref();
	B->add_ref();

	for (;;) {
		if (p & 1) {//A = A*B
			ims_val* nA;
			if (s == ims_val::EST::rational) {
				nA = eval_pool::ep.get_affine_int(dim);
				affine_mul_rational(nA, A, B);
			} else {
				nA = eval_pool::ep.get_affine_real(dim);
				affine_mul_real(nA, A, B);
			}

			eval_pool::ep.release(A);
			A = nA;
		}
		p >>= 1;
		if (!p)break;

		//B=B^2
		ims_val* nB;
		if (s == ims_val::EST::rational) {
			nB = eval_pool::ep.get_affine_int(dim);
			affine_mul_rational(nB, B, B);
		} else {
			nB = eval_pool::ep.get_affine_real(dim);
			affine_mul_real(nB, B, B);
		}

		eval_pool::ep.release(B);
		B = nB;
	}

	eval_pool::ep.release(B);
	return A;
}



const ims_val* to_real3(const ims_val* src)
{
	if (src->is(ims_val::EST::real)) {
		src->add_ref();
		return src;
	}
	if (src->is(ims_val::EST::other)) {
		return nullptr;
	}

	

	//conversion int->real
	if (src->is(ims_val::ETP::number)) {
		Real rv;
		if (src->is(ims_val_b::EST::rational)) {
			let& ival = src->get_int();
			div_to_real(rv, ival.numerator(), ival.denominator());
		} else {
			assert(src->is(ims_val_b::EST::big_rational));
			let& ival = src->get_big_rational();

			
			div_to_real(rv, numerator(ival), denominator(ival));
		}
		return eval_pool::ep.get_scalar_real(rv);
		
	}

	if (src->is_affine()) {
		let n = src->extent(0);
		auto* dst = eval_pool::ep.get_affine_real(n);
		auto* d = dst->p_r();
		let sz = ims_val::affine_num_elems(n);

		if (src->is(ims_val_b::EST::rational)) {
			let* s = src->p_i();
			for (size_t i = 0; i < sz; ++i) {
				let& v = s[i];
				div_to_real(d[i],  v.numerator(), v.denominator());
			}
		} else {
			assert(src->is(ims_val_b::EST::big_rational));
			let* s = src->p_b();
			for (size_t i = 0; i < sz; ++i) {
				let& v = s[i];
				div_to_real(d[i], numerator(v), denominator(v));
			}
		}
		return dst;
	}

	if (src->is(ims_val::ETP::vector)) {
		let n = src->get_size();
		auto* dst = eval_pool::ep.get_vector_real(n);
		auto* d = dst->p_r();

		if (src->is(ims_val_b::EST::rational)) {
			let& s = src->p_i();
			for (size_t i = 0; i < n; ++i) {
				let& v = s[i];
				div_to_real(d[i], v.numerator(), v.denominator());
			}
		} else {
			assert(src->is(ims_val_b::EST::big_rational));
			let& s = src->p_b();
			for (size_t i = 0; i < n; ++i) {
				let& v = s[i];
				div_to_real(d[i], numerator(v), denominator(v));
			}
		}
		return dst;
	}

	return nullptr;
}

const ims_val* affine_charpoly(const ims_val* src)
{
	assert(src->is_affine());

	let n = src->extent(0);

	if (src->is(ims_val::EST::real)) {
		DynMat<Real> A(n, n), B(n, n), T(n, n);
		A = src->MatR();
		auto* ret = eval_pool::ep.get_vector_real(n);
		char_poly(A, n, B, T, ret->p_r());
		return ret;
	}

	//integers - using long arithmetic

	using int_type = big_int_number_boost;
	using big_rational = boost::rational<int_type>;

	DynMat<big_rational> A(n, n), B(n, n), T(n, n);


	let sz = n * n;

	let* sa = src->p_i();
	for (size_t i = 0; i < sz; ++i) {
		let& s = sa[i];
		A(i) = {
			int_type(s.numerator()),
			int_type(s.denominator())
		};
	}

	std::vector<big_rational> poly(n);
	char_poly(A, n, B, T, poly.data());

	using IT = ims_val::Rational::int_type;
	constexpr auto min_val = std::numeric_limits<IT>::lowest();
	constexpr auto max_val = std::numeric_limits<IT>::max();

	//collect the result
	pool_ptr ret(eval_pool::ep.get_vector_int(n));
	auto* dst = ret->p_i();

	for (size_t i = 0; i < n; ++i) {
		let& v = poly[i];
		if (v.numerator() < min_val || v.numerator() > max_val ||
			v.denominator() < min_val || v.denominator() > max_val)
		{
			ims_error("$charpoly overflow");
			return nullptr;//overflow
		}
		dst[i] = {
			static_cast<IT>(v.numerator()),
			static_cast<IT>(v.denominator())
		};
	}

	return ret.release();

}

const ims_val* affine_inv(const ims_val* src)
{
	assert(src->is_affine());

	let dim = src->extent(0);

	if (src->is(ims_val::EST::rational)) {
		pool_ptr T1(eval_pool::ep.get_affine_int(dim));
		pool_ptr dst(eval_pool::ep.get_affine_int(dim));

		auto srcM = src->MatI();
		auto dstM = dst->MatI();
		auto T1M = T1->MatI();
		let d = char_poly(srcM, dim, dstM, T1M);
		if (d == 0) {
			return nullptr;
		}

		//translate
		auto dstT = dst->TrI();
		dstT.noalias() = dstM * src->TrI();


#ifndef NDEBUG
		//TODO: Eigen 5 problem: product of Eigen::Map
		for (size_t i = 0; i < dim; ++i) {
			assert(denominator(dstT[i]) != 0);
		}
#endif

		let q = 1 / d;

		dstT *= -q;
		dstM *= q;

		return dst.release();
	}

	assert(src->is(ims_val::EST::real));

	pool_ptr T1(eval_pool::ep.get_affine_real(dim));
	pool_ptr dst(eval_pool::ep.get_affine_real(dim));

	auto srcM = src->MatR();
	auto dstM = dst->MatR();
	auto T1M = T1->MatR();
	let d = char_poly(srcM, dim, dstM, T1M);

	//translate
	auto dstT = dst->TrR();
	dstT.noalias() = dstM * src->TrR();
	dstT *= -1 / d;

	dstM *= 1 / d;

	return dst.release();
};


//map - canonical
const ims_val* mul_ball(const ims_val* map, const ims_val* b, bool point_mode) 
{
	assert(b->is(ims_val::ETP::vector, ims_val::EST::real));

	pool_ptr sb(b); b->add_ref();

	let** mb = &map;
	let** cm = mb;

	while (cm >= mb) {
		let* m = *cm;

		if (m->is(ims_val::ETP::compos)) {
			assert(cm == mb);//canonicity
			mb = m->p_v();
			cm = mb + m->get_size() - 1;
			continue;
		}

		--cm;

		size_t dim = sb->get_size() - 1;

		if (m->is(ims_val::ETP::number)) {
			assert(m->is(ims_val::EST::real));

			let* db = eval_pool::ep.get_vector_real(dim + 1);

			let* md = m->p_r();
			let* s = sb->p_r();
			auto* d = db->p_r();

			let r = point_mode ? 0 : s[0];

			if (r > 0) {
				d[0] = std::abs(md[0]) * r;
			} else {//including negative radius
				d[0] = r;
			}

			mul_vec_scalar(d + 1, md[0], s + 1, dim);

			sb.reset(db);

		} else if (m->is_affine()) {
			assert(m->is(ims_val::EST::real));

			let mdim = m->extent(0);

			if (dim < mdim) {//transform to a higher dimension
				sb.reset(extend_real_vector_size(sb.get(), mdim + 1));
				dim = mdim;
			}

			let* s = sb->p_r();
			let* db = eval_pool::ep.get_vector_real(dim + 1);
			let* md = m->p_r();

			auto* d = db->p_r();

			let r = point_mode ? 0 : s[0];

			if (r > 0) {
				auto nm = matrix_helper::norm_adet(md, dim);
				if (mdim < dim) {//extend the map by the identity
					nm = std::max(nm, 1.0);
				}
				d[0] = nm * r;
			} else {//including negative radius
				d[0] = r;
			}

			++d;
			++s;

			mul_aff_vec(d, md, s, mdim);

			if (mdim < dim) {
				std::copy(s + mdim, s + dim, d + mdim);
			}

			sb.reset(db);

		} else if (m->is(ims_val::ETP::csg)) {

			let* s = sb->p_r();

			let d = sqrt(vec_norm(s + 1, dim));

			let r = point_mode ? 0 : s[0];

			assert(r >= 0);

			int64_t pw;
			m->p_v()[2]->to_int(pw);

			if ((pw > 0 && d - r > 1) || (pw < 0 && d + r < 1)) {
				return nullptr;//cut off
			}
			//ok
		} else if (!ims_val::is_geom(m->gt())) {
			//ok
		} else {
			assert(false);//not implemented
		}
	};

	return sb.release();
}


const ims_val* try_mulx(
	const ims_val* left,
	const ims_val* right,
	size_t dim
)
{
	if (left->is_empty() || right->is_empty()) {
		return eval_pool::ep.get_empty_val();
	}

	if (right->is_id()) {
		left->add_ref();
		return left;
	}
	if (left->is_id()) {
		right->add_ref();
		return right;
	}

	auto t = left->gt();
	auto s = left->gs();

	t = right->common_affine_type(t, dim);
	s = right->common_subtype(s);

	if (s == ims_val::EST::other) {
		return nullptr;
	}

	if (t == ims_val::ETP::vector) {//don't use vector multiplication
		t = ims_val::ETP::matrix;
	}

	pool_ptr vl(convert_numeric(left, t, s, dim));
	if (!vl)return nullptr;
	pool_ptr vr(convert_numeric(right, t, s, dim));
	if (!vr)return nullptr;

	if (vl->is(ims_val::ETP::number, ims_val::EST::rational)) {
		return eval_pool::ep.get_scalar_int(vl->get_int() * vr->get_int());
	}

	if (vl->is(ims_val::ETP::number, ims_val::EST::real)) {
		return eval_pool::ep.get_scalar_real(vl->get_real() * vr->get_real());
	}

	if (vl->is(ims_val::ETP::matrix, ims_val::EST::rational)) {
		auto* ret = eval_pool::ep.get_affine_int(dim);
		affine_mul_rational(ret, vl.get(), vr.get());
		return ret;
	}

	if (vl->is(ims_val::ETP::matrix, ims_val::EST::real)) {
		auto* ret = eval_pool::ep.get_affine_real(dim);
		affine_mul_real(ret, vl.get(), vr.get());
		return ret;
	}

	return nullptr;
}



const ims_val* mulx(
	const ims_val* left,
	const ims_val* right,
	size_t dim
)
{
	auto* ret = try_mulx(left, right, dim);
	if (ret) return ret;

	auto* v = eval_pool::ep.get_vector(2, ims_val::ETP::compos);
	left->add_ref();
	right->add_ref();
	auto* va = v->p_v();
	va[0] = left;
	va[1] = right;

	return v;
}


static const ims_val* vector_to_affine(const ims_val* src, size_t dim)
{
	if (!src->is(ims_val::ETP::vector) || src->is(ims_val::EST::other)) {
		return nullptr;
	}

	//convert a numerical vector
	let szMat = dim * dim;

	if (src->get_size() == dim) {//translation
		if (src->is(ims_val::EST::rational)) {
			auto* ret = eval_pool::ep.get_affine_int(dim);
			affine_int_set_to(ret, 1);
			auto* d = ret->p_i() + szMat;
			let* s = src->p_i();
			std::copy(s, s + dim, d);
			return ret;
		}
		assert(src->is(ims_val::EST::real));
		{
			auto* ret = eval_pool::ep.get_affine_real(dim);
			affine_real_set_to(ret, 1);
			auto* d = ret->p_r() + szMat;
			let* s = src->p_r();
			std::copy(s, s + dim, d);
			return ret;
		}
	}

	if (src->get_size() != szMat) {
		return nullptr;
	}

	//matrix
	if (src->is(ims_val::EST::rational)) {
		auto* ret = eval_pool::ep.get_affine_int(dim);
		auto* d = ret->p_i();
		let* s = src->p_i();

		//row major -> column major
		for (size_t r = 0; r < dim; ++r) {
			for (size_t c = 0; c < dim; ++c) {
				d[r + c * dim] = *s++;
			}
		}

		d += szMat;
		std::fill(d, d + dim, 0);

		return ret;
	}

	assert(src->is(ims_val::EST::real));
	{
		auto* ret = eval_pool::ep.get_affine_real(dim);
		auto* d = ret->p_r();
		let* s = src->p_r();

		//row major -> column major
		for (size_t r = 0; r < dim; ++r) {
			for (size_t c = 0; c < dim; ++c) {
				d[r + c * dim] = *s++;
			}
		}

		d += szMat;
		std::fill(d, d + dim, 0);

		return ret;
	}

}

const ims_val* to_affine_or_scalar(const ims_val* src, size_t dim)
{
	if (src->is_affine() || src->is(ims_val::ETP::number)) {
		src->add_ref();
		return src;
	}

	return vector_to_affine(src, dim);
}

const ims_val* to_affine3(const ims_val* src, size_t dim)
{
	if (!src)return nullptr;

	if (src->is_affine()) {
		src->add_ref();
		return src;
	}

	if (src->is_id()) {
		ims_val* dst = eval_pool::ep.get_affine_int(dim);
		affine_int_set_to(dst, 1);
		return dst;
	}

	if (src->is(ims_val::ETP::number, ims_val::EST::rational))
	{
		ims_val* dst = eval_pool::ep.get_affine_int(dim);
		affine_int_set_to(dst, src->get_int());
		return dst;
	}

	if (src->is(ims_val::ETP::number, ims_val::EST::real))
	{
		ims_val* dst = eval_pool::ep.get_affine_real(dim);
		affine_real_set_to(dst, src->get_real());
		return dst;
	}

	return vector_to_affine(src, dim);
}

const ims_val* convert_numeric(
	const ims_val* src,
	ims_val::ETP t,
	ims_val::EST s,
	size_t dim)
{
	if (src->gs()>= ims_val::EST::nan || s < src->gs() || s>= ims_val::EST::nan) {
		return nullptr;
	}

	src->add_ref();

	if (!src->is(s)) {
		assert(s == ims_val::EST::real);

		//replace src
		let* nsrc = eval_helpers::to_real3(src);
		eval_pool::ep.release(src);
		src = nsrc;
	}

	assert(src->is(s));

	if (src->is(t)) {
		return src;
	}

	if (dim == 0) {
		return nullptr;
	}

	//convert to affine
	assert(t == ims_val::ETP::matrix);

	let* ret = to_affine3(src, dim);
	eval_pool::ep.release(src);
	return ret;
}



const ims_val*
sum_affine(const ims_val* left, const ims_val* right, size_t dim)
{
	auto t = left->gt();
	auto s = left->gs();

	t = right->common_affine_type(t, dim);
	s = right->common_subtype(s);

	if (t == ims_val::ETP::vector) {//do not use vector addition
		t = ims_val::ETP::matrix;
	}

	if (s == ims_val::EST::other) {
		return nullptr;
	}

	pool_ptr vl(convert_numeric(left, t, s, dim));
	if (!vl)return nullptr;
	pool_ptr vr(convert_numeric(right, t, s, dim));
	if (!vr)return nullptr;

	if (vl->is(ims_val::ETP::number, ims_val::EST::rational)) {
		return eval_pool::ep.get_scalar_int(vl->get_int() + vr->get_int());
	}

	if (vl->is(ims_val::ETP::number, ims_val::EST::real)) {
		return eval_pool::ep.get_scalar_real(vl->get_real() + vr->get_real());
	}

	if (vl->is(ims_val::ETP::matrix, ims_val::EST::rational)) {
		auto* ret = eval_pool::ep.get_affine_int(dim);
		let n = ims_val::affine_num_elems(dim);
		add_vec(ret->p_i(), vl->p_i(), vr->p_i(), n);
		return ret;
	}

	if (vl->is(ims_val::ETP::matrix, ims_val::EST::real)) {
		auto* ret = eval_pool::ep.get_affine_real(dim);
		let n = ims_val::affine_num_elems(dim);
		add_vec(ret->p_r(), vl->p_r(), vr->p_r(), n);
		return ret;
	}

	return nullptr;
}

template<typename Number>
void eval_mod_t(Number& res, const Number& lv, const Number& rv)
{
	assert(rv != 0);
	// (a/b) mod (c/d) = a/b - (c/d) * floor((a*d) / (b*c))
	using IT = ims_get<Number>::int_type;
	IT ad = numerator(lv) * denominator(rv);
	IT bc = denominator(lv) * numerator(rv);
	IT q = ad / bc;
	// adjust truncation toward negative infinity (floor division)
	if ((ad < 0) != (bc < 0) && q * bc != ad) --q;
	res = lv - Number(q) * rv;
}

const ims_val* eval_mod(const ims_val* left, const ims_val* right)
{
	if (!right->is(ims_val_b::ETP::number)) {
		return nullptr;
	}

	//todo: support vectors and matrices
	if (!left->is(ims_val_b::ETP::number)) {
		return nullptr;
	}

	let st = left->common_subtype(right->gs());
	switch (st) {

	case ims_val::EST::rational: {
		let& lv = left->get_int();
		let& rv = right->get_int();
		if (rv == 0) {
			left->add_ref();
			return left;
		}
		Rational res;
		eval_mod_t(res, lv, rv);
		return eval_pool::ep.get_scalar_int(res);
	}
	case ims_val::EST::big_rational: {
		using BigRational = ims_val_b::BigRational;
		BigRational lv, rv;
		left->to_big_rational(lv);
		right->to_big_rational(rv);
		auto* res = eval_pool::ep.get_scalar_big_rational();
		eval_mod_t(*res->p_b(), lv, rv);
		return res;
	}
	case ims_val::EST::real: {
		ims_val_b::Real lv, rv;
		if (!left->to_real(lv) || !right->to_real(rv)) {
			return nullptr;
		}
		if (rv == 0.0) {
			left->add_ref();
			return left;
		}
		return eval_pool::ep.get_scalar_real(lv - rv * floor(lv/rv));
	}
	default:
		return nullptr;
	}
}

const ims_val* edge_mul(const ims_val* A, const ims_val* B, bool geom_only)
{
	//special case - optimization
	if (A->is_affine() && B->is_affine() && A->extent(0) == B->extent(0)) {
		auto* ret = eval_pool::ep.get_affine_real(A->extent(0));
		affine_mul_real(ret, A, B);
		return ret;
	}

	const ims_val* const* sb;
	const ims_val* const* sbe;

	if (B->is(ims_val::ETP::compos)) {
		sb = B->p_v();
		sbe = sb + B->get_size();
	} else {
		sb = &B;
		sbe = sb + 1;
	}


	let nb = static_cast<size_t>(sbe - sb);//how many was originally in B

	if (geom_only) {
		//remove the beginning from the identity maps
		for (; sb < sbe; ++sb) {
			if (ims_val::is_geom((*sb)->gt())) {
				break;
			}
		}

		//remove the end from the identity maps
		for (; sbe > sb; --sbe) {
			if (ims_val::is_geom((*(sbe - 1))->gt())) {
				break;
			}
		}
	}

	if (sb == sbe) {//B is identity
		A->add_ref();
		return A;
	}


	const ims_val* const* sa;
	size_t na;
	if (A->is(ims_val::ETP::compos)) {
		na = A->get_size();
		if (na == 0) {//A is identity

			if (!geom_only) {
				B->add_ref();
				return B;
			}

			//from previous calculations
			assert(sb < sbe);
			assert(ims_val::is_geom((*sb)->gt()));

			if (sb + 1 == sbe) {//on the right there is only one geometric
				(*sb)->add_ref();
				return (*sb);
			}

			size_t num_geom_in_B = 0;

			for (let* s = sb; s < sbe; ++s) {
				if (ims_val::is_geom((*s)->gt())) {
					++num_geom_in_B;
				}
			}

			assert(num_geom_in_B >= 2);//from previous calculations

			if (num_geom_in_B == nb) {//B consisted only of geometric maps
				B->add_ref();
				return B;
			}

			//leave only geometric maps
			auto* ret = eval_pool::ep.get_vector(num_geom_in_B, ims_val::ETP::compos);
			auto* vec = ret->p_v();


			for (; sb < sbe; ++sb) {
				if (!ims_val::is_geom((*sb)->gt())) {
					continue;
				}
				*vec++ = (*sb);
				(*sb)->add_ref();

			}
			//ret->shrink(vec - ret->p_v());
			return ret;
		}
		sa = A->p_v();
	} else {
		na = 1;
		sa = &A;
	}


	let* AX = sa[na - 1];
	let* B0 = sb[0];

	//canonicity condition
	assert(!AX->is(ims_val::ETP::compos) && !B0->is(ims_val::ETP::compos));

	let* AB = collapse_compos(AX, B0);

	pool_ptr ret;
	const ims_val** vec;

	if (AB) {
		++sb;//eaten

		let num = na + sbe - sb;

		assert(num >= 1);

		if (num == 1) {
			return AB;
		}
		ret = eval_pool::ep.get_vector(num, ims_val::ETP::compos);
		vec = ret->p_v();

		//copy the left part without the last one
		for (size_t i = 0; i < na - 1; ++i) {
			*vec++ = sa[i];
			sa[i]->add_ref();
		};

		*vec++ = AB;//transferred ownership
		
	} else {
		ret = eval_pool::ep.get_vector(1 + sbe - sb, ims_val::ETP::compos);
		vec = ret->p_v();
		*vec++ = A; A->add_ref();
	}


	for (; sb < sbe; ++sb) {

		if (geom_only && !ims_val::is_geom((*sb)->gt())) {
			continue;
		}
		*vec++ = *sb;
		(*sb)->add_ref();
	}

	ret.get_mut()->shrink(vec - ret->p_v());

	return ret.release();

}


const ims_val* collapse_compos(const ims_val* A, const ims_val* B)
{
	let bt = B->gt();

	if (bt == ims_val::ETP::matrix) {
		assert(B->is_affine());
		let dim = B->extent(0);

		pool_ptr af;

		if (A->is(ims_val::ETP::matrix)) {
			assert(A->is_affine());
			if (A->extent(0) != dim) {
				return nullptr;
			}
			af = A;
			A->add_ref();
		} else if (A->is(ims_val::ETP::number)) {
			af = eval_helpers::to_affine3(A, dim);
		}
		if (af) {
			auto* ret = eval_pool::ep.get_affine_real(dim);
			affine_mul_real(ret, af.get(), B);
			return ret;
		}
	} else if (bt == ims_val::ETP::number) {
		if (A->is(ims_val::ETP::matrix)) {
			assert(A->is_affine());
			let dim = A->extent(0);
			pool_ptr bf(eval_helpers::to_affine3(B, dim));
			auto* ret = eval_pool::ep.get_affine_real(dim);
			affine_mul_real(ret, A, bf.get());
			return ret;
		}

		if (A->is(ims_val::ETP::number)) {
			auto* ret = eval_pool::ep.get_scalar_real(
				A->get_real() * B->get_real());
			return ret;
		}
	}
	return nullptr;
}

const ims_val* real_affine_increase_dim(const ims_val* v, size_t dim)
{
	assert(v->is(ims_val::ETP::matrix, ims_val_b::EST::real));
	assert(v->is_affine());
	let n = v->extent(0);
	assert(n < dim);

	auto* nv = eval_pool::ep.get_affine_real(dim);

	affine_real_set_to(nv, 1);

	let* s = v->p_r();
	auto* d = nv->p_r();

	//copy the matrix
	for (size_t c = 0; c < n; ++c) {
		for (size_t r = 0; r < n; ++r) {
			*d++ = *s++;
		}
		d += dim - n;
	}

	d += (dim - n) * dim;

	//copy the translation
	for (size_t r = 0; r < n; ++r) {
		*d++ = *s++;
	}

	return nv;
	
}

void edge_append(const ims_val** A, size_t& sz, const ims_val* B)
{
	if (B->is_id()) {
		return;
	}

	assert(!B->is(ims_val::ETP::compos));

	if (sz > 0) {
		let* a = A[sz - 1];
		assert(!a->is(ims_val::ETP::compos));

		let* ret = collapse_compos(a, B);

		if (ret) {
			eval_pool::ep.release(a);
			A[sz - 1] = ret;
			return;
		}
	}

	A[sz++] = B;
	B->add_ref();
}

const ims_val* extend_real_vector_size(const ims_val* v, size_t new_size)
{
	assert(v->is(ims_val::ETP::vector, ims_val::EST::real));
	let sz = v->get_size();
	assert(sz < new_size);
	let* dst = eval_pool::ep.get_vector_real(new_size);
	let* s = v->p_r();
	auto* d = dst->p_r();
	std::copy(s, s + sz, d);
	std::fill(d + sz, d + new_size, 0);//extend with zeros
	return dst;
}

const ims_val* fixed_point(const ims_val* m)
{
	if (m->is(ims_val::ETP::matrix)) {
		assert(m->is_affine());
		assert(m->is(ims_val::EST::real));

		let dim = m->extent(0);
		pool_ptr bt (eval_pool::ep.get_vector_real(dim + 1));
		auto* d = bt->p_r();
		d[0] = 0;//radius
		
		return matrix_helper::calc_fixed_point(
			d + 1, m->p_r(), dim) ?
			bt.release() :
			nullptr;
	}

	if (m->is(ims_val::ETP::number)) {
		assert(m->is(ims_val::EST::real));

		pool_ptr bt(eval_pool::ep.get_vector_real(1));
		bt->p_r()[0] = 0;//radius
		return bt.release();
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
void to_affine_buffer(const ims_val* pv, Real* d)
{
	assert(pv->is_affine());
	
	let sz = ims_val::affine_num_elems(pv->extent(0));

	if (pv->is(ims_val::EST::rational)) {
		let* m = pv->p_i();
		let* me = m + sz;

		while (m < me) {
			div_to_real(*d++, m->numerator(), m->denominator());
			m++;
		}

	} else {
		let* m = pv->p_r();
		let* me = m + sz;

		while (m < me) {
			*d++ = *m++;
		}
	};
}


void to_proj_integer(const ims_val* pv, DynMat<int64_t>& dst)
{
	assert(pv->is_affine() && pv->is(ims_val::EST::rational));

	let n = pv->extent(0);

	let num = n * n;//do not use translation

	int64_t lcm(1);

	let* ptr = pv->p_i();
	for (size_t i = 0; i < num; ++i) {
		lcm = boost::integer::lcm(lcm, ptr[i].denominator());
	}

	///////////////////////////////
	dst.resize(n, n);

	auto* d = dst.data();

	for (size_t i = 0; i < num; ++i) {
		let v = ptr[i] * lcm;
		assert(v.denominator() == 1);
		d[i] = v.numerator();
	}

	arr_to_normal_form(d, num);
}

const ims_val* to_ext_real(const ims_val* v, size_t n) 
{	
	assert(v->is(ims_val::EST::real));
	let* ret = eval_pool::ep.get_matrix_real(n + 1, n + 1);
	auto dst = ret->MR();
	
	
	if (v->is(ims_val::ETP::matrix)) {
		assert(v->is_affine());
		assert(v->extent(0) == n);

		let* ptr = v->p_r();

		for (size_t r = 0; r < n; ++r) {
			for (size_t c = 0; c < n + 1; ++c) {
				dst(r, c) = ptr[r + c * n];
			}
		}
		//the last line
		for (size_t r = 0; r < n; ++r) {
			dst(n, r) = 0;
		}

	} else {
		assert(v->is(ims_val::ETP::number));

		dst.setZero();
		let d = v->get_real();

		for (size_t i = 0; i < n; ++i) {
			dst(i, i) = d;
		}
	}

	dst(n, n) = 1;

	return ret;
}

void to_ext_integer(const ims_val* pv, DynMat<int64_t>& dst)
{
	assert(pv->is_affine() && pv->is(ims_val::EST::rational));
	
	let n = pv->extent(0);

	dst.resize(n + 1, n + 1);

	let* ptr = pv->p_i();

	int64_t lcm(1);
	let num = ims_val::affine_num_elems(n);
	for (size_t i = 0; i < num; ++i) {
		lcm = boost::integer::lcm(lcm, ptr[i].denominator());
	}

	///////////////////////////////
	

	for (size_t r = 0; r < n; ++r) {
		for (size_t c = 0; c < n + 1; ++c) {
			auto v = ptr[r + c * n] * lcm;
			assert(v.denominator() == 1);
			dst(r, c) = v.numerator();
		}
	}
	for (size_t r = 0; r < n; ++r) {
		dst(n, r) = 0;
	}
	dst(n, n) = lcm;
}


//set the matrix with element v on the diagonal
template<typename T>
void affine_set_to(const ims_val* dst, T v) {

	assert(dst->is_affine());

	let n = dst->extent(0);
	let sz = ims_val::affine_num_elems(n);

	auto* d = dst->gp<T>();

	for (size_t i = 0; i < sz; ++i) {
		d[i] = 0;
	}
	if (v == 0)return;

	//set the matrix diagonal
	for (size_t i = 0; i < sz; i += (n + 1)) {
		d[i] = v;
	}
}

void affine_real_set_to(const ims_val* dst, Real v)
{
	assert(dst->is_affine() && dst->is(ims_val::EST::real));
	affine_set_to<Real>(dst, v);
}


void affine_int_set_to(const ims_val* dst, Rational v)
{
	assert(dst->is_affine() && dst->is(ims_val::EST::rational));
	affine_set_to<Rational>(dst, v);
}

template<typename T>
void affine_mul_t(const ims_val* dst, const ims_val* left, const ims_val* right)
{
	assert(dst->is_affine());
	assert(left->is_affine());
	assert(right->is_affine());

	assert(dst->is(ims_val::get_subtype<T>()));
	assert(left->is(dst->gs()));
	assert(right->is(dst->gs()));

	let n = dst->extent(0);
	assert(n == left->extent(0));
	assert(n == right->extent(0));
	let sm = n * n;

	auto* dp = dst->gp<T>();
	auto* lp = left->gp<T>();
	auto* rp = right->gp<T>();

	mul_mat_mat(dp, lp, rp, n);
	mul_mat_vec(dp + sm, lp, rp + sm, n);
	add_vec(dp + sm, dp + sm, lp + sm, n);
}

void affine_mul_rational(const ims_val* dst, const ims_val* left, const ims_val* right)
{
	affine_mul_t<Rational>(dst, left, right);
}

void affine_mul_real(const ims_val* dst, const ims_val* left, const ims_val* right)
{
	affine_mul_t<Real>(dst, left, right);
}

Real norm_adet(const ims_val* m, Real* adet)
{
	if (m->is(ims_val::ETP::matrix)) {
		assert(m->is_affine());
		assert(m->is(ims_val::EST::real));
		return matrix_helper::norm_adet(m->p_r(), m->extent(0), adet);
	}
	if (m->is(ims_val::ETP::number)) {
		assert(m->is(ims_val::EST::real));
		let ret = std::abs(m->get_real());
		if (adet)*adet = ret;
		return ret;
	}

	assert(m->is_id());
	if (adet)*adet = 1.0;
	return 1.0;
}

const ims_val* flat(const ims_val* a)
{
	if (a->is(ims_val_b::ETP::matrix)) {
		let r = a->rows();
		let c = a->cols();
		pool_ptr ret(eval_pool::ep.get_vector(c));
		for (size_t i = 0; i < c; ++i) {
			const ims_val* v = nullptr;
			switch (a->gs()) {
			case ims_val_b::EST::rational:
				v = eval_pool::ep.get_vector_int(r);
				for (size_t j = 0; j < r; ++j) {
					v->p_i()[j] = a->affine_int_get_elem(j, i);
				}
				break;
			case ims_val_b::EST::big_rational:
				v = eval_pool::ep.get_vector_big_rational(r);
				for (size_t j = 0; j < r; ++j) {
					v->p_b()[j] = a->affine_big_rational_get_elem(j, i);
				}
				break;
			case ims_val_b::EST::real:
				v = eval_pool::ep.get_vector_real(r);
				for (size_t j = 0; j < r; ++j) {
					v->p_r()[j] = a->affine_real_get_elem(j, i);
				}
				break;
			default:
				v = nullptr;
			}
			ret->p_v()[i] = v;
		}
		return ret.release();
	}

	if (!a->is(ims_val_b::ETP::vector, ims_val_b::EST::other)) {
		a->add_ref();
		return a;
	}

	let sz = a->get_size();

	size_t new_sz = 0;
	for (size_t i = 0; i < sz; ++i) {
		let* vi = a->p_v(i);
		new_sz += vi && vi->is(ims_val_b::ETP::vector) ? vi->get_size() : 1;
	}
	pool_ptr ret(eval_pool::ep.get_vector(new_sz));
	auto* dst = ret->p_v();
	size_t idx = 0;
	for (size_t i = 0; i < sz; ++i) {
		let* vi = a->p_v(i);
		if (!vi) {
			dst[idx++] = vi;
			continue;
		}
		if (vi->is(ims_val_b::ETP::matrix)) {
			dst[idx++] = flat(vi);
			continue;
		}
		if (!vi->is(ims_val_b::ETP::vector)) {
			dst[idx++] = vi;
			if (vi)vi->add_ref();
			continue;
		}
		let visz = vi->get_size();
		if (vi->is(ims_val_b::EST::other)) {
			for (size_t j = 0; j < visz; ++j) {
				let* vij = vi->p_v(j);
				dst[idx++] = vij;
				if (vij)vij->add_ref();
			}
		} else if (vi->is(ims_val_b::EST::rational)) {
			for (size_t j = 0; j < visz; ++j) {
				dst[idx++] = eval_pool::ep.get_scalar_int(vi->p_i(j));
			}
		} else if (vi->is(ims_val_b::EST::real)) {
			for (size_t j = 0; j < visz; ++j) {
				dst[idx++] = eval_pool::ep.get_scalar_real(vi->p_r(j));
			}
		} else if (vi->is(ims_val_b::EST::big_rational)) {
			for (size_t j = 0; j < visz; ++j) {
				auto* b = eval_pool::ep.get_scalar_big_rational();
				*b->p_b() = vi->p_b(j);
				dst[idx++] = b;
			}
		}
	}
	assert(idx == new_sz);
	return eval_pool::ep.adjust_vec_type(ret.get());
}

}//namespace eval_helpers