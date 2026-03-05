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
#include "ims_operator.h"
#include "ims_keywords.h"
#include "oper_block.h"
#include "block_class.h"
#include "built_in_func.h"
#include "ifs_list.h"

static void write_double(char* dst, size_t dst_size, double v, const char* fmt)
{
	assert(fmt);
	auto res = fmt::format_to_n(dst, dst_size, fmt::runtime(fmt), v);
	*res.out = 0;//terminate string

	//remove trailing zeros
	for (;;) {
		if (*dst == 0)return;
		if (*dst++ == '.')break;
	}
	char* p = dst + strlen(dst) - 1;
	while (*p == '0')*p-- = 0;
	if (*p == '.')*p-- = 0;
};


static void write_num(
	std::ostream& str,
	const sval& v,
	ESUBTYPE st,
	ETYPE par_type,
	const char* fmt=nullptr)
{
	bool pb = par_type == ETYPE::power || par_type == ETYPE::power_imm;
	switch (st) {
	case ESUBTYPE::integer: 
	{
		let& x = v.i64;
		pb = pb && x < 0;
		if (pb)str << "(";
		str << x;
		if (pb)str << ")";
		break;
	}
	case ESUBTYPE::rational:
	{
		let& x = v.rt[0];
		pb = pb && x < 0;
		if (pb)str << "(";
		str << x;
		if (v.rt[1] != 1) {
			str << "/" << v.rt[1];
		}
		if (pb)str << ")";
		break;
	}
	case ESUBTYPE::real:
	{
		let& x = v.f64;
		pb = pb && x < 0;
		if (pb)str << "(";
		if (fmt) {
			char buf[32];
			write_double(buf, std::size(buf), x, fmt);
			str << buf;
		} else {
			str << x;
		}
		if (pb)str << ")";

		break;
	}
	default:
		assert(false);//TODO
		break;
	}

};

static void write_num_imm(
	std::ostream& str, 
	const ims_operator& v, 
	ETYPE par_type,
	const char* fmt = nullptr)
{
	bool pb = par_type == ETYPE::power || par_type == ETYPE::power_imm;
	switch (v.ts) {
	case ESUBTYPE::integer:
	case ESUBTYPE::rational:
	{
		int64_t a, b;
		v.get_int_imm(a, b);

		let& x = a;
		pb = pb && x < 0;
		if (pb)str << "(";
		str << x;
		if (b != 1) {
			str << "/" << b;
		}
		if (pb)str << ")";
		break;
	}
	case ESUBTYPE::real:
	{
		let& x = v.f32;
		pb = pb && x < 0;
		if (pb)str << "(";
		if (fmt) {
			char buf[32];
			write_double(buf, std::size(buf), x, fmt);
			str << buf;
		} else {
			str << x;
		}
		if (pb)str << ")";
		break;
	}
	default:
		assert(false);//TODO
		break;
	}

};




void print_operator(
	const ifs_list& lst,
	std::ostream& str,
	const operator_ptr& ptr,
	const ETYPE par_type,
	const char* fmt //{:.3f} for example
	)
{
	let& b = *ptr.a;
	let& op = ptr.h;

	let& a = b.m_ops;

	let ofs = op.get_offset();
	let t = op.tt;


	switch (t) {
	case ETYPE::reference:
	{
		{
			let id = b.get_class()->get_var_name(ofs);
			if (id.empty()) {
				str << ims_keywords::autoprefix << ofs;
			}else {
				str << id;
			}
		}
		
		break;
	}
	case ETYPE::unk_reference:
	{
		str << lst.m_idf.get_str_from_unk(ofs);
		
		break;
	}
	case ETYPE::empty:
	case ETYPE::pi:
	case ETYPE::id:
	{
		str << ims_keywords::builtin << ims_operator::to_string(t);
		break;
	}
	case ETYPE::this_vector:
	{
		str << ims_keywords::this_arr;
		break;
	}

	case ETYPE::power:
	case ETYPE::power_imm:
	{
		bool pr_bra = par_type == ETYPE::power || par_type == ETYPE::power_imm;
		if (pr_bra)str << "(";
		
		print_operator(lst, str, b.get_ptr(ofs), t, fmt);

		str << "^";
		if (t == ETYPE::power_imm) {
			str << op.get_pow_exponent_imm();
		}else{
			str << "(";
			print_operator(lst, str, b.get_ptr(ofs + 1), ETYPE::min_priority, fmt);
			str << ")";
		}
		if (pr_bra)str << ")";
		break;
	}
	case ETYPE::index:
	case ETYPE::index_imm:
	{
		print_operator(lst, str, b.get_ptr(ofs), t, fmt);
		str << "[";

		if (t == ETYPE::index_imm) {
			str << op.get_elem_index();
		}
		else {
			let sz = op.num_args();
			for (size_t i = 1; i < sz; ++i) {
				print_operator(lst, str, b.get_ptr(ofs + i), t, fmt);
				if (i + 1 < sz)str << ",";
			}
		}
		str << "]";
		break;
	}
	case ETYPE::vector_imm:
	case ETYPE::vector:
	{
		let sz = op.num_args();

		str << "[";
		for (size_t i = 0; i < sz; ++i) {
			if (t == ETYPE::vector_imm) {
				write_num(str, b.m_ops[ofs + i], op.ts, ETYPE::min_priority, fmt);
			}
			else {
				print_operator(lst, str, b.get_ptr(ofs + i), t, fmt);
			}
			if (i + 1 < sz)str << ",";
		}
		str << "]";
		break;
	}
	case ETYPE::number_imm:
		write_num_imm(str, op, par_type, fmt);
		break;
	case ETYPE::number:
		write_num(str, b.m_ops[ofs], op.ts, par_type, fmt);
		break;
	case ETYPE::call_built_in:
	{
		let f = op.get_builtin_func();
		let& bif = built_in_func::info[f];
		let sz = bif.sz;
		str << bif.name << "(";
		for (size_t i = 0; i < sz; ++i) {
			print_operator(lst, str, b.get_ptr(ofs + i), t, fmt);
			if (i + 1 < sz)str << ",";
		}
		str << ")";
		break;
	}

	case ETYPE::call:
	{
		let sz = op.num_args();

		if (op.ts == ESUBTYPE::call_new) {
			
			//standard layout
			str << ims_keywords::builtin;
			str << ims_operator::to_string(t);

			str << "(";
			for (size_t i = 0; i < sz; ++i) {
				print_operator(lst, str, b.get_ptr(ofs + i), t, fmt);
				if (i + 1 < sz)str << ",";
			}
			str << ")";

			break;
		}

		let& h = a[ofs].hdr;
		str << lst.m_idf.get_str_from_unk(h.get_offset());

		if (op.ts == ESUBTYPE::call_normal) {
			str << "(";

			for (size_t i = 1; i < sz; ++i) {
				print_operator(lst, str, b.get_ptr(ofs + i), t, fmt);
				if (i + 1 < sz)str << ",";
			}
			str << ")";
		} else {
			assert(op.ts == ESUBTYPE::call_fields);
			for (size_t i = 1; i < sz; ++i) {
				str << ".";
				let unk_id = b.get_ptr(ofs + i).h.get_offset();
				str << lst.m_idf.get_str_from_unk(unk_id);
			}
		}

		


		break;
	}
	case ETYPE::set_interval:
	case ETYPE::set_vector:
	case ETYPE::set_binary:
	{
		str << ims_keywords::builtin << ims_operator::to_string(t) << "(";

		size_t idx = 0;
		bool has_arg = false;

		if (t == ETYPE::set_vector ||
			t == ETYPE::set_binary ||
			t == ETYPE::set_permutation)
		{
			let d = op.get_u24();
			if (d > 0) {
				str << op.get_u24();
				has_arg = true;
			}
		}

		let ai = ofs + idx;
		if (!a[ai].hdr.is_distirib_def()) {
			if (has_arg) {
				str << ",";
			}
			print_operator(lst, str, b.get_ptr(ai), t, fmt);
		}

		str << ")";
		break;
	}
	case ETYPE::distribution_int:
	case ETYPE::distribution_real:
	{
		size_t na = 0;
		switch (op.ts) {
		case ESUBTYPE::dist_normal_def:
			na = 0;	break;
		case ESUBTYPE::dist_normal:
			na = 1;	break;
		case ESUBTYPE::dist_uniform:
			na = 2;	break;
		default:
			break;
		}

		str << ims_keywords::builtin << ims_operator::to_string(t);

		if (na > 0)str << "(";
		for (size_t i = 0; i < na; ++i) {
			let v = a[ofs + i].f64;
			let iv = int64_t(v);
			if (iv == v) str << iv;
			else 		str << v;
			if (i + 1 < na)str << ",";
		}

		if (na > 0)str << ")";
		break;
	}
	case ETYPE::exchange:
	case ETYPE::inversion:
	{
		str << ims_keywords::builtin << ims_operator::to_string(t) << "()";
		break;
	}

	case ETYPE::inv:
	{
		str << "/";
		print_operator(lst, str, b.get_ptr(ofs), t, fmt);
		break;
	}
	case ETYPE::neg:
	{
		//whether to print brackets
		bool pr_bra = par_type == ETYPE::power || par_type == ETYPE::power_imm;
		if (pr_bra)str << "(";
		str << "-";
		print_operator(lst, str, b.get_ptr(ofs), t, fmt);
		if (pr_bra)str << ")";
		break;
	}
	case ETYPE::uni:
	case ETYPE::mul:
	case ETYPE::sum:
	case ETYPE::mod:
	{
		let sz = op.num_args();

		const char* delim = nullptr;
		if (t == ETYPE::mul) {
			if (sz == 0) {
				str << "1";
				break;
			}
			delim = "*";
		}else if (t == ETYPE::uni) {
			if (sz == 0) {
				str << "$e";
				break;
			}
			delim = "|";
		}else if (t == ETYPE::sum) {
			if (sz == 0) {
				str << "0";
				break;
			}
			delim = "+";
		}else if (t == ETYPE::mod) {
			delim = "%";
		}
		else {
			assert(false);
		}

		//whether to print brackets
		bool pr_bra = par_type < t;

		if (pr_bra)str << "(";

		for (size_t i = 0; i < sz; ++i) {
			if (i > 0) {
				let at = a[ofs + i].hdr.tt;

				bool pr;

				if (at == ETYPE::inv && t == ETYPE::mul)
					pr = false;
				else if (at == ETYPE::neg && t == ETYPE::sum)
					pr = false;
				else
					pr = true;

				if (pr) {
					str << delim;
				}
			}
			print_operator(lst, str, b.get_ptr(ofs + i), t, fmt);
		}
		if (pr_bra)str << ")";

		break;
	}
	//standard layout
	case ETYPE::csg:
	case ETYPE::charpoly:
	case ETYPE::companion:
	case ETYPE::color_style:
	case ETYPE::thickness:
	case ETYPE::diagonal:
	case ETYPE::vector_func:
	case ETYPE::condition:
	case ETYPE::vector_union:
	case ETYPE::set_permutation:
	{
		let sz = op.num_args();

		if (t != ETYPE::condition) {
			str << ims_keywords::builtin;
		}
		
		str << ims_operator::to_string(t);

		if (op.ts == ESUBTYPE::call_normal) {
			str << "(";
			for (size_t i = 0; i < sz; ++i) {
				print_operator(lst, str, b.get_ptr(ofs + i), t, fmt);
				if (i + 1 < sz)str << ",";
			}
			str << ")";
		} else {
			assert(op.ts == ESUBTYPE::call_fields);
			for (size_t i = 0; i < sz; ++i) {
				str << ".";
				let unk_id = b.get_ptr(ofs + i).h.get_offset();
				str << lst.m_idf.get_str_from_unk(unk_id);
			}
		}

		break;
	}
	default:
		str << "~UNDEF~";
		break;
	}

}
