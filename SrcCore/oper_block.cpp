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
#include "oper_block.h"

#include "block_class.h"
#include "ims_random.h"
#include "search_info.h"
#include "variator.h"
#include "ims_info.h"

#include "block_graph.h"//can_exists

#include "eval_context.h"
#include "variable.h"
#include "ovr_data.h"

IMS_DEFINE_OPAQUE_DELETER(search_info);



std::mutex oper_block::s_access_lock;

const oper_block* oper_block::get_parent() const
{
	return m_parent;
}

void oper_block::set_parent(const oper_block* b)
{
	m_parent = b;
	if (b) {
		m_parent_id = b->m_block_id;
	} else {
		m_parent_id = block_id_max;
	}
}


void oper_block::prepare_js_parent()
{
	if (m_js_parent) {
		m_js_parent->clear();
	} else {
		m_js_parent = std::make_unique<oper_block>();
	}

	auto* jsp = m_js_parent.get();
	jsp->set_parent(get_parent());
	jsp->m_flags.from_js = true;
	jsp->m_flags.priv = true;
	
}

eval_context* oper_block::own_ctx() const
{
	let* p = get_parent();
	auto* r = m_ctx.get();
	return (p && p->ctx() == r) ? nullptr : r;
}


block_class* oper_block::get_class() const
{
	for (let* b = this; b; b = b->get_parent()) {
		if (b->m_class)return b->m_class.get();
	}
	return nullptr;
}

block_graph* oper_block::get_graph() const
{
	return m_graph.get();
}

eval_context* oper_block::ctx() const
{
	return m_ctx.get();
}


std::string_view oper_block::str_id4() const
{
	if (m_block_id == block_id_max) {
		return {};
	}
	return get_list().get_str(m_block_id);
}


size_t oper_block::simple_hash() const
{
	size_t hash = 0;
	for(let& q : m_ops){
		boost::hash_combine(hash, q.i64);
	}
	return hash;
}



const ifs_list& oper_block::get_list() const
{
	return get_class()->m_nfo->m_list;
}

size_t oper_block::get_froot() const
{
	return get_graph()->ref2fg(get_active_ref());
}

size_t oper_block::add(size_t num)
{
	size_t ret = m_ops.size();
	m_ops.resize(ret + num);
	for (size_t i = 0; i < num; ++i) {
		m_ops[ret + i].hdr.clear();
	}
	return ret;
}



bool oper_block::can_exists() const
{
	if (!m_flags.ready) {
		return true;//potentially - maybe
	}
	
	let* g = get_graph();
	return g && !g->m_g1.empty();
}

operator_ptr oper_block::get_ptr(size_t idx) const
{
	operator_ptr dst;
	dst.h = m_ops[idx].hdr;
	dst.a = this;
	return dst;
}

operator_ptr oper_block::get_ptr_to_elem(
	const ims_operator& vec, size_t idx) const
{
	if (vec.tt != ETYPE::vector_imm) {
		return get_ptr(vec.u32 + idx);
	}

	operator_ptr ret;
	ret.a = this;
	ret.h = vec;
	return ret.index_imm(idx);
}

bool oper_block::get_val(
	const ims_operator& h, bool& is_rational,
	double& fv, int64_t& n, int64_t& d) const
{
	is_rational = true;

	switch (h.tt) {
	case ETYPE::number:
	{
		let& im = m_ops[h.u32];

		switch (h.ts) {
		case ESUBTYPE::real:
			fv = im.f64;
			is_rational = false;
			break;
		case ESUBTYPE::integer:
			n = im.i64;
			d = 1;
			break;
		case ESUBTYPE::rational:
			n = im.rt[0];
			d = im.rt[1];
			break;
		default:
			return false;
		}
		break;
	}
	case ETYPE::number_imm:
	{
		switch (h.ts) {
		case ESUBTYPE::real:
			fv = h.f32;
			is_rational = false;
			break;
		case ESUBTYPE::integer:
			n = h.i32;
			d = 1;
			break;
		case ESUBTYPE::rational:
			n = h.r16[0];
			d = h.r16[1];
			break;
		default:
			return false;
		}
		break;
	}
	default:
		return false;
	}


	return true;
}


bool oper_block::get_int64(
	int64_t& numerator,
	int64_t& denominator,
	size_t idx,
	const ESUBTYPE st) const
{
	let& sv = m_ops[idx];

	switch (st) {

	case ESUBTYPE::integer:
		numerator = sv.i64;
		denominator = 1;
		return true;
	case ESUBTYPE::rational:
		numerator = sv.rt[0];
		denominator = sv.rt[1];
		return true;
	default:
		assert(false);
		return false;//not supported yet
	}
}


bool oper_block::get_double(const ims_operator& h, double& fv) const
{
	bool isr;
	int64_t an[2];
	if (!get_val(h, isr, fv, an[0], an[1])) {
		return false;//not implemented
	};

	if (isr) {
		fv = double(an[0]) / an[1];
	}
	return true;
};

std::string_view oper_block::get_graph_id() const
{
	let* p = get_parent();
	if (!p)return {};
	return p->str_id4();
}

bool oper_block::get_distrib(distrib_info& di, const operator_ptr& src) const
{
	let t = src.h.tt;

	if (t != ETYPE::set_interval && 
		t != ETYPE::set_binary && 
		t != ETYPE::set_vector) 
	{
		return false;
	}
	
	let dist_ptr = src.index_base(0);

	assert(dist_ptr.h.tt == ETYPE::distribution_int ||
		dist_ptr.h.tt == ETYPE::distribution_real);

	let ofs = dist_ptr.h.get_offset();

	
	di.t = dist_ptr.h.tt;
	di.s = dist_ptr.h.ts;
	di.d[0] = dist_ptr.a->m_ops[ofs].f64;
	di.d[1] = dist_ptr.a->m_ops[ofs + 1].f64;

	return true;
}

bool oper_block::apply_templates(
	const control_values& arr,
	const variator_params& vp,
	int_arr_ref* rel_vec)
{
	bool ret = false;

	for (let& q : arr.data) {
		let& src = q.src;
		let& op = src.h;
		let dst_idx = q.dst_idx9;


		distrib_info di;
		get_distrib(di, src);

		
		//convert the default distribution
		if (di.s == ESUBTYPE::dist_normal_def) {
			di.s = ESUBTYPE::dist_normal;
			di.d[0] = -vp.m_search_rad;//can be negative
		}

		switch (op.tt) {
		case ETYPE::set_vector:
		{
			bool rel = vp.m_relative_shift;
			generate_random_vector(dst_idx, src, di, rel ? rel_vec : nullptr);
			ret = true;
			break;
		}
		case ETYPE::set_interval:
			generate_random_number(dst_idx, di);
			ret = true;
			break;
		case ETYPE::set_binary:
			generate_random_binary(dst_idx, src, di);
			ret = true;
			break;
		default:
			break;
		}
		
	}

	return ret;

}





void oper_block::generate_random_vector(size_t dst_idx,
	size_t dim, const distrib_info& di, const int_arr_ref* proto)
{
	let nidx = add(dim);
	m_ops[dst_idx].hdr.u32 = static_cast<uint32_t>(nidx);

	bool err = false;

	auto& irn = ims_random::getR();

	if (di.t == ETYPE::distribution_int) {
		m_ops[dst_idx].hdr.ts = ESUBTYPE::integer;
		switch (di.s) {
		case ESUBTYPE::dist_normal:
		{
			let v = di.d[0];

			bool rel = proto && proto->sz == dim;

			for (size_t i = 0; i < dim; ++i) {
				auto r = round(std::abs(v) * irn.get_normal());
				if (v > 0)r = std::abs(r);

				auto ir = static_cast<int64_t>(r);

				if (rel) {
					ir += proto->ptr[i];
				}

				m_ops[nidx + i].i64 = ir;

			}
			break;
		}
		case ESUBTYPE::dist_uniform:
		{
			std::uniform_int_distribution<int64_t> distribution(
				(int64_t)di.d[0], (int64_t)di.d[1]);

			for (size_t i = 0; i < dim; ++i) {
				m_ops[nidx + i].i64 = distribution(irn.rng);
			}
			break;
		}
		default:
			err = true;
			break;
		}
	}
	else if (di.t == ETYPE::distribution_real) {
		m_ops[dst_idx].hdr.ts = ESUBTYPE::real;
		switch (di.s) {
		case ESUBTYPE::dist_normal:
		{
			let v = di.d[0];
			for (size_t i = 0; i < dim; ++i) {
				auto r = v * irn.get_normal();
				if (v > 0)r = std::abs(r);
				m_ops[nidx + i].f64 = r;
			}
			break;
		}
		case ESUBTYPE::dist_uniform:
		{
			std::uniform_real_distribution<double>
				distribution(di.d[0], di.d[1]);

			for (size_t i = 0; i < dim; ++i) {
				m_ops[nidx + i].f64 = distribution(irn.rng);
			}
			break;
		}
		default:
			err = true;
			break;
		}
	}
	else {
		err = true;
	}

	if (err) {
		assert(false);
		m_ops[dst_idx].hdr.ts = ESUBTYPE::integer;
		for (size_t i = 0; i < dim; ++i) {
			m_ops[nidx + i].i64 = 0;
		}
	}
}


void oper_block::generate_random_vector(size_t dst_idx,
	const operator_ptr& src, const distrib_info& di, const int_arr_ref* proto)
{
	let& op = src.h;
	auto dim = op.get_u24();
	if (dim == 0) {
		dim = get_dim();
	};

	m_ops[dst_idx].hdr.tt = ETYPE::vector_imm;
	m_ops[dst_idx].hdr.set_u24(dim);
	generate_random_vector(dst_idx, dim, di, proto);
}

void oper_block::generate_random_binary(size_t dst_idx,
	const operator_ptr& src, const distrib_info& di)
{

	auto& irn = ims_random::getR();

	let& op = src.h;
	auto dim = op.get_u24();

	let nidx = add(dim);


	auto& h = m_ops[dst_idx].hdr;
	h.tt = ETYPE::vector;
	h.u32 = static_cast<uint32_t>(nidx);
	h.set_u24(dim);


	if (di.t == ETYPE::distribution_int && di.s == ESUBTYPE::dist_uniform) {

		auto d0 = static_cast<size_t>(di.d[0]);
		auto d1 = static_cast<size_t>(di.d[1]);

		if (d1 > dim)d1 = dim;
		if (d0 > d1)d0 = d1;

		std::uniform_int_distribution<size_t> distribution(d0, d1);

		let v = distribution(irn.rng);

		boost::container::small_vector<size_t, 64> iarr;
		iarr.resize(dim);
		for (size_t i = 0; i < dim; ++i) {
			iarr[i] = i;
		}


		std::shuffle(iarr.begin(), iarr.end(), irn.rng);


		for (size_t i = 0; i < dim; ++i) {
			m_ops[nidx + i].hdr.set_id();
		}

		for (size_t i = 0; i < v; ++i) {
			m_ops[nidx + iarr[i]].hdr.set_xempty();
		};
	}
	else {
		assert(false);
		m_ops[dst_idx].hdr.ts = ESUBTYPE::integer;
		for (size_t i = 0; i < dim; ++i) {
			m_ops[nidx + i].hdr.set_xempty();
		}
	}
}

void oper_block::generate_random_number(size_t dst_idx,
	const distrib_info& di)
{
	m_ops[dst_idx].hdr.tt = ETYPE::number;
	generate_random_vector(dst_idx, 1, di);
}



void oper_block::insert_op_ex(
	size_t dst_idx,
	ast_context src,
	const eval_context& ctx,
	control_values* arr,
	bool do_subst)
{
	let& op = src.h;

	m_ops[dst_idx].hdr = op;//copy the header, then correct it

	switch (op.tt) {
	case ETYPE::reference:
	{
		if (do_subst) {
			//we don't copy the substitutions, but go deeper
			let& oi = ctx.m_refs5[src.get_ref_idx()];
			if (oi.is_subs) {
				insert_op_ex(dst_idx, oi.c, ctx, arr, do_subst);
			}
		}
		return;
	}
#ifdef use_call_resolver
	case ETYPE::call:
	{
		auto it = ctx.m_call_resolver.find(src);
		if (it == ctx.m_call_resolver.end()) {
			break;
		}
		src.h.set_reference(it->second);
		src.call_offset = 0;
		insert_op_ex(dst_idx, src, ctx, arr, do_subst);
		return;
	}
#endif
	case ETYPE::set_interval:
	case ETYPE::set_vector:
	case ETYPE::set_binary:
	{
		if (arr) {
			//remember the position for future filling
			auto& p = arr->data.emplace_back();
			p.dst_idx9 = dst_idx;
			p.src = src;
			return;//we're going to sort this out later
		}
		break;
	}
	default:
		break;
	}

	////////////////////////////////////////////////////////////////////////////

	let nd = src.h.data_args();
	let no = src.h.oper_args();

	if (nd == 0 && no == 0) {
		return;
	}

	//assert(this != src.a);

	let nidx = m_ops.size();

	//replace the offset
	m_ops[dst_idx].hdr.set_offset(nidx);

	//m_ops starts to change below, so dst can no longer be used!

	if (nd > 0) {//just copy the immediate arguments

		let it = src.a->m_ops.begin() + op.u32;
		m_ops.insert(m_ops.end(), it, it + nd);
		return;
	}

	//arguments - operators
	assert(no > 0);
	m_ops.resize(nidx + no);

	for (size_t i = 0; i < no; ++i) {
		//recursion
		insert_op_ex(
			nidx + i,
			src.index(i),
			ctx,
			arr,
			do_subst);
	}
}


////////////////////////////////////////////////////////////////////////////////

void oper_block::simple_copy(oper_block& dst) const
{
	dst.clear();

	dst.m_parent = m_parent;
	dst.m_parent_id = m_parent_id;
	dst.m_flags = m_flags;
	dst.m_block_id = block_id_max;
	dst.m_line8 = m_line8;
	dst.m_dim2 = m_dim2;
	dst.m_subspace = m_subspace;
	dst.m_named_vars = m_named_vars;
	dst.m_conv_id = m_conv_id;
	dst.m_name = m_name;
	dst.m_timestamp = m_timestamp;
	dst.m_ops = m_ops;//copying operations
	dst.m_first_var = m_first_var;
	
	dst.m_js_init = m_js_init;
	dst.m_js_parent = m_js_parent;
	dst.m_flags.ready = false;
	//dst.m_graph = m_graph;
	//dst.m_ctx = m_ctx;
	//dst.m_class = m_class;
	//dst.m_calc_data = m_calc_data;
	//dst.m_src2 = m_src2;
}


size_t oper_block::add_args(size_t idx, ETYPE t, size_t num)
{
	let ni = add(num);
	auto& ar = m_ops[idx];
	ar.hdr.set(t, ni, num);
	return ni;
}

var_header::type oper_block::add_var(uint32_t& prev, size_t ref, bool is_subs)
{
	let v =(var_header::type)add(var_header::total_elems);
	
	if (m_first_var == var_header::nil) {
		m_first_var = v;
	} else {
		assert(prev < var_header::nil);
		m_ops[prev].vh.next = v;
	}

	auto& vh = m_ops[v].vh;
	vh.ref8 = var_header::pack(ref, is_subs);
	vh.next = var_header::nil;

	prev = v;
	m_ops[v + var_header::offset_left].hdr.set_xempty();
	m_ops[v + var_header::offset_right].hdr.set_xempty();
	return v + var_header::offset_right;
}

size_t oper_block::add_builtin(builtin_ids bid)
{
	for (auto v = m_first_var; v != var_header::nil; v = m_ops[v].vh.next) {
		if (m_ops[v].vh.is_builtin(bid)) {
			return v + var_header::offset_right;//already exists
		}
	};

	let v = add(var_header::total_elems);

	auto& vh = m_ops[v].vh;
	vh.ref8 = (var_header::type)builtin2ref(bid);
	vh.next = m_first_var;
	m_first_var = (var_header::type)v;

	m_ops[v + var_header::offset_left].hdr.set_xempty();
	m_ops[v + var_header::offset_right].hdr.set_xempty();
	return v + var_header::offset_right;
}


void oper_block::remove_builtin(builtin_ids bid)
{
	var_header::type prev = var_header::nil;
	for (auto v = m_first_var; v != var_header::nil; v = m_ops[v].vh.next) {
		if (m_ops[v].vh.is_builtin(bid)) {
			let nxt = m_ops[v].vh.next;
			if (v == m_first_var) {
				m_first_var = nxt;
			} else {
				m_ops[prev].vh.next = nxt;
			}
		}
		prev = v;
	};
};


void oper_block::set_one_vector_arg(
	std::span<const int64_t> p, size_t ds, ETYPE t)
{
	//one block
	size_t rg = set_vector(ds, t, 1);
	let degree = p.size() - 1;

	let d = p[degree];

	let pbeg = set_vector_ex(
		rg++, 
		ETYPE::vector_imm, 
		d == 1 ? ESUBTYPE::integer : ESUBTYPE::rational, 
		degree);

	for (size_t k = 0; k < degree; ++k) {
		auto& dst = m_ops[pbeg + k];
		if (d == 1) {
			dst.i64 = p[k];
		} else {
			dst.rt[0] = static_cast<int32_t>(p[k]);
			dst.rt[1] = static_cast<int32_t>(d);
			assert(dst.rt[0] == p[k] && dst.rt[1] == d);
		}
	}
};

void oper_block::set_int_poly(
	std::span<const int64_t> p, 
	int64_t denom, size_t ds, 
	size_t arg)
{
	size_t ofs;

	if (denom == 1) {
		ofs = ds;
	} else {
		ofs = add_args(ds, ETYPE::mul, 2);
		assert(denom == static_cast<int32_t>(denom));
		set_rational(ofs++, 1, static_cast<int32_t>(denom));
	}

	size_t not_zero = 0;
	for (let& v : p) {
		if (v != 0)++not_zero;
	}

	ofs = add_args(ofs, ETYPE::sum, not_zero);

	for (size_t j = 0; j < p.size(); ++j) {
		int64_t v = p[j];
		if (v == 0)continue;

		auto a = ofs++;

		if (j == 0) {
			set_integer(a, v);
			continue;
		}

		if (v < 0) {
			a = add_args(a, ETYPE::neg, 1);
			v = -v;
		}

		if (v != 1) {
			a = add_args(a, ETYPE::mul, 2);
			set_integer(a++, v);
		}

		if (j != 1) {
			a = set_power(a, j);
		}
		m_ops[a].hdr.set_reference(arg);
	}
}

size_t oper_block::set_vector_ex(size_t idx, ETYPE t, ESUBTYPE st, size_t narg)
{
	let ni = add(narg);//where the arguments will be

	auto& h = m_ops[idx].hdr;
	h.tt = t;
	h.ts = st;
	h.u32 = static_cast<uint32_t>(ni);
	h.set_u24(narg);

	return ni;
}

size_t oper_block::set_vector(size_t idx, ETYPE t, size_t narg)
{
	//ESUBTYPE::integer is used as a mockup
	return set_vector_ex(idx, t, ESUBTYPE::integer, narg);
}

size_t oper_block::set_vector(size_t idx, size_t narg)
{
	return set_vector(idx, ETYPE::vector,  narg);
}


size_t oper_block::find_default_ref() const
{	
	let& ec = *ctx();

	let& ra = ec.m_refs5;
	let* g = get_graph();

	size_t ret = ims_max;
	for (let* b = this; b; b = b->get_parent()) {
		size_t var_idx = 0;
		
		for (let& q : *b) {
			IMS_SCOPE([&] {++var_idx; });
			if (q.is_builtin())continue;
			if (var_idx >= b->m_named_vars) {
				break;
			}

			let idx = q.gr();

			if (g->closed2(idx) && !ra[idx].is_subs) {
				if (ret == ims_max || ret < idx) {
					ret = idx;
				}
				//seeking further
			}
		}
	}

	return ret;
}

size_t oper_block::set_neg(size_t idx)
{
	auto h(m_ops[idx].hdr);//copy because m_ops changes!!!

	let ni = add(1);
	h.tt = ETYPE::neg;
	h.set_u32(ni);

	//copy back
	m_ops[idx].hdr = h;

	return ni;
}

size_t oper_block::set_power(size_t idx, intptr_t e)
{
	if (e == 1)return idx;
	auto h(m_ops[idx].hdr);//copy because m_ops changes!!!
	
	size_t ni;//where the arguments will be

	if (h.set_i24(e)) {
		ni = add(1);
		h.tt = ETYPE::power_imm;
	} else {
		ni = add(2);
		h.tt = ETYPE::power;
		set_integer(ni + 1, e);
	}

	h.set_u32(ni);

	//copy back
	m_ops[idx].hdr = h;

	return ni;
}

void oper_block::set_exchange(size_t idx)
{
	auto& h = m_ops[idx].hdr;
	h.tt = ETYPE::exchange;
	h.ts = ESUBTYPE::integer;
	h.set_u32(0);
	h.set_u24(0);
};


void oper_block::set_mobius(size_t idx)
{
	auto& h = m_ops[idx].hdr;
	h.tt = ETYPE::inversion;
	h.ts = ESUBTYPE::integer;
	h.set_u32(0);
	h.set_u24(0);
};


size_t oper_block::set_power_ref(size_t idx)
{
	let ni = add(2);//where the argument will be located

	auto& h = m_ops[idx].hdr;

	h.tt = ETYPE::power;
	h.set_u32(ni);

	return ni;
}


void oper_block::set_distribution(
	size_t idx, ETYPE t, ESUBTYPE s, double v1, double v2)
{
	let ni = add(2);
	auto& h = m_ops[idx].hdr;
	h.tt = t;
	h.ts = s;
	h.set_u32(ni);
	m_ops[ni].f64 = v1;
	m_ops[ni+1].f64 = v2;
}


size_t oper_block::set_binary_or_vector(size_t idx, ETYPE t, size_t sz)
{
	let ni = add(1);
	auto& h = m_ops[idx].hdr;
	h.tt = t;
	h.ts = ESUBTYPE::integer;
	h.set_u32(ni);
	h.set_u24(sz);
	return ni;
}


void oper_block::set_distribution_def(size_t idx)
{
	set_distribution(idx, 
		ETYPE::distribution_int, ESUBTYPE::dist_normal_def, 0, 0);
}

void oper_block::set(size_t idx, ETYPE t, size_t primary, size_t secondary)
{
	m_ops[idx].hdr.set(t, primary, secondary);
}


void oper_block::set_double(ims_operator& h, double v)
{
	h.tt = ETYPE::number;
	h.ts = ESUBTYPE::real;
	h.set_u32(m_ops.size());
	let ni = add(1);//where the argument will be located
	m_ops[ni].f64 = v;
}

void oper_block::set_double(size_t idx, double v)
{
	set_double(m_ops[idx].hdr, v);
}

 bool oper_block::get_builtin(operator_ptr& dst, builtin_ids id) const
{
	for (let* p = this; p; p = p->get_parent()) {
		for (let& q : *p) {
			if (q.is_builtin() && q.get_builtin() == id) {
				dst = p->get_ptr(q.pos5);
				return true;
			}
		}
	}
	return false;
}

void oper_block::get_builtins(builtin_arr& dst_arr, bool rec) const
{
	//fill in the references from all parents
	for (size_t i = 0; i < c_num_builtins; ++i) {
		auto& dst = dst_arr[i];
		dst.a = nullptr;
		dst.h.set_xundef();
	}
	for (let* p = this; p; p = p->get_parent()){
		for (let& q : *p) {
			if (!q.is_builtin())continue;
			auto& dst = dst_arr[(size_t)q.get_builtin()];
			if (dst.h.is_xundef()) {//not filled out before
				dst = p->get_ptr(q.pos5);
			}
		}
		if (!rec)break;
	}
}

void oper_block::set_integer(size_t idx, int64_t v)
{
	set_integer(m_ops[idx].hdr, v);
}

void oper_block::set_integer(ims_operator& h, int64_t v)
{
	let v32 = static_cast<int32_t>(v);
	if (v32 == v) {
		h.set_small_int(v32);
		return;
	}

	h.tt = ETYPE::number;
	h.ts = ESUBTYPE::integer;
	h.set_u32(m_ops.size());
	let ni = add(1);//where the argument will be located
	m_ops[ni].i64 = v;
}


void oper_block::set_rational(size_t idx, int32_t numer, int32_t denom)
{
	set_rational(m_ops[idx].hdr, numer, denom);
}


void oper_block::set_rational(ims_operator& h, int32_t numer, int32_t denom)
{
	let g = boost::integer::gcd(numer, denom);
	numer /= g;
	denom /= g;
	if (denom < 0) {
		denom = -denom;
		numer = -numer;
	};
	
	if (denom == 1) {
		h.set_small_int(numer);
		return;
	}

	

	let n16 = static_cast<int16_t>(numer);
	let d16 = static_cast<int16_t>(denom);

	if (n16 == numer && d16 == denom) {
		h.set_small_rational(n16, d16);
		return;
	}

	let ni = add(1);//where the argument will be located
	h.tt = ETYPE::number;
	h.ts = ESUBTYPE::rational;
	m_ops[ni].rt[0] = numer;
	m_ops[ni].rt[1] = denom;
}


block_class& oper_block::set_new_class(const ims_info* nfo)
{
	assert(!m_class);
	m_class.reset(new block_class(nfo));
	return *m_class;
}

block_class& oper_block::create_own_class()
{
	return m_class ? *m_class : create_copy(get_class());
}

block_class& oper_block::create_copy(const block_class* p)
{
	assert(!m_class);
	assert(p);
	m_class.reset(new block_class(*p));
	return *m_class;
}



size_t oper_block::get_var_from_unk(size_t unk) const
{
	return get_class()->find_var(unk);
}

size_t oper_block::num_vars() const
{
	return get_class()->m_refs.size();
}

bool oper_block::completely_defined() const
{
	if (is_invalid()) {
		return false;
	}
	if (get_dim() == 0) {
		return false;
	}

	let* G = ctx();
	//assert(G);
	if (!G)return false;
	
	if (own_ctx()) {
		return !G->has_vars();
	} else {
		return !m_flags.free_var;
	}
}

bool oper_block::can_be_proto() const
{
	if (is_invalid()) {
		return false;
	}
	let* G = ctx();
	assert(G);
	
	if (!can_exists() || is_converter()) {
		return false;
	}
	
	if (own_ctx()) {
		return G->has_vars();//there is something to vary
	} else {
		//only if there are free or consists only of non-free variations
		return m_flags.free_var || m_flags.only_var;
	}
}

bool oper_block::can_be_proto_ex() const
{
	if (is_invalid()) {
		return false;
	}
	let* G = ctx();
	assert(G);

	if (!can_exists() || is_converter()) {
		return false;
	}

	if (own_ctx()) {
		return !G->has_vars();
	}else {
		return !m_flags.free_var;
	}
}


bool oper_block::can_be_replaced() const
{
	//TODO: understand
	return !has_id() && !m_src2 && !own_ctx();
}


bool oper_block::convert_type_inplace(ims_operator& h, bool to_int)
{
	auto dst_type = to_int ? ESUBTYPE::integer : ESUBTYPE::real;

	if (h.ts == dst_type) {
		return false;
	}

	size_t num_el;
	if (h.tt == ETYPE::number)num_el = 1;
	else if (h.tt == ETYPE::vector_imm)num_el = h.get_u24();
	else {
		return false;
	};

	for (size_t i = 0; i < num_el; ++i) {
		double dv;
		int64_t iv = 0, denominator = 0;
		bool is_int = false;

		ims_operator op = h;
		if (op.tt == ETYPE::vector_imm) {
			op.tt = ETYPE::number;
			op.set_offset(op.get_offset() + i);
		};

		bool res = get_val(op, is_int, dv, iv, denominator);

		if (to_int) {
			if (!res) {
				iv = 0;
			} else if (is_int) {
				iv = iv / denominator;
			} else {
				iv = (int64_t)std::round(dv);
			}
			m_ops[h.u32 + i].i64 = iv;
		} else {
			if (!res) {
				dv = 0;
			} else if (is_int) {
				dv = double(iv) / denominator;
			}
			m_ops[h.u32 + i].f64 = dv;
		}
	}

	h.ts = dst_type;
	return true;
};

void oper_block::adjust_vector(size_t pos)
{
	//find a common numeric type
	auto& v = m_ops[pos].hdr;
	assert(v.tt == ETYPE::vector);
	let nv = v.num_args();

	//the resulting vector can be an int64, a pair of int32, or a float64

	//all arguments can be converted to type
	bool conv_to_f64 = true;
	bool conv_to_rat = true;
	bool conv_to_int = true;
	
	bool isr;//is it rational?
	int64_t an[2];//for rational
	double fv;//for real

	for (size_t i = 0; i < nv; ++i) {
		let& el = m_ops[v.u32 + i].hdr;

		if (!get_val(el, isr, fv, an[0], an[1])) {
			return;
		};
		
		if (isr) {
			if (an[1] != 1) {
				conv_to_f64 = false;
				conv_to_int = false;
				assert(static_cast<int32_t>(an[0]) == an[0]);
				assert(static_cast<int32_t>(an[1]) == an[1]);
			} else {
				if (static_cast<double>(an[0]) != an[0]) {
					conv_to_f64 = false;
				}
				if (static_cast<int32_t>(an[0]) != an[0]) {
					conv_to_rat = false;
				}
			}
		} else {
			if (el.ts == ESUBTYPE::real) {
				if (static_cast<int64_t>(fv) != fv) {
					conv_to_int = false;
				} if (static_cast<int32_t>(fv) != fv) {
					conv_to_rat = false;
				}
			} else {
				return;
			}
		};
	}

	ESUBTYPE targ;
	if (conv_to_int) {
		targ = ESUBTYPE::integer;
	} else if (conv_to_rat) {
		targ = ESUBTYPE::rational;
	} else  if (conv_to_f64) {
		targ = ESUBTYPE::real;
	} else {
		return;
	}

	//replace each element of the vector (type ims_operator)
	//with immediate values

	v.tt = ETYPE::vector_imm;
	v.ts = targ;
	
	for (size_t i = 0; i < nv; ++i) {
		auto& im = m_ops[v.u32 + i];

		if (!get_val(im.hdr, isr, fv, an[0], an[1])) {
			assert(false);
			return;
		};

		switch (targ) {
		case ESUBTYPE::integer:
			if (isr) {
				im.i64 = an[0];
			} else {
				im.i64 = static_cast<int64_t>(fv);
			}
			break;
		case ESUBTYPE::rational:
			if (isr) {
				im.rt[0] = static_cast<int32_t>(an[0]);
				im.rt[1] = static_cast<int32_t>(an[1]);
			} else {
				im.rt[0] = static_cast<int32_t>(fv);
				im.rt[1] = 1;
			}
			break;
		case ESUBTYPE::real:
			if (isr) {
				im.f64 = static_cast<double>(an[0]);
			} else {
				im.f64 = fv;
			}
			break;
		default:
			assert(false);
			return;
		}

	};

	m_ops.resize(v.u32 + nv);

}


void oper_block::copy_ovr(
	const oper_block& src,
	const variator_params& vp,
	control_values& od,
	std::span<const override_info> ovr)
{

	auto& dst = *this;
	assert(&src != &dst);

	dst.m_class.reset();

	let* p = src.get_parent();
	if (src.m_js_parent) {
		p = p->get_parent();
	}
	dst.set_parent(p);
	
	dst.m_dim2 = src.m_dim2;
	dst.m_subspace = src.m_subspace;
	
	boost::container::small_vector<int64_t, 10> vec;

	//////////////////////////////////////////////
	dst.clear_ops();
	uint32_t pos = 0;


	for(let& oi: ovr){
		if (ref_is_builtin(oi.ref2)) {
			continue;//do not copy
		}

		let ds = dst.add_var(pos, oi.ref2, false);//not a substitution
		
		od.data.clear();
		dst.insert_op_ex(ds, oi.src, *src.ctx(), &od);

		////////////////////////////////////////////////////////////////////////
		//support for relative variation of shifts
		oper_block::int_arr_ref rel_vec = { nullptr,0 };
		if (!operator_ptr::equal_raw(oi.proto2, oi.src)) {
			//find the first vector in the operator
			auto vop(oi.proto2);

			if (vop.h.tt == ETYPE::mul) {//looking for a vector inside the product
				let na = vop.h.num_args();
				for (size_t j = 0; j < na; ++j) {
					let arg = vop.index(j);
					let& ha = arg.h;
					if (ha.tt == ETYPE::vector_imm && ha.ts == ESUBTYPE::integer) {
						vop = arg;
						break;
					};
				}
			}
			if (vop.h.tt == ETYPE::vector_imm && vop.h.ts == ESUBTYPE::integer) {
				let na = vop.h.num_args();
				vec.resize(na);
				for (size_t j = 0; j < na; ++j) {
					vec[j] = vop.a->m_ops[vop.h.u32 + j].i64;
				}
				rel_vec.ptr = vec.data();
				rel_vec.sz = vec.size();
			};
		};
		////////////////////////////////////////////////////////////////////////

		
		dst.apply_templates(od, vp, &rel_vec);
	}


	dst.m_block_id = block_id_max;
	dst.remove_search();
	

	auto& f = dst.m_flags;
	f = src.m_flags;
	f.free_var = false;
	f.only_var = true;
	f.has_dim = false;
	f.checked = false;
	f.hidden = false;
	f.from_js = false;
	f.ready = false;
};


void oper_block::inherit_view(const oper_block& src)
{
	let* p = &src;
	while (p->m_flags.only_view)p = p->get_parent();
	
	auto& dst = *this;

	dst.clear();
	dst.set_parent(p);
	dst.m_name = p->m_name;
	dst.m_dim2 = p->m_dim2;
	dst.m_subspace = p->m_subspace;

	auto& f = dst.m_flags;
	f = p->m_flags;

	f.only_view = true;
	f.only_var = true;
	f.hidden = false;
	f.checked = false;
	f.ready = false;
	f.has_dim = false;
}


//for the new block, we collect:
//1) copies of all variations from the original block if it doesn't have its own graph
//2) variations that aren't overlapped in the hierarchy - randomly from the graph
//returns true if random changes were occured
bool oper_block::inherit_from(
	const oper_block& src,
	const variator_params& vp,
	control_values2& temp,
	bool copy_view)
{		
	bool ret = false;

	std::scoped_lock lock(s_access_lock);

	auto& dst = *this;
	
	assert(&src != &dst);


	let& ec = *src.ctx();

	let sz = src.num_vars();
	//let sz = g->m_refs5.size();
	
	temp.a.resize(sz);

	for (size_t i = 0; i < sz; ++i) {
		auto& q = temp.a[i];
		q.vis = ec.m_refs5[i].is_var();	//can be taken from its own context
		q.vis2 = false;	//this should be taken from the current block
	}

	let* par = &src;

	if (!src.own_ctx() || src.m_js_parent) {
		if (src.m_js_parent) {
			par = src.m_js_parent->get_parent();
		}else if (src.m_flags.only_view && copy_view) {
			//if inherited from view, then we go up once
			par = par->get_parent();
		} else {
			//all variations will be overridden
			while (!par->own_ctx() && par->m_flags.only_var) {
				par = par->get_parent();
			}
		}
		assert(par);

		//copy all variations from the current block
		for (let& q : src) {
			if (q.is_builtin() || !ec.m_refs5[q.gr()].is_var()) {
				continue;
			}
			auto& d = temp.a[q.gr()];
			d.vis2 = true;
			d.ptr2 = ast_context{ src.get_ptr(q.pos5), 0 };
		}

		//all variables that are not overridden in the hierarchy are taken randomly
		//from their own context (open variations)
		for (let* p = src.get_parent(); p && (!p->own_ctx() || p->m_js_parent); p = p->get_parent()) {
			//if (p->m_flags.priv)continue;
			for (let& q : *p) {
				if (q.is_builtin())continue;
				//you can't take a random variation from its own context,
				//because the variation is overridden by the hierarchy
				temp.a[q.gr()].vis = false;
			}
		}
	}
	

	dst.m_class.reset();
	dst.set_parent(par);
	dst.m_dim2 = src.m_dim2;
	dst.m_subspace = src.m_subspace;
	
	dst.clear_ops();
	uint32_t pos = 0;

	for (size_t i = 0; i < sz; ++i) {
		auto& od = temp.a[i];
		if (!od.vis2 && !od.vis)continue;

		let ds = dst.add_var(pos, i, false);//not a substitution
	
		if (od.vis2) {//copy the variation from the current block
			dst.insert_op_ex(ds, od.ptr2, ec);
		} else {//take an open variation from the context
			assert(od.vis);
			temp.cv.data.clear();
			dst.insert_op_ex(ds, ec.m_refs5[i].c, ec, &temp.cv);
			if (dst.apply_templates(temp.cv, vp, nullptr)) {
				ret = true;
			}
		}
	}

	if (copy_view) {
		builtin_arr ptrs;
		src.get_builtins(ptrs, true);

		for (size_t i = 0; i < c_num_builtins; ++i) {
			let bid = builtin_ids(i);
			if (bid == builtin_ids::subspace)continue;
			let& ptr = ast_context{ ptrs[i],0 };
			if (ptr.h.is_xundef()) {
				continue;
			}
			
			let ds = dst.add_var(pos, builtin2ref(bid), false);
			dst.insert_op_ex(ds, ptr, ec);
		}
	}

	//the following fields are empty - the block is temporary
	dst.m_block_id = block_id_max;
	dst.m_name.clear();
	dst.remove_search();

	auto& f = dst.m_flags;
	f = src.m_flags;
	f.free_var = false;
	f.only_var = true;
	f.has_dim = false;
	f.checked = false;
	f.hidden = false;
	f.from_js = false;
	f.ready = false;

	return ret;
}


///////////////////////////////////////////////////////////////////////////////

const std::string* oper_block::get_comment(size_t ref_id) const
{
	for (let* q = this; q; q = q->get_parent()) {
		if (!q->m_src2)continue;
		let& a = q->m_src2->ref2comments;
		if (ref_id < a.size() && !a[ref_id].empty()) {
			return &a[ref_id];
		}
	}
	return nullptr;
}

std::string oper_block::get_block_decription() const
{

	let* q = this;
	for (; q; q = q->get_parent()) {
		if (q->m_src2)break;
	}
	

	std::string ret;

	if (!q)return ret;

	if (q && !q->m_src2->md.empty()) {
		ret += q->m_src2->md;
		ret += "\n---\n\n";
	}
	
	if (q->has_id()) {
		ret += "**ID: ";
		ret += q->str_id4();
		ret += "**\n";
	}
	if (!q->m_name.empty()) {
		ret += "**Name: ";
		ret += m_name;
		ret += "**\n";
	}

	if (q) {
		let& r2c = q->m_src2->ref2comments;
		if (!r2c.empty()) {
			ret += "\n**Operators:** \n\n";
			let* c = q->get_class();
			for (size_t ref = 0; ref < r2c.size(); ++ref) {
				let& d = r2c[ref];
				if (!d.empty()) {
					ret += c->get_var_name(ref);
					ret += ": ";
					ret += d;
					ret += "\n";
				}
			}
		}
	}

	return ret;
}


bool oper_block::empty4() const
{
	return m_ops.empty() && !m_src2 && !m_parent;
}

const oper_block* oper_block::elevate_empty() const
{
	let* b = this;
	for (; b; b = b->get_parent()) {
		if (b->m_src2 || !b->m_ops.empty())break;
	}
	return b;
}


const oper_block* oper_block::elevate_priv() const
{
	let* b = this;
	for (; b; b = b->get_parent()) {
		if (!b->m_flags.priv)break;
	}	
	return b;
}

void oper_block::remove_search()
{
	m_calc_data.reset();
}


void oper_block::clear_ops()
{
	m_first_var = var_header::nil;
	m_ops.clear();
}

void oper_block::clear()
{
	m_js_init = ims_max;
	
	m_parent = nullptr;

	m_parent_id = block_id_max;
	m_flags.clear();
	
	m_timestamp = 0;

	clear_ops();

	m_line8 = 0;

	m_name.clear();

	if (m_calc_data) {
		m_calc_data->clear();
	}
	m_src2.reset();

	
	m_js_parent.reset();

	m_dim2 = 0;
	m_subspace.set_undef();

	m_named_vars = ims_max;

	
	m_conv_id = block_id_max;

	m_block_id = block_id_max;

	m_graph.reset();
	m_ctx.reset();
	m_class.reset();
}


size_t oper_block::get_active_ref() const
{
	for (let* p = this; p; p = p->get_parent()) {
		let ar = p->get_class()->m_active_ref;
		if (ar != ims_max) {
			return ar;
		}
	}
	return ims_max;
}

void oper_block::set_active_ref(size_t r)
{
	get_class()->m_active_ref = r;
}

bool oper_block::has_own_dim() const
{
	return m_flags.has_dim;
}

size_t oper_block::get_dim() const
{
	return m_dim2;
}

void oper_block::set_own_dim()
{
	let* p = get_parent();
	m_flags.has_dim = (!p && m_dim2 > 0) || (p && p->get_dim() != m_dim2);
}

bool oper_block::fix_js_parent()
{
	let* p = get_parent();
	if (p && p == m_js_parent.get()) {
		set_parent(p->get_parent());
		m_flags.ready = false;
		return true;
	}
	return false;
}

void oper_block_flags::clear()
{
	ready = false;
	checked = false;
	hidden = false;
	has_dim = false;
	has_timestamp = false;
	marked = false;
	free_var = false;
	only_var = false;
	only_view = false;
	priv = false;
	from_js = false;
}

void oper_block_flags::clear_but_attr()
{
	auto attr = *this;
	clear();
	checked = attr.checked;
	hidden = attr.hidden;
}

void oper_block_flags::clear_attr()
{
	checked = false;
	hidden = false;
}

var_header& oper_block::it_value::get_vh(oper_block& b) const
{
	let pos = pos5 - var_header::offset_right;
	return b.m_ops[pos].vh;
}
ims_operator& oper_block::it_value::get_left(oper_block& b) const
{
	let pos = pos5 - var_header::offset_right + var_header::offset_left;
	return b.m_ops[pos].hdr;
}
