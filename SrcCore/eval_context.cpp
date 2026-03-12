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
#include "eval_context.h"
#include "oper_block.h"
#include "ifs_list.h"//ETYPE::call - get_func_for_call
#include "built_in_func.h"
#include "eval_helpers.h"
#include "ims_val.h"
#include "eval_pool.h"
#include "math_helpers.h"
#include "block_graph.h"
#include "variable.h"
#include "ims_info.h"

#define eval_error(msg) if (!m_stack) { ims_error (msg);}
#define eval_error2(msg, a) if (!m_stack) { ims_error (msg, a);}
#define eval_error3(msg, a1, a2) if (!m_stack) { ims_error (msg, a1, a2);}

static bool geom_invalid(const ims_val * v, bool is_geom)
{
	if (!is_geom) {
		return false;
	}
	if (!v) {
		return true;
	}
	if (!v->is(ims_val_b::ETP::ast_ptr)) {
		return false;
	}
	if (v->gp<ast_context>()->h.tt == ETYPE::reference) {
		return true;
	}
	return false;
}

const ims_val* get_ast_val(const ast_context & p)
{
	auto* ret = eval_pool::ep.alloc_scalar(
		ims_val::ETP::ast_ptr,
		ims_val::EST::pod,
		sizeof(ast_context));

	new (ret->gp<ast_context>()) ast_context(p);
	return ret;
}

variable& eval_context::get_ref4(size_t idx)
{
	return m_refs5[idx];
}

const variable& eval_context::get_ref4(size_t idx) const
{
	return m_refs5[idx];
}

bool eval_context::is_geom(size_t ref) const
{
	return get_ref4(ref).is_geom();
}

bool eval_context::is_ready(size_t idx, bool is_geom) const
{
	return m_refs5[idx].ready[get_val_idx(is_geom)];
}


bool eval_context::has_vars() const
{
	return m_geom_templates > 0;
}


void eval_context::set_own_block(const oper_block& block, eval_stack* stack)
{
	let sz = block.num_vars();
	m_nfo = block.get_nfo();

	m_topo_unions = 0;
	m_topo_cycles = 0;
	m_geom_templates = 0;

	ims_resize(m_refs5, sz);//reset all to default

	m_fields_call.clear();

#ifdef use_eval_arg_cahche
	m_call_cache.clear();
#endif

	for (size_t i = 0; i < sz; ++i) {
		auto& r = m_refs5[i];
		r.c.call_offset = ims_max;//flag that it is not blocked
	}

	for (auto* b = &block; b; b = b->get_parent()) {
		for (let& q : *b) {
			if (q.is_builtin())continue;
			auto& d = get_ref4(q.gr());

			if (d.c.call_offset != ims_max)continue;//already overridden
			d.c = { b->get_ptr(q.pos5), 0 };//overriding
			d.is_subs = q.is_subs();
		}
	}

	m_stack = stack;
	//calculate everything that is possible geometrically
	for (size_t i = 0; i < sz; ++i) {
		eval_ref(i, true);
	}
	m_stack = nullptr;
}


bool eval_context::set_block(const oper_block& block)
{
	assert(this != block.ctx());

	*this = *block.ctx();//full copy

	//set up new overloaded
	for (let* b = &block; !b->own_ctx(); b = b->get_parent()) {

		for (let& q : *b) {
			if (q.is_builtin())continue;

			auto& d = get_ref4(q.gr());

			if (d.overriden)continue;//already overridden
			d.overriden = true;

			d.c = { b->get_ptr(q.pos5), 0 };//overriding
			d.is_subs = q.is_subs();
		}
	}

	//propagate the override flag to dependent ones
	let& g = block.get_graph()->m_deps;
	for (let v : g.m_ver_sorted) {

		let ne = g.num_edges(v);

		for (size_t e = 0; e < ne; ++e) {
			let& qe = g.get_edge(v, e);
			if (m_refs5[qe.second].overriden && m_refs5[v].is_geom()) {
				m_refs5[v].overriden = true;
				break;
			}
		}
	}

	for (auto& r : m_refs5) {
		if (r.overriden) {
			if (!r.is_geom()) {
				//overriding topology results in a new graph
				return false;
			}

			//geometrically recalculate all overrides (if needed)
			r.ready[0] = false; r.v[0].reset();
		}

		//there is a chance that the previously erroneous one will be calculated
		if (r.ready[0] && !r.v[0]) {
			r.ready[0] = false;//for example for 4Gen.aifs
		}

		if (r.is_geom()) {
			//will topologically recalculate all geometric
			r.ready[1] = false;
		}
	}

	//topological calculations of former geometric
	for (size_t i = 0; i < m_refs5.size(); ++i) {
		auto& r = m_refs5[i];
		
		if (r.v[0])continue;//can be used without breaking it into atoms

		if (!r.is_geom() && r.v[1])continue;
		
		bool was_invalid = !r.v[1] && r.ready[1];
		if (was_invalid) {
			r.ready[1] = false;
		}

		eval_ref(i, false);

		//r is invalidated here

		if (m_refs5[i].v[1] && was_invalid) {
			return false;
		}

		if (!m_refs5[i].is_geom() && r.overriden) {
			return false;//someone overriden by topological one
		}
	}

	return true;
};



const ims_val* eval_context::eval_ref(size_t idx, bool is_geom)
{
	auto* rf = &get_ref4(idx);//may be invalidated on the first eval

	//let orig_sz = m_refs5.size();
	//let orig_fc = m_fields_call.size();

	if (rf->in_eval) {
		++m_topo_cycles;	//cycle
	}

	const ims_val* v;

	let v_idx = get_val_idx(is_geom);

	if (m_stack && is_geom) {
		m_stack->push(idx);
	}

	if (!rf->ready[v_idx]) {
		rf->ready[v_idx] = true;
		//recursive execution will result in a nullptr return
		//this is correct for any type of calculation!
		
		let was_unions	= m_topo_unions;
		let was_cycles	= m_topo_cycles;
		let was_templates = m_geom_templates;

		rf->in_eval = true;
		v = eval7(rf->c, is_geom);
		rf = &get_ref4(idx);//restore after eval
		rf->in_eval = false;
		rf->v[v_idx].reset(v);//take ownership

		if (m_topo_unions > was_unions) {
			rf->dep_from_unions = true;
		}
	
		if (m_topo_cycles > was_cycles) {
			rf->dep_from_cycles = true;
		}
		
		if (m_geom_templates > was_templates) {
			if (!rf->is_subs) {	
				//if it is not a substitution, don't report above that there were substitutions
				//this happens in the following code, in exactly this order:
				
				//&r=$number($integer(0,7))
				//a=b
				//b=r
				
				//if you don't keep track, then "a" also starts to be considered as variation
				//which is incorrect...

				//GoogleTest: testEval::test_vars
				m_geom_templates = was_templates;
			} 
			rf->is_var2 = true;
		}

	} else {
		v = rf->v[v_idx].get();
		if (v) {
			//inherit flags
			if (rf->dep_from_unions)++m_topo_unions;
			if (rf->dep_from_cycles)++m_topo_cycles;
			
		}
		if (rf->is_subs && rf->is_var2)++m_geom_templates;
		
	}

	if (m_stack && is_geom) {
		m_stack->pop();
	}

	return v;
}


////////////////////////////////////////////////////////////////////////////////


const ims_val* eval_context::eval_reference(ast_context p, bool is_geom)
{
	assert(p.h.ts != ESUBTYPE::ref_unknown);
	let idx = p.get_ref_idx();
	//calculate in any case to enumerate dependencies!
	let* v = eval_ref(idx, is_geom);

	if (is_geom && v) {
		v->add_ref();
		return v;
	}
	return get_ast_val(p);
};

const ims_val* eval_context::eval_unk_ref(ast_context p, bool)
{
	let uid = p.h.get_unk_id();

	let& d =  m_nfo->m_list.m_idf.m_idx2unknown[uid]->second;
	if (d.has_block()) {
		return get_ast_val(p);
	}
	assert(d.has_js_entry());

	if (m_nfo->m_js_engine.js_obj_is_function(d.js_export_entry)) {
		return get_ast_val(p);
	}

	let* ret = const_cast<ims_info*>(m_nfo)->
		m_js_engine.js_obj_get_imm_value(d.js_export_entry);

	if (!ret) {
		eval_error("invalid js value");
	}
	return ret;
}


const ims_val* eval_context::eval_mod(ast_context p, bool)
{
	let& op = p.h;
	let na = op.num_args();

	//accumulate the result here
	pool_ptr res(eval7(p.index(0), true));

	for (size_t i = 1; i < na; ++i) {
		if (!res) {
			return nullptr;
		}
		pool_ptr ai(eval7(p.index(i), true));
		if (!ai) {
			return nullptr;
		}

		res.reset(eval_helpers::eval_mod(res.get(), ai.get()));
	}

	return res.release();
}

const ims_val* eval_context::eval_pow(ast_context p, bool is_geom)
{
	intptr_t ipw = 0;
	double fpw = 0;

	//eval exponent
	auto e_type = ESUBTYPE::integer;

	let& op = p.h;

	if (op.tt == ETYPE::inv) {
		ipw = -1;
	}else if (op.tt == ETYPE::power_imm) {
		ipw = op.get_pow_exponent_imm();
	} else {
		assert(op.tt == ETYPE::power);
		//geometric calculations
		pool_ptr m(eval7(p.index(1), true));

		if (geom_invalid(m.get(), is_geom)) {
			return nullptr;
		}
		if (!m || !m->is(ims_val::ETP::number)) {
			if (!is_geom)return get_ast_val(p);
			eval_error("pow: invalid exponent");
			return nullptr;
		}
		if (m->is(ims_val::EST::real)) {
			fpw = m->get_real();
			e_type = ESUBTYPE::real;
		}else{
			assert(m->is(ims_val::EST::rational));
			let ival = m->get_int();
			let n = ival.numerator();
			let d = ival.denominator();

			if (d == 1) {
				ipw = intptr_t(n);
				e_type = ESUBTYPE::integer;
			} else {
				fpw = double(n) / d;
				e_type = ESUBTYPE::real;
			}
		}
	}


	if (e_type == ESUBTYPE::integer) {
		if (ipw == 0) {
			return eval_pool::ep.get_id_val();
		}
		if (ipw == 1) {
			return eval7(p.index(0), is_geom);
		}
		if (ipw >= 2 && ipw < 256 && !is_geom) {

			//argument - in topological mode
			pool_ptr arg(eval7(p.index(0), false));
			if (!arg) {
				return get_ast_val(p);
			}

			let na = (size_t)ipw;
			auto* ret = eval_pool::ep.get_vector(na, ims_val::ETP::compos);
			
			auto* dst = ret->p_v();
			for (size_t j = 0; j < na; ++j) {
				dst[j] = arg.get();
				arg->add_ref();				
			}
			return ret;
		}
	} else if (e_type != ESUBTYPE::real) {
		if (!is_geom)return get_ast_val(p);
		eval_error("pow: invalid exponent");
		return nullptr;
	}

	if (!is_geom)return get_ast_val(p);

	//next - geometric mode
	pool_ptr arg(eval7(p.index(0), true));

	//empty map to any degree is empty
	if (arg && arg->is_empty()) {
		return arg.release();
	}

	if (geom_invalid(arg.get(), is_geom)) {
		return nullptr;
	}
	if (arg->gs() >= ims_val::EST::nan) {
		eval_error("pow: invalid base");
		return nullptr;
	}

	if (arg->is(ims_val::ETP::number)) {//base - scalar
		if (e_type == ESUBTYPE::integer) {
			if (arg->is(ims_val::EST::rational)) {
				auto base = arg->get_int();
				if (ipw < 0) {
					let n = base.numerator();
					if (n == 0) {
						ims_error("pow: division by zero");
						return nullptr;
					}
					base = { base.denominator(), n };
					ipw = -ipw;
				}
				return eval_pool::ep.get_scalar_int(::ipow(base, uint8_t(ipw)));
			}
			auto base = arg->get_real();
			if (ipw < 0) {
				base = 1.0 / base;
				ipw = -ipw;
			}
			return eval_pool::ep.get_scalar_real(::ipow(base, uint8_t(ipw)));
			
		}

		double fbase;
		if (!arg->to_real(fbase)) {
			assert(false);
			return nullptr;
		}
		return eval_pool::ep.get_scalar_real(std::pow(fbase, fpw));
	}
	let dim = p.a->get_dim();
	if (arg->is(ims_val::ETP::vector)) {//base - vector
		let sz = arg->get_size();
		if (sz == dim * dim) {//base - matrix
			if (e_type == ESUBTYPE::integer) {
				arg.reset(eval_helpers::to_affine3(arg.get(), dim));
				//continue
			} else {
				ims_error("affine real pow: not implemented");
				return nullptr;
			}
		} else {
			if (e_type == ESUBTYPE::integer) {
				//multiplying the vector arg by the number pw
				if (arg->is(ims_val::EST::rational)) {
					auto* ret = eval_pool::ep.get_vector_int(sz);
					auto* d = ret->p_i();
					let* s = arg->p_i();
					for (size_t i = 0; i < sz; ++i) {
						d[i] = s[i] * ipw;
					}
					return ret;
				}
	
				//continue with real arithmetic
				fpw = ims_val::Real(ipw);
			}

			//raising to a fractional power fpw
			//multiplying the vector arg by the number fpw
			auto* ret = eval_pool::ep.get_vector_real(sz);
			auto* d = ret->p_r();

			if (arg->is(ims_val::EST::rational)) {
				let* s = arg->p_i();
				for (size_t i = 0; i < sz; ++i) {
					let& si = s[i];
					let rsi =
						static_cast<ims_val::Real>(si.numerator()) /
						static_cast<ims_val::Real>(si.denominator());
					d[i] = rsi * fpw;
				}
			} else {
				let* s = arg->p_r();
				for (size_t i = 0; i < sz; ++i) {
					d[i] = s[i] * fpw;
				}
			}

			return ret;
		}
	}

	if (!arg->is_affine()) {
		assert(false);
		return nullptr;
	}

	//raising an affine map to an integer power using multiplication and inversion
	if (ipw < 0) {
		arg.reset(eval_helpers::affine_inv(arg.get()));
		if (!arg) {
			ims_error("the matrix is ​​not invertible");
			return nullptr;
		}
		ipw = -ipw;
	};

	if (ipw == 1) {
		return arg.release();
	};

	//now we need to raise arg to the power e>=2
	let ae = static_cast<uint8_t>(ipw);
	assert(ae >= 2);

	if (arg->is(ims_val::EST::rational)) {//T1=this^ae
		pool_ptr T1(eval_pool::ep.get_affine_int(dim));
		eval_helpers::affine_int_set_to(T1.get(), 1);
		return eval_helpers::mulpow_affine(T1.get(), arg.get(), ae);
	}

	pool_ptr T1(eval_pool::ep.get_affine_real(dim));
	eval_helpers::affine_real_set_to(T1.get(), 1);
	return eval_helpers::mulpow_affine(T1.get(), arg.get(), ae);
}


const oper_block* 
eval_context::get_func_for_call(
	const ims_val* func, 
	bool use_cache, 
	size_t dim, 
	size_t& new_call_offset,
	block_id_t* js_obj)
{
	if (js_obj)*js_obj = block_id_max;

	if (!func || !func->is(ims_val::ETP::ast_ptr)) {
		return nullptr;
	}
	
	let* ast = func->gp<ast_context>();

	if (ast->h.tt == ETYPE::undef) {
		new_call_offset = ast->call_offset;
		return ast->a;
	}

	if (ast->h.tt != ETYPE::unk_reference) {
		return nullptr;
	}

	let unk_id = ast->h.get_unk_id();

	let& lst = m_nfo->m_list;

	let& d = lst.m_idf.m_idx2unknown[unk_id]->second;

	if (d.block_id == block_id_max) {
		if (js_obj)*js_obj = d.js_export_entry;
		return nullptr;
	}

	auto* fb = lst.get_block(d.block_id);
	if (!fb) {
		ims_error("call: block not found: {}", 
			lst.m_idf.get_str_from_unk(unk_id));
		return nullptr;
	}

	if (fb->m_dim2 == 0) {
		//support compatibility - if the block doesn't specify a dimension
		//then it is set by the first caller
		fb->m_dim2 = dim;
	}

	check_block(fb);

	////////////////////////////////////////////////////////////////////////////

	if (use_cache) {
		let it = m_fields_call.find(fb->m_block_id);
		if (it != m_fields_call.end()) {
			new_call_offset = it->second;
			return fb;//previously created
		}
	}

	//initialization, sets up correct references through the hierarchy
	new_call_offset = m_refs5.size();

	let new_sz = new_call_offset + fb->num_vars();
	m_refs5.resize(new_sz);

	for (size_t i = new_call_offset; i < new_sz; ++i) {
		auto& r = m_refs5[i];
		r.c.call_offset = ims_max;//flag that is not ovrriden
	}

	//overlap with the ENTIRE hierarchy
	for (let* b = fb; b; b = b->get_parent()) {
		
		for (let& q : *b) {
			if (q.is_builtin())continue;

			auto& r = m_refs5[q.gr() + new_call_offset];
			if (r.c.call_offset != ims_max) continue;//already overriden

			r.c = { b->get_ptr(q.pos5), new_call_offset };
			r.is_subs = b->ctx()->m_refs5[q.gr()].is_subs;
		}
	}

	if (use_cache) {//remember for the future
		m_fields_call.emplace(fb->m_block_id, new_call_offset);
	}

	return fb;
}


//any occurrence of call is replaced by a specially created new variable
const ims_val* eval_context::eval_call(ast_context p, bool is_geom)
{
	let& op = p.h;

	let ofs = op.get_offset();
	let na = op.num_args();
	assert(na >= 1);

	//calculate the function itself geometrically...
	pool_ptr func(eval7(p.index(0), true));

	if (!func) {
		return nullptr;
	}

	if (na == 1 && func->is(ims_val_b::ETP::vector)) {//arr() == array size
		return eval_pool::ep.get_scalar_int(func->get_size());
	}

	if (na == 2 &&
		!m_vec_stack.empty() &&
		func.get() == m_vec_stack.back().get())
	{
		//$(idx)
		pool_ptr a(eval7(p.index(1), true));
		if (!a) {
			return nullptr;
		}

		int64_t idx = 0;
		if (!a->to_int(idx) || idx < 0) {
			eval_error("$(...): invalid arguments");
			return nullptr;
		}
		let uidx = (size_t)idx;
		if (uidx >= m_vec_stack.size()) {
			eval_error("$(...): invalid arguments");
			return nullptr;
		}
		let* ret = m_vec_stack[m_vec_stack.size() - 1 - uidx].get();
		ret->add_ref();
		return ret;
	}


	let orig_size = m_refs5.size();
	let orig_cache = m_fields_call.size();
	bool revert = false;

	IMS_SCOPE([&] {
		//if something is cached, we don't delete it
		if (revert && orig_cache == m_fields_call.size()) {
			m_refs5.resize(orig_size);
		}
	});

	size_t new_call_offset;

	block_id_t js_obj = block_id_max;

	let* fb = get_func_for_call(
		func.get(),
		na == 1 || op.ts == ESUBTYPE::call_fields, 
		p.a->get_dim(), 
		new_call_offset,
		&js_obj);

	if (js_obj != block_id_max) {
		if (!is_geom)return get_ast_val(p);

		pool_ptr args;
		for (size_t i = 1; i < na; ++i) {
			let* vi = eval7(p.index(i), true);
			if (!vi) return nullptr;
			if (na > 2) {
				if (!args) {
					args.reset(eval_pool::ep.get_vector(na - 1));
				}
				args->p_v()[i - 1] = vi;
			} else {
				args = vi;
			}
		}

		std::string err;
		let* ret = const_cast<ims_info*>(m_nfo)->m_js_engine.
			js_obj_call(js_obj, args.get(), na - 1, err);

		if (!ret) {
			if (err.empty()) {
				err = "invalid return value";
			}
			eval_error2("js call: {}", err);
			return nullptr;
		}

		return ret;
	}

	if (!fb) {
		if (!is_geom)return get_ast_val(p);
		revert = true;
		eval_error("attempt to call an invalid object");
		return nullptr;
	}

	////////////////////////////////////////////////////////////////////////////
	if (op.ts == ESUBTYPE::call_new) {
		
		//overriding fields
		for (size_t i = 1; i < na; i+=2) {
			
			let ai = p.index(i);

			size_t idx;

			if (ai.h.tt == ETYPE::unk_reference) {
				let unk_idx = (size_t)ai.h.get_offset();
				idx = fb->get_var_from_unk(unk_idx);
			} else {
				pool_ptr ki(eval7(ai, true));
				if (!ki) {
					return is_geom? nullptr:get_ast_val(p);
				}
				if (!ki->is(ims_val_b::ETP::number, ims_val_b::EST::rational) ||
					ki->get_int().denominator() != 1 ||
					ki->get_int().numerator() < 0)
				{
					idx = ims_max;
				} else {
					idx = (size_t)ki->get_int().numerator();
				}
				
			}
			if (idx >= fb->num_vars()) {
				revert = true;
				if (!is_geom)return get_ast_val(p);
				ims_error("invalid field {}", i);
				return nullptr;
			}

			auto& d = get_ref4(idx + new_call_offset);
			d.c = p.index(i + 1);
			d.is_subs = true;//consider it as a substitution
		}

		//return value of a special type
		ast_context ret;
		ret.a = fb;//callable block
		ret.call_offset = new_call_offset;//where is it located
		ret.h.tt = ETYPE::undef;//used for identification
		return get_ast_val(ret);
	}

	////////////////////////////////////////////////////////////////////////////
	if (op.ts == ESUBTYPE::call_normal) {
		//return value
		size_t res_id = ims_max;

		//looking for the last one
		for (let& q : *fb) {
			if (q.is_builtin())continue;
			res_id = q.gr();
		}
		if (res_id == ims_max) {//there is nothing but builtins...
			revert = true;
			if (!is_geom)return get_ast_val(p);
			ims_error("call: invalid return value");
			return nullptr;
		}

		//overlap the beginning with the arguments of the current context
		if (na > 1) {
			size_t i = 1;

			for (let& q : *fb) {
				if (q.is_builtin())continue;
				if (i >= na)break; //finished
				auto& d = get_ref4(q.gr() + new_call_offset);
				d.c = p.index(i);
				d.is_subs = true;
				++i;
			}
#ifdef use_eval_arg_cahche
			call_context c;
			c.num_args = na - 1;
			c.start_arg = new_call_offset;
			c.m_block_id = fb->m_block_id;

			for (size_t j = new_call_offset; j < m_refs5.size(); ++j) {
				auto& q = m_refs5[j].c;
				if (q.call_offset == new_call_offset) {
					q.call_offset = ims_max;
				}
			}

			auto it = m_call_cache.emplace(c);

			if (!it.second) {//there was already such a call
				m_refs5.resize(new_call_offset);//restoring
				new_call_offset = it.first->start_arg;
			} else {
				for (size_t j = new_call_offset; j < m_refs5.size(); ++j) {
					auto& q = m_refs5[j].c;
					if (q.call_offset == ims_max) {
						q.call_offset = new_call_offset;
					}
				}
			}
#endif
		}
		
		////////////////////////////////////////////////////
#ifdef use_call_resolver
		if (strict_mode()) {
			m_call_resolver[p] = res_id + new_call_offset;
		}
#endif
		let* v = eval_ref(res_id + new_call_offset, is_geom);
		if (v)v->add_ref();
		return v;
	}

	////////////////////////////////////////////////////////////////////////////
	assert(op.ts == ESUBTYPE::call_fields);

	//indexing by fields
	for (size_t j = 1; j < na; ++j) {
		let unk_idx = (size_t)p.a->get_ptr(ofs + j).h.u32;

		//return value
		let res_id = fb->get_var_from_unk(unk_idx);
		if (res_id == ims_max) {
			revert = true;
			if (!is_geom)return get_ast_val(p);
			ims_error("invalid field {}", j);
			return nullptr;
		}

		//all iterations, except maybe the last one, are calculated geometrically
#ifdef use_call_resolver
		if (strict_mode() && j + 1 == na) {
			m_call_resolver[p] = res_id + new_call_offset;
		}
#endif

		let* v = eval_ref(res_id + new_call_offset, is_geom || j + 1 < na);
		if (v)v->add_ref();
		func.reset(v);

		if (j + 1 < na) {
			fb = get_func_for_call(
				func.get(), 
				true,
				p.a->get_dim(),
				new_call_offset,
				nullptr);
		}
	}

	return func.release();
}

const ims_val* eval_context::eval_exchange(ast_context p, bool)
{
	let n = p.a->get_dim();
	auto* ret = eval_pool::ep.get_affine_int(n);
	eval_helpers::affine_int_set_to(ret, 0);
	auto* da = ret->p_i();
	for (size_t i = n * (n - 1); i > 0; i -= n - 1) {
		da[i] = 1;
	}
	return ret;
};


const ims_val* eval_context::eval_inversion(ast_context, bool)
{
	return eval_pool::ep.alloc_scalar(
		ims_val::ETP::inversion, ims_val::EST::pod);
};

const ims_val* eval_context::eval_thickness(ast_context p, bool)
{
	double arg;
	pool_ptr m(eval7(p.index(0), true));
	if (!m)return nullptr;
	if (!m->to_real(arg)) {
		ims_error("$thickness: invalid argument");
		return nullptr;
	}
	return eval_pool::ep.get_scalar_real(arg, ims_val::ETP::thickness);
};


const ims_val* eval_context::eval_color_style(ast_context p, bool)
{
	double arg;
	pool_ptr m(eval7(p.index(0), true));
	if (!m)return nullptr;
	if (!m->to_real(arg)) {
		ims_error("$style: invalid argument");
		return nullptr;
	}

	return eval_pool::ep.get_scalar_real(arg, ims_val::ETP::style2);
};

const ims_val* eval_context::eval_number_imm(ast_context p, bool)
{
	let& op = p.h;

	if (op.ts == ESUBTYPE::real) {
		return eval_pool::ep.get_scalar_real(op.f32);
	}
	int64_t an[2];
	op.get_int_imm(an[0], an[1]);
	if (an[1] == 0) {
		ims_error("Integer division by zero");
		return nullptr;
	}
	return eval_pool::ep.get_scalar_int({ an[0], an[1] });
};


const ims_val* eval_context::eval_number(ast_context p, bool)
{
	let& op = p.h;

	bool is_rational;
	int64_t an[2];
	ims_val::Real fv;
	if (!p.a->get_val(op, is_rational, fv, an[0], an[1])) {
		ims_error("Not implemented");
		return nullptr;//not implemented
	};

	if (!is_rational) {
		return eval_pool::ep.get_scalar_real(fv);
	}

	if (an[1] != 0) {
		return eval_pool::ep.get_scalar_int({ an[0], an[1] });
	}

	assert(false);
	return nullptr;
};

const ims_val* eval_context::eval_condition(ast_context p, bool is_geom)
{
	let& op = p.h;

	let na = op.num_args();
	for (size_t i = 0; i < na; i += 2) {
		//condition - always in geometric mode
		pool_ptr a(eval7(p.index(i), true));

		if (!a) {
			return is_geom ? nullptr : get_ast_val(p);
		}

		size_t arg = a->is_true() ? 0 : 1;

		if (i + 1 == na) {//step function
			if (!is_geom) {
				p.h.set_small_int(1 - (int32_t)arg);//change p
				return get_ast_val(p);
			}
			return eval_pool::ep.get_scalar_int(1 - arg);
		}

		if (arg == 0 || i + 3 == na) {//calculate only one branch
			return eval7(p.index(i + 1 + arg), is_geom);
		}
	}
	if (!is_geom)return get_ast_val(p);
	assert(false);
	return nullptr;
};

const ims_val* eval_context::eval_call_built_in(ast_context p, bool)
{
	let& op = p.h;

	pool_ptr a0(eval7(p.index(0), true));

	if (!a0) {
		return nullptr;
	}

	auto t = static_cast<BUILTIN_FUNC>(op.get_builtin_func());

	switch (t)
	{
	case BUILTIN_FUNC::arg:
	{
		double x, y;
		if (!a0->to_real(x)) {
			eval_error("arg: invalid argument 1");
			return nullptr;
		}

		pool_ptr a1(eval7(p.index(1), true));
		if (!a1)return nullptr;
		if (!a1->to_real(y)) {
			eval_error("arg: invalid argument 2");
			return nullptr;
		}
		return eval_pool::ep.get_scalar_real(atan2(y, x));
	}
	default:
	{
		double x;
		if (!a0->to_real(x)) {
			let& bf = built_in_func::info[op.get_builtin_func()];
			eval_error2("{}: invalid argument", bf.name);
			return nullptr;
		}

		let v = built_in_func::eval(&x, t);
		return eval_pool::ep.get_scalar_real(v);
	}
	}
};

const ims_val* eval_context::eval_vector_imm(ast_context p, bool)
{
	let& op = p.h;

	let nv = op.num_args();

	assert(nv > 0);

	if (op.ts == ESUBTYPE::real) {
		auto* ret = eval_pool::ep.get_vector_real(nv);
		auto* dst = ret->p_r();
		for (size_t j = 0; j < nv; ++j) {
			dst[j] = p.a->m_ops[op.u32 + j].f64;
		};
		return ret;
	};

	if (op.ts == ESUBTYPE::integer || op.ts == ESUBTYPE::rational) {
		auto* ret = eval_pool::ep.get_vector_int(nv);
		auto* dst = ret->p_i();
		for (size_t j = 0; j < nv; ++j) {
			int64_t an[2];
			p.a->get_int64(an[0], an[1], op.u32 + j, op.ts);
			dst[j] = { an[0],an[1] };
		};
		return ret;
	}

	ims_error("Big numbers are not implemented");
	assert(false);
	return nullptr;
};


const ims_val* eval_context::eval_diagonal(ast_context p, bool)
{
	pool_ptr vec(eval7(p.index(0), true));

	let n = p.a->get_dim();
	if (!vec) {
		return nullptr;
	}

	if (vec->num_vec_length() != n) {
		eval_error("$diagonal: invalid argument");
		return nullptr;
	}

	if (vec->is(ims_val::EST::rational)) {
		auto* ret = eval_pool::ep.get_affine_int(n);
		eval_helpers::affine_int_set_to(ret, 0);
		let* s = vec->p_i();
		auto* d = ret->p_i();
		for (size_t j = 0; j < n; ++j) {
			d[j + n * j] = s[j];
		};
		return ret;
	}

	if (vec->is(ims_val::EST::real)) {
		auto* ret = eval_pool::ep.get_affine_real(n);
		eval_helpers::affine_real_set_to(ret, 0);
		let* s = vec->p_r();
		auto* d = ret->p_r();
		for (size_t j = 0; j < n; ++j) {
			d[j + n * j] = s[j];
		};

		return ret;
	}

	assert(false);
	ims_error("Big numbers are not implemented");
	return nullptr;
};

const ims_val* eval_context::eval_charpoly(ast_context p, bool)
{
	let na = p.h.num_args();
	pool_ptr a0;

	if (na == 1) {
		a0.reset(eval7(p.index(0), true));
	}

	if (!a0) {
		return nullptr;
	}

	pool_ptr v(eval_helpers::to_affine3(a0.get(), p.a->get_dim()));
	if (!v) {
		eval_error("$charpoly: argument is not affine");
	}

	return eval_helpers::affine_charpoly(v.get());
};

const ims_val* eval_context::eval_numden(ast_context p, bool)
{
	let na = p.h.num_args();
	pool_ptr a0;

	if (na >= 1) {
		a0.reset(eval7(p.index(0), true));
	}

	if (!a0) {
		return nullptr;
	}

	if (a0->is(ims_val_b::ETP::number)) {
		switch (a0->gs())
		{
		case ims_val_b::EST::rational:
		{
			pool_ptr ret(eval_pool::ep.get_vector_int(2));
			ret->p_i()[0] = numerator(*a0->p_i());
			ret->p_i()[1] = denominator(*a0->p_i());
			return ret.release();
		}
		case ims_val_b::EST::big_rational:
		{
			pool_ptr ret(eval_pool::ep.get_vector_big_rational(2));
			ret->p_b()[0] = numerator(*a0->p_b());
			ret->p_b()[1] = denominator(*a0->p_b());
			return ret.release();
		}
		case ims_val_b::EST::real:
		{
			if (na < 2) {
				eval_error("$numden: invalid number of arguments");
				return nullptr;
			}
			pool_ptr a1(eval7(p.index(1), true));
			if (!a1) return nullptr;
			double eps = 0;
			if (!a1->to_real(eps) || eps <= 0) {
				eval_error("$numden: invalid argument 2");
				return nullptr;
			}

			auto r = a0->get_real();
			let ir = floor(r);

			pool_ptr ret(eval_pool::ep.get_vector_int(2));
			ims_val_b::Integer n{}, d{};
			if (!farey(r - ir, eps, 100, n, d)) {
				n = 0;
				d = 0;
			} else if (ir != 0) {
				n += int64_t(ir)*d;
				if (std::abs(double(n) / d - r) > eps) {
					n = 0;
					d = 0;
				}
			}

			ret->p_i()[0] = n;
			ret->p_i()[1] = d;

			return ret.release();
		}
		default:
			break;
		}
	}
	eval_error("$numden: invalid argument type");
	return nullptr;
};

const ims_val* eval_context::eval_csg(ast_context p, bool is_geom)
{
	let& op = p.h;

	let na = op.num_args();
	assert(na == 4);

	pool_ptr ret(eval_pool::ep.get_vector(na, ims_val::ETP::csg));

	static constexpr std::initializer_list<std::array<int64_t, 2>> bounds =
	{
		{0, 2},
		{-1, 1},
		{0, 1},
	};

	auto* dst = ret->p_v();
	for (size_t j = 0; j < na; ++j) {
		dst[j] = eval7(p.index(j), is_geom);
		bool err = !dst[j];
		if (!err && j > 0) {
			auto& b = bounds.begin()[j - 1];
			int64_t v;
			err = !dst[j]->to_int(v) || v < b[0] || v > b[1];
		}
		if (err) {
			eval_error2("csg: invalid argument {}", j + 1);
			return nullptr;
		}
	}

	return ret.release();
};


static int64_t adjust_index(int64_t idx, size_t arr_sz)
{
	if (idx >= 0)return idx;
	return idx + (int64_t)arr_sz;
}

static bool adjust_index2(int64_t& idx, size_t arr_sz)
{
	let ret = adjust_index(idx, arr_sz);
	if (ret < 0 || ret >= (int64_t)arr_sz) { return false; }
	idx = ret;
	return true;
}

const ims_val* eval_context::vector_flat(ast_context p, bool is_geom)
{
	pool_ptr a;

	a.reset(eval7(p, is_geom));
	if (!a) {
		return nullptr;
	}

	return eval_helpers::flat(a.get());
}

const ims_val* eval_context::eval_companion(ast_context p, bool)
{
	let& op = p.h;

	//check for correct types and dimensions
	let na = op.num_args();

	//first we calculate all the blocks
	pool_ptr args(eval_pool::ep.get_vector(na, ims_val::ETP::vector));
	auto* da = args->p_v();
	for (size_t j = 0; j < na; ++j) {
		//with ownership
		da[j] = eval7(p.index(j), true);
		if (!da[j]) {
			return nullptr;
		}
	}

	size_t sum_dim = 0;

	for (size_t i = 0; i < na; ++i) {

		auto* ni = da[i];

		if (!ni->is(ims_val::ETP::vector) || ni->gs()>=ims_val::EST::nan)
		{
			eval_error("$companion arguments must be vectors");
			return nullptr;
		}

		let nv = ni->get_size();

		if (nv == 0) {
			ims_error("$companion: empty");
			return nullptr;
		}

		sum_dim += nv;

	}

	auto common_sbt = ims_val::EST::rational;
	for (size_t j = 0; j < na; ++j) {
		auto& v = *da[j];
		if (v.is(ims_val::EST::real)) {
			common_sbt = ims_val::EST::real;
			break;
		}
	}

	if (common_sbt == ims_val::EST::real) {
		//convert all arguments to Real
		for (size_t j = 0; j < na; ++j) {
			auto* v = da[j];
			da[j] = eval_helpers::to_real3(v);
			eval_pool::ep.release(v);
		}
	}

	if (common_sbt == ims_val::EST::rational) {
		//assemble the matrix
		auto* ret = eval_pool::ep.get_affine_int(sum_dim);
		eval_helpers::affine_int_set_to(ret, 0);

		size_t ofs = 0;//where we will write now
		for (size_t i = 0; i < na; ++i) {
			let* ai = da[i];
			let* mi = ai->p_i();
			let d = ai->get_size();
			//filling the extended companion matrix
			for (size_t j = 0; j < d; ++j) {
				ret->affine_int_get_elem(ofs + j, ofs + d - 1) = -mi[j];
				if (j + 1 < d) {
					ret->affine_int_get_elem(ofs + j + 1, ofs + j) = 1;
				}
			};

			ofs += d;
		}

		assert(ofs == sum_dim);

		return ret;
	}

	//almost a complete copy of the block above
	//the only differences are EST::Real and m_float_small_x
	if (common_sbt == ims_val::EST::real) {
		//assemble the matrix
		auto* ret = eval_pool::ep.get_affine_real(sum_dim);
		eval_helpers::affine_real_set_to(ret, 0);

		size_t ofs = 0;//where we will write now
		for (size_t i = 0; i < na; ++i) {
			let* ai = da[i];
			let* mi = ai->p_r();
			let d = ai->get_size();
			//fill the extended companion matrix
			for (size_t j = 0; j < d; ++j) {
				ret->affine_real_get_elem(ofs + j, ofs + d - 1) = -mi[j];
				if (j + 1 < d) {
					ret->affine_real_get_elem(ofs + j + 1, ofs + j) = 1;
				}
			};

			ofs += d;
		}

		assert(ofs == sum_dim);

		return ret;
	}

	
	return nullptr;
};


const ims_val* eval_context::eval_neg(ast_context p, bool)
{
	pool_ptr a0(eval7(p.index(0), true));
	if (!a0) {
		return nullptr;
	}

	//the order of arguments is important: -1*aff != aff *-1
	pool_ptr mu(eval_pool::ep.get_scalar_int(-1));
	auto* ret = eval_helpers::mulx(mu.get(), a0.get(), p.a->get_dim());
	assert(ret);
	return ret;
};

const ast_context* eval_context::resolve_topo_reference(const ast_context* ast)
{
	size_t num = 0;
	while (ast->h.tt == ETYPE::reference) {
		let vidx = ast->h.get_offset() + ast->call_offset;
		//calculate in any case to enumerate dependencies!
		eval_ref(vidx, false);
		ast = &get_ref4(vidx).c;
		if (++num > m_refs5.size()) {
			return nullptr;
		}
	}
	return ast;
}

const ims_val* eval_context::eval_index(ast_context p, bool is_geom)
{
	let& op = p.h;
	let na = op.num_args();
	assert(na > 0);

	if (na == 1) {//arr[] == flat
		if (!is_geom)return get_ast_val(p);
		return vector_flat(p.index(0), true);
	}

	if (na > 2) {//slice
		if (!is_geom)return get_ast_val(p);

		pool_ptr a(eval7(p.index(0), true));
		if (!a || !a->is(ims_val_b::ETP::vector)) {
			return a.release();//ready
		}

		let sz = (int64_t)a->get_size();

		int64_t istart = 0, iend = 0, istep = 1;
		if (na > 3) {
			let p3 = p.index(3);
			if (p3.h.tt != ETYPE::def) {
				pool_ptr a3(eval7(p3, true));//step
				if (!a3 || !a3->to_int(istep) || istep == 0) {
					eval_error("slice: invalid step");
					return nullptr;
				}
			}
		}

		let p1 = p.index(1);
		if (p1.h.tt == ETYPE::def) {
			istart = 0;
		} else {
			pool_ptr a1(eval7(p1, true));
			if (!a1 || !a1->to_int(istart)) {
				eval_error("slice: invalid start postion");
				return nullptr;
			}
			istart = adjust_index(istart, sz);
			istart = std::max(istart, (int64_t)0);
			if (istep < 0)istart = sz - 1 - istart;
		}

		let p2 = p.index(2);
		if (p2.h.tt == ETYPE::def) {
			iend = sz;
		} else {
			pool_ptr a2(eval7(p2, true));
			if (!a2 || !a2->to_int(iend)) {
				eval_error("slice: invalid start postion");
				return nullptr;
			}
			iend = adjust_index(iend, sz);
			iend = std::min(iend, sz);
			if (istep < 0)iend = sz - 1 - iend;
		}

		let astep = std::abs(istep);

		size_t dst_size = 0;//target size
		for (auto i = istart; i < iend; i += (int64_t)astep) {
			++dst_size;
		}

		size_t idx = 0;

		switch (a->gs()) {
		case ims_val_b::EST::rational:
		{
			pool_ptr ret(eval_pool::ep.get_vector_int(dst_size));
			for (auto i = istart; i < iend; i += (int64_t)astep) {
				ret->p_i()[idx++] = a->p_i(istep > 0 ? i : sz - i - 1);
			}
			return ret.release();
		}
		case ims_val_b::EST::big_rational:
		{
			pool_ptr ret(eval_pool::ep.get_vector_big_rational(dst_size));
			for (auto i = istart; i < iend; i += (int64_t)astep) {
				ret->p_b()[idx++] = a->p_b(istep > 0 ? i : sz - i - 1);
			}
			return ret.release();
		}
		case ims_val_b::EST::real:
		{
			pool_ptr ret(eval_pool::ep.get_vector_real(dst_size));
			for (auto i = istart; i < iend; i += (int64_t)astep) {
				ret->p_r()[idx++] = a->p_r(istep > 0 ? i : sz - i - 1);
			}
			return ret.release();
		}
		case ims_val_b::EST::other:
		{
			pool_ptr ret(eval_pool::ep.get_vector(dst_size));
			for (auto i = istart; i < iend; i += (int64_t)astep) {
				let* vi = a->p_v(istep > 0 ? i : sz - i - 1);
				if (vi)vi->add_ref();
				ret->p_v()[idx++] = vi;
			}
			return eval_pool::ep.adjust_vec_type(ret.get());
		}
		default: {
			assert(false);
			return nullptr;
		}
		}
	}

	assert(na == 2);

	//element to index
	pool_ptr v(eval7(p.index(0), is_geom));
	if (!v) {
		if (!is_geom)return get_ast_val(p);
		eval_error("Attempt to index an invalid object");
		return  nullptr;
	};


	//one of the values below is valid
	int64_t idx2{ 0 };//for ordinary indexing
	pool_ptr ai;//for fancy indexing
	size_t asz = 0;

	if (op.tt == ETYPE::index_imm) {
		idx2 = op.get_elem_index();
	} else {
		ai.reset(eval7(p.index(1), true));//geometrically

		if (!ai) {
			if (!is_geom)return get_ast_val(p);
			eval_error("Invalid array index");
			return nullptr;
		}

		if (ai->is(ims_val_b::ETP::vector, ims_val_b::EST::rational)) {
			asz = ai->get_size();
			for (size_t j = 0; j < asz; ++j) {
				if (1 != denominator(ai->p_i(j))) {
					eval_error("Invalid array index");
					return nullptr;
				}
			}
		} else {
			if (!ai->to_int(idx2)) {
				if (!is_geom)return get_ast_val(p);
				eval_error2("Invalid array index type {}", (int)ai->gt());
				return nullptr;
			}
			ai.reset();
		}
	}


	if (!is_geom && v->is(ims_val::ETP::ast_ptr)) {
		let* ast = resolve_topo_reference(v->gp<ast_context>());
		if (!ast) {
			return get_ast_val(p);
		}
		let sz = ast->h.num_args();

		if (ast->h.tt == ETYPE::vector_imm) {
			if (ai) {//fancy indexing
				pool_ptr ret(eval_pool::ep.get_vector(asz));
				for (size_t j = 0; j < asz; ++j) {
					idx2 = numerator(ai->p_i(j));
					if (!adjust_index2(idx2, sz)) {
						return nullptr;
					}
					ret.get_mut()->p_v()[j] =
						get_ast_val({ ast->index_imm(idx2), ast->call_offset });
				}
				return ret.release();
			}
			//ordinary indexing
			if (!adjust_index2(idx2, sz)) {
				return nullptr;
			}
			//successfully indexed
			return get_ast_val({ ast->index_imm(idx2), ast->call_offset });
		}
		if (ast->h.tt == ETYPE::vector) {
			if (ai) {//fancy indexing
				pool_ptr ret(eval_pool::ep.get_vector(asz));
				for (size_t j = 0; j < asz; ++j) {
					idx2 = numerator(ai->p_i(j));
					if (!adjust_index2(idx2, sz)) {
						return nullptr;
					}
					ret.get_mut()->p_v()[j] =
						eval7(ast->index(idx2), is_geom);
				}
				return ret.release();
			}
			//ordinary indexing
			if (!adjust_index2(idx2, sz)) {
				eval_error3("Subscript out of range: {}, {}", idx2, sz);
				return nullptr;
			}
			//successfully indexed
			return eval7(ast->index(idx2), is_geom);
		}
		v.reset(eval7(*ast, is_geom));
	}


	if (geom_invalid(v.get(), is_geom)) {
		return nullptr;
	}

	if (!v || !v->is(ims_val::ETP::vector)) {
		if (!is_geom)return get_ast_val(p);
		eval_error2("Attempt to index a non-vector object: {}", (size_t)v->gt());
		return  nullptr;
	}

	let sz = v->get_size();

	if (ai) {

		switch (v->gs())
		{
		case ims_val::EST::rational:
		{
			pool_ptr ret(eval_pool::ep.get_vector_int(asz));
			for (size_t j = 0; j < asz; ++j) {
				idx2 = numerator(ai->p_i(j));
				if (!adjust_index2(idx2, sz)) {
					return nullptr;
				}
				ret.get_mut()->p_i()[j] = v->p_i(idx2);
			}
			return ret.release();
		}
		case ims_val::EST::big_rational:
		{
			pool_ptr ret(eval_pool::ep.get_vector_int(asz));
			for (size_t j = 0; j < asz; ++j) {
				idx2 = numerator(ai->p_i(j));
				if (!adjust_index2(idx2, sz)) {
					return nullptr;
				}
				ret.get_mut()->p_b()[j] = v->p_b(idx2);
			}
			return ret.release();
		}
		case ims_val::EST::real:
		{
			pool_ptr ret(eval_pool::ep.get_vector_int(asz));
			for (size_t j = 0; j < asz; ++j) {
				idx2 = numerator(ai->p_i(j));
				if (!adjust_index2(idx2, sz)) {
					return nullptr;
				}
				ret.get_mut()->p_r()[j] = v->p_r(idx2);
			}
			return ret.release();
		}
		default:
		{
			pool_ptr ret(eval_pool::ep.get_vector(asz));
			for (size_t j = 0; j < asz; ++j) {
				idx2 = numerator(ai->p_i(j));
				if (!adjust_index2(idx2, sz)) {
					return nullptr;
				}
				auto* nv = v->p_v()[idx2];//indexing
				if (nv)nv->add_ref();
				ret.get_mut()->p_v()[j] = nv;
			}
			return ret.release();
		}
		}
	};

	if (!adjust_index2(idx2, sz)){
		if (!is_geom)return get_ast_val(p);
		eval_error3("Subscript out of range: {}, {}", idx2, sz);
		return  nullptr;
	}

	//indexing...
	switch (v->gs())
	{
	case ims_val::EST::rational:
	{
		return eval_pool::ep.get_scalar_int(v->p_i()[idx2]);//indexing
	}
	case ims_val::EST::big_rational:
	{
		auto* ret_big = eval_pool::ep.get_scalar_big_rational();
		*ret_big->p_b() = v->p_b(idx2);//indexing
		return ret_big;
	}
	case ims_val::EST::real:
	{
		return eval_pool::ep.get_scalar_real(v->p_r()[idx2]);//indexing
	}
	default:
		auto* nv = v->p_v()[idx2];//indexing
		if (nv)nv->add_ref();
		v.reset(nv);
		break;
	}

	return  v.release();
};

static const ims_val*
convert_vector_to_operator(const ims_val* a, size_t level)
{
	if (!a) {
		return nullptr;
	}

	if (!a->is(ims_val_b::ETP::vector) || !a->is(ims_val_b::EST::other)) {
		a->add_ref();
		return a;
	}

	let sz = a->get_size();
	if (sz == 0) {
		return eval_pool::ep.get_empty_val();
	}

	bool has_vectors = false;
	for (size_t i = 0; i < sz; ++i) {
		let* ai = a->p_v(i);
		if (!ai)return nullptr;
		if (ai->is(ims_val_b::ETP::vector)) {
			has_vectors = true;
			break;
		}
	}

	if (!has_vectors) {
		a->add_ref();
		return a;
	}

	if (sz == 1) {
		return convert_vector_to_operator(a->p_v(0), level + 1);
	}

	//create an operator
	pool_ptr res(eval_pool::ep.get_vector(sz,
		(level%2==0)? ims_val::ETP::uni: ims_val::ETP::compos));

	auto* dst = res->p_v();
	for (size_t i = 0; i < sz; ++i) {
		let* vi = convert_vector_to_operator(a->p_v(i), level+1);
		if (!vi)return nullptr;
		vi->add_ref();
		dst[i] = vi;
	}
	return res.release();
}

const ims_val* eval_context::eval_vector_uni(ast_context p, bool is_geom)
{
	if (is_geom) {
		return nullptr;
	}

	pool_ptr a(eval7(p.index(0), true));//in the geometric mode

	if (!a || !a->is(ims_val_b::ETP::vector, ims_val_b::EST::other)) {
		return a.release();//ready
	}

	if (a->get_size() == 0) {
		return eval_pool::ep.get_empty_val();
	}

	let* ret = convert_vector_to_operator(a.get(), 0);
	if (ret && !ret->is_empty()) {
		++m_topo_unions;//even in geometric mode - we will take it into account
	}

	return ret;
}


const ims_val* eval_context::eval_uni(ast_context p, bool is_geom)
{
	let& op = p.h;

	let na = op.num_args();

	if (na == 0) {
		return eval_pool::ep.get_empty_val();
	}
	if (na == 1) {
		return eval7(p.index(0), is_geom);
	}

	++m_topo_unions;//even in geometric mode - we will take it into account

	if (is_geom) {
		return nullptr;
	}

	auto* ret = eval_pool::ep.get_vector(na, ims_val::ETP::uni);

	auto* dst = ret->p_v();
	for (size_t j = 0; j < na; ++j) {
		auto* v = eval7(p.index(j), is_geom);
		assert(v);
		dst[j] = v;
	}
	return ret;
};



const ims_val* eval_context::eval_vector(ast_context p, bool is_geom)
{
	let na = p.h.num_args();

	//reserve na elements
	m_vec_stack.emplace_back(eval_pool::ep.get_vector(na, ims_val::ETP::vector));
	IMS_SCOPE([&] {m_vec_stack.pop_back(); });

	pool_ptr vec = m_vec_stack.back();

	//increase during iterations
	vec.get_mut()->shrink(0);

	auto set_vector = [&](const ims_val* v) -> bool
	{
		if (v == vec.get()) {
			eval_pool::ep.release(v);
			return false;
		};
		let cur_pos = vec->get_size();
		vec = eval_pool::ep.update_vec_size(vec.get_mut(), cur_pos + 1);
		m_vec_stack.back() = vec;//update reference
		vec->p_v()[cur_pos] = v;//take ownership
		return true;
	};

	//calculate the components
	for (size_t i = 0; i < na; ++i) {
		if (set_vector(eval7(p.index(i), is_geom))) {
			continue;
		}

		//generator mode
		if (i + 2 >= na) {
			eval_error("invalid recursive vector");
			return nullptr;
		}

		let cptr = p.index(i + 1);
		let vptr = p.index(i + 2);

		i += 2;

		static constexpr size_t MAX_ELEMS_TO_GEN = 1024 * 1024;

		for (;;) {
			if (vec->get_size() > MAX_ELEMS_TO_GEN + na) {
				eval_error("recursive vector is too long");
				return nullptr;
			}
			pool_ptr vcond(eval7(cptr, true));
			if (!vcond) {
				return nullptr;
			}
			if (!vcond->is_true() || !set_vector(eval7(vptr, true))) {
				break;//end of the loop
			}
		}
	}
	return eval_pool::ep.adjust_vec_type(vec.get());
}

const ims_val* eval_context::eval_mul(ast_context p, bool is_geom)
{
	let& op = p.h;

	let na = op.num_args();

	if (na == 0) {
		return eval_pool::ep.get_id_val();
	}

	if (na == 1) {
		return eval7(p.index(0), is_geom);
	}

	if (!is_geom) {//create a composition
		auto* ret = eval_pool::ep.get_vector(na, ims_val::ETP::compos);

		auto* dst = ret->p_v();
		for (size_t j = 0; j < na; ++j) {
			auto* v = eval7(p.index(j) , is_geom);
			assert(v);
			dst[j] = v;
		}
		return ret;
	}

	//calculate the composition
	pool_ptr prod(eval7(p.index(0), true));

	for (size_t i = 1; i < na; ++i) {
		if (!prod) {
			return nullptr;
		}
		pool_ptr ai(eval7(p.index(i), true));
		if (!ai) {
			return nullptr;
		}
		prod.reset(eval_helpers::mulx(prod.get(), ai.get(), p.a->get_dim()));
	}

	assert(prod);

	
	if (strict_mode() && prod->is(ims_val_b::ETP::compos) && prod->get_size() > 0) {
		return nullptr;//compositions in strict geometric mode is prohibited
	}
	
	return prod.release();
};



const ims_val* eval_context::eval_sum(ast_context p, bool)
{
	let& op = p.h;

	let& na = op.num_args();

	//accumulate the sum here
	pool_ptr sum(eval7(p.index(0), true));

	for (size_t i = 1; i < na; ++i) {
		if (!sum) {
			return nullptr;
		}
		pool_ptr ai(eval7(p.index(i), true));
		if (!ai) {
			return nullptr;
		}
		sum.reset(eval_helpers::sum_affine(sum.get(), ai.get(), p.a->get_dim()));
	}
	
	return sum.release();
};


const ims_val* eval_context::eval_empty(ast_context, bool)
{
	return eval_pool::ep.get_empty_val();
};

const ims_val* eval_context::eval_this_vector(ast_context, bool)
{
	if (m_vec_stack.empty()) {
		return nullptr;
	}
	auto* ret = m_vec_stack.back().get();
	assert(ret);
	ret->add_ref();
	return ret;
};

const ims_val* eval_context::eval_id(ast_context, bool)
{
	return eval_pool::ep.get_id_val();
};

const ims_val* eval_context::eval_pi(ast_context, bool)
{
	return eval_pool::ep.get_scalar_real(boost::math::constants::pi<ims_val_b::Real>());
};

const ims_val* eval_context::eval7(ast_context p, bool is_geom)
{
	if (!is_geom && !ims_operator::is_topo_eval(p.h.tt)) {
		if (p.h.is_closed()) {
			++m_topo_cycles;//closed ones are considered looped
		}
		return get_ast_val(p);
	}

	const ims_val* ret = nullptr;

	switch (p.h.tt) {
	case ETYPE::empty:			ret = eval_empty(p, is_geom); break;
	case ETYPE::id:				ret = eval_id(p, is_geom); break;
	case ETYPE::pi:				ret = eval_pi(p, is_geom); break;
	case ETYPE::reference:		ret = eval_reference(p, is_geom); break;
	case ETYPE::exchange:		ret = eval_exchange(p, is_geom); break;
	case ETYPE::inversion:		ret = eval_inversion(p, is_geom); break;
	case ETYPE::call:			ret = eval_call(p, is_geom); break;
	case ETYPE::thickness:		ret = eval_thickness(p, is_geom); break;
	case ETYPE::color_style:	ret = eval_color_style(p, is_geom); break;
	case ETYPE::number_imm:		ret = eval_number_imm(p, is_geom); break;
	case ETYPE::number:			ret = eval_number(p, is_geom); break;
	case ETYPE::condition:		ret = eval_condition(p, is_geom); break;
	case ETYPE::call_built_in:	ret = eval_call_built_in(p, is_geom); break;
	case ETYPE::diagonal:		ret = eval_diagonal(p, is_geom); break;
	case ETYPE::charpoly:		ret = eval_charpoly(p, is_geom); break;	
	case ETYPE::numden:			ret = eval_numden(p, is_geom); break;
	case ETYPE::csg:			ret = eval_csg(p, is_geom); break;
	case ETYPE::vector_union:	ret = eval_vector_uni(p, is_geom); break;
	case ETYPE::vector:			ret = eval_vector(p, is_geom); break;
	case ETYPE::vector_imm:		ret = eval_vector_imm(p, is_geom); break;
	case ETYPE::this_vector:	ret = eval_this_vector(p, is_geom); break;
	case ETYPE::companion:		ret = eval_companion(p, is_geom); break;
	case ETYPE::mod:			ret = eval_mod(p, is_geom); break;
	case ETYPE::power_imm:		ret = eval_pow(p, is_geom); break;
	case ETYPE::power:			ret = eval_pow(p, is_geom); break;
	case ETYPE::inv:			ret = eval_pow(p, is_geom); break;
	case ETYPE::neg:			ret = eval_neg(p, is_geom); break;
	case ETYPE::index_imm:		ret = eval_index(p, is_geom); break;
	case ETYPE::index:			ret = eval_index(p, is_geom); break;
	case ETYPE::uni:			ret = eval_uni(p, is_geom); break;
	case ETYPE::mul:			ret = eval_mul(p, is_geom); break;
	case ETYPE::sum:			ret = eval_sum(p, is_geom); break;
	case ETYPE::unk_reference:	ret = eval_unk_ref(p, is_geom); break;
	case ETYPE::set_interval:
	case ETYPE::set_vector:
	case ETYPE::set_permutation:
	{
		if (!is_geom) {
			ret = get_ast_val(p);
		} else {
			++m_geom_templates;
			ret = nullptr;
		}
		break;
	}
	default: ret = get_ast_val(p); break;

	}
	
	//here you can check conditions on the return value
#if 0
	if (!is_geom && ret->is(ims_val::ETP::scalar)) {
		assert(false);
	};
#endif
	return ret;
}

#ifdef use_eval_arg_cahche

bool eval_context::call_hasher::operator()(call_context c1, call_context c2) const
{
	if (c1.m_block_id != c2.m_block_id)return false;
	if (c1.num_args != c2.num_args)return false;

	let* a1 = &m_ec.m_refs5[c1.start_arg];
	let* a2 = &m_ec.m_refs5[c2.start_arg];

	for (size_t i = 0; i < c1.num_args; ++i) {
		if (ast_context::lexic_compare( a1[i].c, a2[i].c) != 0) {
			return false;
		}
	}
	return true;
}

size_t eval_context::call_hasher::operator()(call_context c) const
{
	size_t ret = c.m_block_id;
	boost::hash_combine(ret, c.num_args);
	let* a = &m_ec.m_refs5[c.start_arg];
	for (size_t i = 0; i < c.num_args; ++i) {
		ast_context::hash_combine(a[i].c, ret);
	}
	return ret;
}
#endif

void eval_stack::push(size_t ref)
{
	if (!stack.empty()) {
		m_edges.push_back({ stack.back(), ref, 0 });
	}
	stack.push_back(ref);
}

void eval_stack::pop()
{
	stack.pop_back();
}
