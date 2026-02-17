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
#include "ast_stack.h"
#include "eval_context.h"
#include "matrix_group.h"
#include "block_class.h"

#include "ims_val.h"
#include "eval_helpers.h"
#include "variable.h"

#include "graph_builder.h"
#include "graph_init_data_ptr.h"
#include "block_graph.h"
#include "pool_ptr.h"
#include "ovr_data.h"


//	&R = $semigroup([s,r])
//		==========>
//	S = [1, s, s^2, s^3, r, r*s, r*s^2, r*s^3]
//	&R = S[$number($integer(0, 7))]

static bool replace_semigroups(oper_block& b)
{
	if (b.m_ops.empty())return false;

	auto& ec = *b.ctx();

	matrix_group mgr;
	ast_stack ai;
	DynMat<ims_val::Rational::int_type> tmpx;
	std::vector<ast_context> gens_ptrs;
	std::vector<size_t> semigroups;
	

	for (let& q : b) {
		if (q.is_builtin())continue;
		//need to use addresses of ims_operators, hence the offsets!
		for (auto& x : ai.reset3(ast_stack::value({ &b, &b.m_ops[q.pos5].hdr, 0 }))) {
			if (x.h->tt == ETYPE::set_semigroup) {
				semigroups.emplace_back(x.h - &b.m_ops[0].hdr);
			}
		}
	}

	if (semigroups.empty())return false;

	for (let spos : semigroups) {
		let x = ast_context(b.get_ptr(spos), 0);

		bool err = false;
		mgr.clear();

		let arg = x.index(0);//1st argument
		assert(arg.h.tt == ETYPE::reference);
		size_t order = ims_max;

		let ref_idx = arg.get_ref_idx();

		IMS_SCOPE([&] {
			if (err) {
				ims_error("Invalid semigroup");
				b.m_ops[spos].hdr.set_small_int(0);
			} else {

				let ar = b.add_args(spos, ETYPE::index, 2);
				b.m_ops[ar].hdr.set_reference(ref_idx);

				let da = b.set_binary_or_vector(ar + 1, ETYPE::set_interval, 0);
				b.set_distribution(da, ETYPE::distribution_int,
					ESUBTYPE::dist_uniform, 0, double(order) - 1);
			}
		});

		size_t pos = ims_max;

		for (let& v : b) {
			if (v.is_builtin())continue;
			if (v.gr() == arg.h.get_offset()) {
				pos = v.pos5;
				break;
			}
		}

		if (pos == ims_max) {
			err = true;
			continue;
		}

		auto ap = arg;
		ap.h = b.m_ops[pos].hdr;


		if (ap.h.tt != ETYPE::vector && ap.h.tt != ETYPE::vector_imm) {
			err = true;
			continue;
		}


		let na = ap.h.num_args();//number of generators

		//calculate generators
		gens_ptrs.clear();
		for (size_t i = 0; i < na; ++i) {
			let qi = ap.h.tt == ETYPE::vector? ap.index(i): ast_context(ap.index_imm(i), ap.call_offset);
			pool_ptr v(ec.eval7(qi, true));
			if (!v) {
				err = true;
				break;
			}

			pool_ptr qv(eval_helpers::to_affine3(v.get(), qi.a->get_dim()));

			if (!qv || !qv->is(ims_val::EST::rational)) {

				err = true;
				break;
			}

			eval_helpers::to_ext_integer(qv.get(), tmpx);

			mgr.add_generator(tmpx);
			gens_ptrs.emplace_back(qi);
		}

		if (err || mgr.m_gens.empty()) {
			ims_error("Invalid semigroup");
			err = true;
			continue;
		}

		//mgr.sort_gens();

		mgr.init();
		//generate the initial number of elements

		constexpr size_t max_order = 384;
		constexpr size_t max_weight = 3;

		while (mgr.extend()) {
			if (mgr.m_elems.size() > max_order) {
				break;
			}
		};

		auto accepted = [&](size_t w) {
			return mgr.m_elems.size() <= max_order || w <= max_weight;
		};

		order = 0;
		for (let* e : mgr.m_elems) {
			if (accepted(e->second.weight)) {
				++order;
			}
		}

		auto ds = b.add(order);

		b.m_ops[pos].hdr.tt = ETYPE::vector;
		b.m_ops[pos].hdr.set_u24(order);
		b.m_ops[pos].hdr.set_u32(ds);

		for (size_t j = 0; j < mgr.m_elems.size(); ++j) {
			let& e = mgr.m_elems[j]->second;

			if (!accepted(e.weight))continue;
			
			
			let nf = e.factors.size();
			let ar = b.add_args(ds++, ETYPE::mul, nf);

			//linear part - product of generators
			for (size_t i = 0; i < nf; ++i) {
				let& f = e.factors[i];
				let& gen = gens_ptrs[f.idx];

				if (f.pw == 1) {
					b.insert_op_ex(ar + i, gen, ec);
				} else {
					let ari = b.add_args(ar + i, ETYPE::power_imm, 1);
					b.m_ops[ar + i].hdr.set_i24(f.pw);
					b.insert_op_ex(ari, gen, ec);
				}
			}
		}
		err = false;
	}

	b.m_src2.reset();//invalidated

	return true;//the semigroup is replaced
}


static bool create_graph(oper_block& b, eval_stack& es)
{
	b.m_graph = std::make_shared<block_graph>();
	b.m_ctx = std::make_shared<eval_context>();//may change later

	auto& ec = *b.m_ctx;
	ec.set_own_block(b, &es);

	if (replace_semigroups(b)) {
#if defined(DEVELOPER_VERSION)
		ims_warning("Semigroups were replaced.");
#endif
		ec.set_own_block(b, &es);//recreate because it changed
	}

	let user_vars = b.num_vars();

	//calculate all variables topologically
	bool hv = false;
	ec.m_stack = &es;
	for (size_t i = 0; i < user_vars; ++i) {
		ec.eval_ref(i, false);
		if (ec.m_refs5[i].is_var()) {
			hv = true;
		}
	}
	ec.m_stack = nullptr;
	ec.m_geom_templates = hv ? 1 : 0;

	////////////////////////////////////////////////////////////////////////////
	//creating a graph
	auto& g = *b.get_graph();

	graph_builder gb2;
	graph_init_data_ptr idata;
	
	if (!gb2.create(g, user_vars, ec.m_refs5)) {
		b.m_graph.reset();//error indicator
		b.m_ctx.reset();
		return false;
	}

	g.m_g1.init(idata.get());

	g.m_deps.m_edges = std::move(es.m_edges);
	g.m_deps.init(idata.get(), user_vars, true);

	return true;
}



#include "ims_info.h"
#include "ims_view.h"
bool call_js_init(oper_block&, size_t, ims_view<operator_ptr>);

//relative to parent, we create:
//class, if new variables have appeared
//graph, if a new class has appeared or the geometricity of any variable has changed
//context, if a new graph exists or if there are templates in the graph edges
//returns false if the block cannot be materialized
bool check_block_ex(oper_block& sr, eval_context& ec, ast_maps& am)
{
	ast_stack ai;
	eval_stack es;

	assert(!sr.m_flags.ready);

	//initialize the entire hierarchy
	boost::container::small_vector<oper_block*, 3> parr;//for temporary needs

	for (auto* p = &sr; p; ) {
		
		if (p->m_flags.ready)break;

		p->fix_js_parent();//TODO maybe not here

		p->m_ctx.reset();
		p->m_graph.reset();

		parr.emplace_back(p);

		p = const_cast<oper_block*>(p->get_parent());
	}

	bool need_set_block = true;

	while (!parr.empty()) {//can grow during iterations!

		auto* b = parr.back();

		let was_ready = b->m_flags.ready;

		if (b->m_flags.ready) {//second pass
			parr.pop_back();
		} else {
			b->m_flags.ready = true;
		}

		if (was_ready && !b->has_js_parent()) {
			assert(!b->is_invalid());
			assert(!b->m_flags.priv);

			size_t js_init = ims_max;
			for (let* p = b; p; p = p->get_parent()) {
				if (p->m_flags.priv) {
					break;//important
				}
				js_init = p->m_js_init;
				if (js_init != ims_max) {
					break;
				}
			}
			if (js_init == ims_max) {
				continue;//the block is ready
			}

			static ovr_data opod;//TODO reuse
			opod.init(*b);

			let sz = b->num_vars();
			for (size_t i = 0; i < sz; ++i) {
				if (!b->ctx()->m_refs5[i].is_var()) {
					opod.m_arr[i].p.h.set_xundef();
				}
			}

			ims_view v(&opod.m_arr.data()->p, sizeof(ovr_data::elem));
			
			let res = call_js_init(*b, js_init, v);

			if (!res) {
				continue;//ignore $init if any JavaScript error occurred
			}

			b->m_graph.reset();
			b->m_ctx.reset();

			assert(b->m_js_parent);

			auto* p = b->m_js_parent.get();
			p->m_flags.ready = true;
			ims_info::link_refs_for_block(*b->get_class()->m_nfo, p, ai);

			let active_ref = b->get_active_ref();//let's remember
			b->set_parent(p);
			b->m_class.reset();//use js_parent
			b->set_active_ref(active_ref);

			assert(!p->m_graph);
			assert(!p->m_ctx);
			
			//In this iteration, process the new parent element without
			//inserting it into parr, then process block b again.
			parr.emplace_back(b);
			b = p;
		}

		assert(b->get_class());

		let* p = b->get_parent();
		
		if (p && p->is_invalid()) {
			return false;
		}

		bool need_graph = !p || p->get_class() != b->get_class();

		if (!need_graph) {

			//try to reuse
			b->m_graph = p->m_graph;
			b->m_ctx = p->m_ctx;

			if (b->m_flags.only_view) {
				//assert(b->m_flags.only_var);
				b->m_flags.free_var = p->m_flags.free_var;
				continue;//processing completed
			}

			//here is the VERY ESSENCE OF CHECKING (topological compliance)...
			need_graph = !ec.set_block(*b);
		}

		if (need_graph) {
			if (create_graph(*b, es)) {
				assert(b->own_ctx());
				continue;
			}
			//fatal error, the rest will automatically receive a new status
			b->m_graph.reset();
			b->m_ctx.reset();
			if (ims_need_stop()) {
				b->m_flags.ready = false;
			}
			return false;
		}

		assert(!b->own_ctx());

		am.inherit(b->m_graph->m_am, ec.m_refs5);

		if (am.has_tempaltes()) {
			auto nctx = std::make_shared<eval_context>();
			nctx->set_own_block(*b, &es);
			b->m_ctx = nctx;
			continue;
		}

		if (b == &sr) {//last iteration
			need_set_block = false;//successful: ec.set_block(*b) && inherit
		}

		//fill the free_var and only_var flags
		//here, the eval_context ec is correctly configured for the current block
		//all graph edges in it are topologically calculated

		assert(!b->own_ctx());

		let sz = b->num_vars();

		size_t num = 0;
		for (size_t i = 0; i < sz; ++i) {
			if (ec.m_refs5[i].is_var()) {
				++num;
			}
		}

		size_t var_over = 0;//how many variations were overriden

		b->m_flags.only_var = true;

		for (let* x = b; !x->own_ctx(); x = x->get_parent()) {

			for (let& q : *x) {
				if (q.is_builtin())continue;

				if (ec.m_refs5[q.gr()].is_var()) {
					var_over++;
				} else if (x == b) {
					b->m_flags.only_var = false;
				}
			}
		}

		b->m_flags.free_var = (var_over < num);
		
	}

	if (!need_set_block) {
		return true;//everything was done inside the loop above
	}

	//we get here, for example, if the block is overriden by a template (replace_ctx.aifs)

	if (!ec.set_block(sr)) {
		assert(false);//not sure if we can get here
		return false;
	}

	am.inherit(sr.m_graph->m_am, ec.m_refs5);
	return true;
};

void check_block(const oper_block* sr)
{
	if (sr->m_flags.ready)return;
	thread_local eval_context t_eval_context;
	thread_local ast_maps t_ast_maps;
	check_block_ex(const_cast<oper_block&>(*sr), t_eval_context, t_ast_maps);
}
