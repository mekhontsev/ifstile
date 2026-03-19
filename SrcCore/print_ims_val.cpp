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
#include "oper_block.h"
#include "ifs_list.h"
#include "ims_keywords.h"

template<typename Number>
void print_number(std::ostream& str, const boost::rational<Number>& v)
{
	if (v.denominator() == 1) {
		str << v.numerator();
	} else {
		str << v;
	}
}

template<typename Number>
void print_number(std::ostream& str, const Number& v)
{
	str << v;
}

template<typename Number>
void print_vector_as_aifs(std::ostream& str, const Number* m, size_t dim)
{

	str << "[";
	for (size_t r = 0; r < dim; ++r) {
		print_number(str, m[r]);
		if (r + 1 < dim)str << ",";
	}
	str << "]";

}

template<typename Number>
void print_affine_as_aifs(std::ostream& str, const Number* m, size_t dim)
{
	auto e = [&](size_t r, size_t c)
	{
		return m[r + c * dim];
	};

	bool has_translate = false;
	for (size_t r = 0; r < dim; ++r) {
		if (e(r, dim) != 0) {
			has_translate = true;
			break;
		}
	}

	if (has_translate) {
		str << "[";
		for (size_t r = 0; r < dim; ++r) {
			print_number(str, e(r, dim));
			if (r + 1 < dim)str << ",";
		}
		str << "]";
	}

	bool mat_is_scalar = true;//equivalent to the number m(0,0)
	let& vn = e(0, 0);//implied scalar

	for (size_t r = 0; r < dim; ++r) {
		if (!mat_is_scalar)break;
		for (size_t c = 0; c < dim; ++c) {
			let& v = e(r, c);
			if ((r == c && v != vn) || (r != c && v != 0)) {
				mat_is_scalar = false;
				break;
			}
		}
	}
	if (mat_is_scalar) {
		if (has_translate && vn == 1) {
			return;
		}
		if (has_translate) {
			str << "*";
		}
		print_number(str, vn);
		return;
	}

	if (has_translate)str << "*";

	str << "[" << std::endl;
	for (size_t r = 0; r < dim; ++r) {
		for (size_t c = 0; c  < dim; ++c) {
			let& v = e(r, c);
			if (v >= 0)str << ' ';
			print_number(str, v);
			if (r + 1 < dim || c + 1 < dim)str << ",";
		}
		str << std::endl;
	}
	str << "]";
};

void print_operator(
	const ifs_list& lst,
	std::ostream& str,
	const operator_ptr& ptr,
	const ETYPE par_type,
	const char* fmt
);
void print_ims_val_ex(std::ostream& str, const ims_val* v, const ifs_list* lst)
{
	if (!v) {
		str << "invalid";
		return;
	}

	switch (v->gt()) {
	case ims_val::ETP::number:
		switch (v->gs()) {
		case ims_val::EST::rational: {
			let& ival = v->get_int();
			str << ival.numerator();
			let& d = ival.denominator();
			if (d != 1)str << "/" << d;
			return;
		}
		case ims_val::EST::real:
			str << v->get_real();
			return;
		default:
			break;
		}
		break;
	case ims_val::ETP::matrix:
		if (v->is_affine()) {
			switch (v->gs()) {
			case ims_val::EST::rational:
				print_affine_as_aifs(str, v->p_i(), v->extent(0));
				return;
			case ims_val::EST::real:
				print_affine_as_aifs(str, v->p_r(), v->extent(0));
				return;
			case ims_val::EST::big_rational:
				print_affine_as_aifs(str, v->p_b(), v->extent(0));
				return;
			default:
				break;
			}
		} else {
			assert(false);//not implemented
		}
		break;
	case ims_val::ETP::vector:
	{
		let sz = v->get_size();

		switch (v->gs()) {
		case ims_val::EST::rational:
			print_vector_as_aifs(str, v->p_i(), sz);
			return;
		case ims_val::EST::real:
			print_vector_as_aifs(str, v->p_r(), sz);
			return;
		case ims_val::EST::big_rational:
			print_vector_as_aifs(str, v->p_b(), sz);
			return;
		default:
			str << "[";
			let* sa = v->p_v();
			for (size_t i = 0; i < sz; ++i) {
				print_ims_val_ex(str, sa[i], lst);//recursion
				if (i + 1 < sz)str << ",";
			}
			str << "]";
			return;
		}
		break;
	}
		
	case ims_val::ETP::ast_ptr:
	{
		let* ast = v->gp<ast_context>();
		if (ast->h.tt == ETYPE::identifier) {
			let unk_id = ast->h.get_offset();
			str << ims_keywords::block << lst->m_idf.get_str_from_unk(unk_id);
		} else {
			str << "@" << ast->call_offset << "{";
			print_operator(*lst, str, *ast, ETYPE::min_priority, nullptr);
			//str<< (int)v->m_ast.h.tt << ", " <<	(int)v->m_ast.h.ts;
			str << "}";
		}
		return;
	}

	case ims_val::ETP::string:
	{
		str << "\"" << v->get_string() << "\"";
		return;
	}

	case ims_val::ETP::compos:
	{
		let sz = v->get_size();
		
		if (sz == 0) {
			str << "$i";
			return;
		}

		let* sa = v->p_v();

		if (sz == 1) {
			print_ims_val_ex(str, sa[0], lst);//recursion
			return;
		}

		str << "(";
		for (size_t i = 0; i < sz; ++i) {
			print_ims_val_ex(str, sa[i], lst);//recursion
			if (i + 1 < sz)str << " * ";
		}
		str << ")";
		return;

	}
	case ims_val::ETP::uni:
	{
		let sz = v->get_size();

		if (sz == 0) {
			str << "$e";
			return;
		}

		str << "(";
		let* sa = v->p_v();
		for (size_t i = 0; i < sz; ++i) {
			print_ims_val_ex(str, sa[i], lst);//recursion
			if (i + 1 < sz)str << " | ";
		}
		str << ")";
		return;
	}
	case ims_val::ETP::csg:
	{
		let sz = v->get_size();

		str << "csg(";
		let* sa = v->p_v();
		for (size_t i = 0; i < sz; ++i) {
			print_ims_val_ex(str, sa[i], lst);//recursion
			if (i + 1 < sz)str << ", ";
		}
		str << ")";
		return;
	}
	default:
		break;
	}
	str << "error";
}

void print_ims_val(const ims_val* v, const ifs_list* lst) 
{
	print_ims_val_ex(std::cout, v, lst);
}