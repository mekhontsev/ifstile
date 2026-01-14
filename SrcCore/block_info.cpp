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
#include "block_info.h"
#include "affine_calc.h"
#include "oper_block.h"
#include "math_helpers.h"
#include "eval_info.h"
#include "matrix_helper.h"
#include "eval_helpers.h"
#include "ims_val.h"
#include "block_graph.h"
#include "edge_map.h"
#include "edge_ball.h"
#include "dfs.h"
#include "eval_context.h"
#include "variable.h"
#include "ast_maps.h"
#include "eval_pool.h"

bool check_block_ex(oper_block& b, eval_context& ec, ast_maps& am);


void block_info::gen_next_id()
{
	ims_func_static size_t g_next_id = 1;
	m_id8 = g_next_id++;
};

void block_info::recalc_graph()
{
	m_id8 = 0;
}

bool block_info::exists() const
{
	return m_cur_fg != nullptr;
}

bool block_info::init4(
	const oper_block& b,
	eval_context& ec,
	ast_maps& am,
	bool checked,
	graph_init_data& gid,
	affine_calc& ac)
{
	bool err = true;

	assert(m_id8 == 0);
	IMS_SCOPE([&] {

		gen_next_id();

		if (err) {
			m_cur_fg = nullptr;
		}
	});

	if (!b.m_flags.ready) {
		if (!check_block_ex(const_cast<oper_block&>(b), ec, am)){
			return false;
		}
	} else {
		if (!b.m_graph || !ec.set_block(b)) {
			return false;
		}
		am.inherit(b.m_graph->m_am, ec.m_refs5);
	}
	
	
	////////////////////////////////////////////////////////////////////////////
	//calculate the atoms - only those that are referenced by the edges

	m_proj_data.recheck();

	ims_resize(m_atom_data, am.m_atoms.size());

	for (size_t i = 0; i < m_atom_data.size(); ++i) {
		let& ast = am.m_atoms[i];
		if (!ast_maps::atom_is_used(ast)) continue;

		auto& a = m_atom_data[i];

		if (i < am.m_num_refs) {
			a.m_v = ec.m_refs5[i].v[0];
		}

		a.eval(ec, ast);
		a.project(ec, ast, m_proj_data, checked);
	}

	////////////////////////////////////////////////////////////////////////////

	let& gg = *b.get_graph();
	let nmaps = gg.m_am.m_ixm.m_maps.size();

	//define the emptiness of the edges
	bool has_empty_maps = false;
	ims_resize(m_map_info, nmaps);

	for (size_t i = 0; i < nmaps; ++i) {
		auto& mi = m_map_info[i];

		let& m = am.get_map(i);
		for (size_t j = 0; j < m.num; ++j) {
			let& a = get_atom(j + m.start, am);

			if (a.m_v && a.m_v->is_empty()) {
				mi.status = map_info::empty;
				has_empty_maps = true;
				break;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	//vertex processing
	let& g = gg.m_g1;
	let nvers = g.num_ver();

	ims_resize(m_ver_dim, nvers);

	////////////////////////////////////////////////////////////////////////////
	//construct a graph by skipping empty edges and vertices
	if (has_empty_maps) {

		//DFS - find empty vertices
		auto pred = [&](const ims_edge& e) {return m_map_info[e.m].status != map_info::empty; };
		for (size_t v = m_dfs.init(g, pred); v < nvers; v = m_dfs.next(g)) {
			let ne = g.num_edges(v);
			bool all_empty = true;
			for (size_t e = 0; e < ne; ++e) {
				let& qe = g.get_edge(v, e);
				if (m_map_info[qe.m].status != map_info::empty &&
					m_ver_dim[qe.second] != ims_max)
				{
					all_empty = false;
					break;
				}
			}
			if (all_empty) {
				m_ver_dim[v] = ims_max;//empty
			}
		}

		//discard only empty maps and vertices,
		//but we leave incorrect maps
		if (!init_fg(g, true)) {
			return true;
		};
		m_fg.init(gid, g.num_ver());
	} else {
		m_cur_fg = &g;
	}

	////////////////////////////////////////////////////////////////////////////

	for (size_t i = 0; i < nmaps; ++i) {
		auto& mi = m_map_info[i];
		if (mi.status == map_info::empty)continue;

		let& m = am.get_map(i);
		for (size_t j = 0; j < m.num; ++j) {
			let& a = get_atom(j + m.start, am);


			if (a.invalid()) {
				mi.status = map_info::invalid;
				break;
			}

			mi.dim_p = std::max(mi.dim_p, a.get_dim());
		}
	}
	////////////////////////////////////////////////////////////////////////////
	//find the correctness and dimension of the vertices
	//there are no empty vertices in the graph components here
	//and there are no edges leading to empty vertices
	let& cg = get_fg();

	bool has_invalid_vers = false;

	for (size_t comp_idx = 0; comp_idx < cg.m_comp.size(); ++comp_idx) {
		auto& c = cg.m_comp[comp_idx];

		bool comp_valid = true;
		size_t dim = 0;//dimension of the component set

		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = cg.m_ver_sorted[j + c.idx_sorted];
			let ne = cg.num_edges(v);
			assert(ne != 0);

			for (size_t e = 0; e < ne; ++e) {
				let& qe = cg.get_edge(v, e);
				let& mi = m_map_info[qe.m];
				let tdim = m_ver_dim[qe.second];
				assert(m_ver_dim[qe.first] == 0);
				if (mi.status == map_info::invalid || tdim == ims_max) {
					comp_valid = false;//the component depends on bad edge
					goto lab_exit_loop;
				}
				dim = std::max(dim, mi.dim_p);
				dim = std::max(dim, tdim);
			}
		}
	lab_exit_loop:;
		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = cg.m_ver_sorted[j + c.idx_sorted];
			auto& vi = m_ver_dim[v];
			if (comp_valid) {
				vi = dim;
			} else {
				vi = ims_max;
				has_invalid_vers = true;
			}
		}
	}

	if (has_invalid_vers) {
		if (!init_fg(g)) {
			return true;
		}
		m_fg.init(gid, g.num_ver());
	}

	any_to_used(g.m_edges);

	if (get_fg().empty()) {
		return true;
	}

	////////////////////////////////////////////////////////////////////////////
	ims_resize(m_em, nmaps);


	//find projected maps
	//don't generate nested compositions
	for (size_t i = 0; i < nmaps; ++i) {
		let& mi = m_map_info[i];
		if (mi.status != map_info::used) {
			continue;
		}

		auto& emi = m_em[i];
		emi.m = create_proj_map(i, am);
		emi.used = true;
	}

	////////////////////////////////////////////////////////////////////////////
	//TODO: should be reworked
	m_style.resize(nmaps);
	for (size_t i = 0; i < nmaps; ++i) {
		m_style[i].pal_idx.clear();
	}

	for (size_t i = 0; i < nmaps; ++i) {
		let& mi = m_map_info[i];
		if (mi.status != map_info::used) {
			continue;
		}
		auto& emi = m_em[i];

		let* m = emi.m.get();

		const ims_val** sa;
		size_t na;
		if (m->is(ims_val::ETP::compos)) {
			sa = m->p_v();
			na = m->get_size();
		} else {
			sa = &m;
			na = 1;
		}

		for (size_t j = 0; j < na; ++j) {
			let* v = sa[j];
			if (v->is(ims_val::ETP::style2)) {
				m_style[i].pal_idx.push_back(v->get_real());
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	//affine part

	for (size_t i = 0; i < nmaps; ++i) {
		let& mi = m_map_info[i];
		if (mi.status != map_info::used) {
			continue;
		}
		auto& emi = m_em[i];


		set_map(emi, emi.m.get(), mi.dim_p);
	}

	ac.m_af_point_calc.process(m_vb, m_em, get_fg());

	//adjust dims
	for (size_t i = 0; i < m_vb.size(); ++i) {
		auto& vb = m_vb[i];
		if (!vb || vb.dim() == m_ver_dim[i])continue;
		vb.reset(eval_helpers::extend_real_vector_size(vb.get(), m_ver_dim[i] + 1));
	}

	ac.m_af_bc.calc_bounds(
		m_vb,
		ac.m_af_point_calc.m_points_in_comp,
		m_em,
		get_fg());

	////////////////////////////////////////////////////////////////////////////
	has_invalid_vers = false;
	for (size_t i = 0; i < get_fg().m_comp.size(); ++i) {
		if (ac.m_af_bc.m_comp_valid[i])continue;
		has_invalid_vers = true;
		let& c = get_fg().m_comp[i];
		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = g.m_ver_sorted[j + c.idx_sorted];
			m_ver_dim[v] = ims_max;
			m_vb[v].reset();
		}
	}

	if (has_invalid_vers) {
		for (auto& m : m_map_info) {
			if (m.status == map_info::used) {
				m.status = map_info::any;
			}
		}
		any_to_used(get_fg().m_edges);

		if (!init_fg(g)) {
			return true;
		}

		m_fg.init(gid, g.num_ver());
	}

	err = false;//passed all checks

	let& fg = get_fg();

	////////////////////////////////////////////////////////////////////////////

	ims_resize(m_comp_info, cg.m_comp.size());

	for (size_t comp_idx = 0; comp_idx < fg.m_comp.size(); ++comp_idx) {
		auto& c = fg.m_comp[comp_idx];
		let v = fg.m_ver_sorted[c.idx_sorted];//first vertex of the component
		m_comp_info[comp_idx].dim_p = m_ver_dim[v];
	}
	////////////////////////////////////////////////////////////////////////////
	//algebraic part
	for (size_t i = 0; i < nmaps; ++i) {
		auto& mi = m_map_info[i];
		if (mi.status != map_info::used) {
			continue;
		}

		auto& emi = m_em[i];

		if (emi.det_rootn < ims_num_traits<Real>::almost_zero()) {
			continue;//algebraic matrices need inverse matrices
		}

		emi.ma = create_alg_map(mi.proj_id, i, am);
	}

	////////////////////////////////////////////////////////////////////////////
	//fill in the alg_id for the component
	for (size_t comp_idx = 0; comp_idx < fg.m_comp.size(); ++comp_idx) {

		auto& alg_id = m_comp_info[comp_idx].alg_id;

		auto update_alg_id = [&alg_id](alg_id_t id)
			{
				if (id == 0)return true;
				if (alg_id == 0) {
					alg_id = id;
				} else if (alg_id != id) {
					alg_id = ims_max;
				}
				return alg_id != ims_max;
			};

		auto& c = fg.m_comp[comp_idx];

		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = fg.m_ver_sorted[j + c.idx_sorted];
			let ne = fg.num_edges(v);

			for (size_t e = 0; e < ne; ++e) {
				let& qe = fg.get_edge(v, e);

				let* ma = m_em[qe.m].ma.get();

				if (!ma) {
					alg_id = ims_max;
					goto lab_next_comp;
				}

				let& mi = m_map_info[qe.m];

				alg_id_t aid;
				if (mi.proj_id != ims_max) {
					aid = mi.proj_id + s_first_non_trivail_alg_id;
				} else {
					aid = mi.dim_p;
				}

				if (!update_alg_id(aid)) {
					goto lab_next_comp;
				}

				let tcom = fg.m_ver2com[qe.second];
				if (tcom != comp_idx) {
					if (!update_alg_id(m_comp_info[tcom].alg_id)) {
						goto lab_next_comp;
					}
				}
			}
		}
	lab_next_comp:;//next component

	}


	return true;
}

size_t block_info::common_dim_proj() const
{
	size_t dim_p = 0;

	for (let& c : m_comp_info) {
		if (c.dim_p == 0 || c.dim_p == ims_max)continue;

		if (dim_p == 0) {
			dim_p = c.dim_p;
		} else if(dim_p != c.dim_p){
			return 0;
		}
	}

	return dim_p;
}

void block_info::clear_proj_data()
{
	m_proj_data.clear();
}

void block_info::get_affine(Real* dst, const Real* src, alg_id_t id) const
{
	if (id < s_first_non_trivail_alg_id) {
		let sz = id * (id + 1);
		std::copy(src, src + sz, dst);
	} else {
		let& proj = m_proj_data.m_projs[id - s_first_non_trivail_alg_id].m_projector;
		proj.get_affine(dst, src);
	}
}


void block_info::atom_data::eval(eval_context& ec, ast_context ast)
{
	pool_ptr v;

	if (!m_v) {
		if (ast.h.is_xundef()) {
			m_v = eval_pool::ep.get_empty_val();
			return;
		}
		v = ec.eval7(ast, true);
		if (!v) {
			return;
		}
	} else {
#if defined(DEVELOPER_VERSION)
		assert(!m_v->is(ims_val::ETP::compos));
#endif
		v = m_v;
	}
	
	pool_ptr va(eval_helpers::to_affine_or_scalar(v.get(), ast.a->get_dim()));
	m_v = va ? va : v;
}


void block_info::atom_data::project(eval_context& ec, ast_context ast, proj_data& pd, bool checked)
{
	assert(!m_v_proj);

	if (!m_v || !m_v->is_affine()) {
		return;//use the projector only for affine
	}

	pool_ptr afr(eval_helpers::to_real3(m_v.get()));

	let proj_ptr = ast.a->m_subspace;
	if (!proj_ptr.is_def()) {
		m_v_proj = afr;//trivial projector, everything is fine
		return;
	}

	m_proj_id = pd.get_proj(ast_context(proj_ptr, ast.call_offset), ec);

	let& fp = pd.m_projs[m_proj_id].m_projector;

	if (fp.empty()) {
		assert(!m_v_proj);//error
		return;
	}


	if (checked && !fp.check(afr->p_r())) {
		assert(!m_v_proj);//error
		return;
	}

	m_v_proj = eval_pool::ep.get_affine_real(fp.dim_proj());
	fp.get_affine(m_v_proj->p_r(), afr->p_r());
}

size_t block_info::atom_data::get_dim() const
{
	if (!m_v_proj)return 0;
	assert(m_v_proj->is(ims_val::ETP::matrix, ims_val_b::EST::real));
	return m_v_proj->extent(0);
}

bool block_info::atom_data::invalid() const
{
	if (m_v_proj)return false;	//success - projected affine
	if (!m_v)return true;		//error in geometric calculations
	
	let t = m_v->gt();

	let err =
		//affine, which could not be projected
		t == ims_val_b::ETP::matrix ||
		//a vector that could not be transformed into an affine
		t == ims_val_b::ETP::vector ||
		//unknown - for example, a link to a block
		t == ims_val_b::ETP::ast_ptr;
	
	//If there are scalars, shaders, identities, etc., then there is no error
	return err;
}



#include "ims_chrono.h"
void test_allocation_rate()
{
	let num_iters = 1024 * 1024 * 16;
	size_t sum = 0;

	let start = ims_chrono::now();
	for (size_t iter = 0; iter < num_iters; ++iter) {
		pool_ptr p(eval_pool::ep.get_affine_real(((iter + sum) & 15) + 1));//170 million/c
		//pool_ptr p(eval_pool::ep.get_affine_real(1));//250 million/c
		sum += reinterpret_cast<size_t>(p.get() + iter);
	}
	let time = ims_chrono::dif_micro(start, ims_chrono::now()) * 1e-6;
		
	std::cout << "rate = " << num_iters/time  <<
		" sum = " << sum << std::endl;
}


const ims_val* block_info::create_proj_map(size_t i, const ast_maps& am) const
{
	let& m = am.get_map(i);

	pool_ptr prod(eval_pool::ep.get_vector(m.num, ims_val::ETP::compos));
	auto** vec = prod->p_v();
	size_t idx = 0;

	for (size_t j = 0; j < m.num; ++j) {
		let& a = get_atom(j + m.start, am);
	

		let* v = a.m_v.get();

		assert(v);


		if (v->is_id()) {
			continue;//does not affect anything
		}

		//as a result of geometric calculations, if any compositions appear,
		//they are only identical (for example, ^0 or $id)
		assert(!v->is(ims_val::ETP::compos));

		pool_ptr pa;

		if (v->is(ims_val::ETP::number)) {
			pa = eval_helpers::to_real3(v);
		} else if (v->is(ims_val::ETP::matrix)) {
			//pa = q ? q: eval_helpers::to_real3(a.m_v_proj.get());
			pa = eval_helpers::to_real3(a.m_v_proj.get());
		} else if (!ims_val::is_geom(v->gt())) {
			pa = v; v->add_ref();
		} else {
			//can get here when printing projections in the console
			pa = v; v->add_ref();
		}

		eval_helpers::edge_append(vec, idx, pa.get());
	}
	prod.get_mut()->shrink(idx);
	prod.reset_if(prod->elevate_compos());
	return prod.release();

}


const ims_val* block_info::create_alg_map(size_t& proj_id,	size_t i, const ast_maps& am) const
{
	let& m = am.get_map(i);

	pool_ptr prod_a;

	bool err_alg = false;

	for (size_t j = 0; j < m.num; ++j) {
		let& a = get_atom(j + m.start, am);

		let* v = a.m_v.get();

		size_t a_proj_id = a.m_proj_id;

		assert(v);



		if (v->is_id()) {
			continue;//does not affect anything
		}

		if (v->is(ims_val::ETP::number)) {
			if (!v->is(ims_val::EST::rational)) {
				err_alg = true;
				break;
			}

			if (!prod_a) {
				prod_a = v; v->add_ref();
			} else if (prod_a->is(ims_val::ETP::number)) {
				prod_a.reset(eval_pool::ep.get_scalar_int(
					prod_a->get_int() * v->get_int()));
			} else {
				assert(prod_a->is(ims_val::ETP::matrix));
				let dim = prod_a->extent(0);
				auto* res = eval_pool::ep.get_affine_int(dim);
				pool_ptr af(eval_helpers::to_affine3(v, dim));
				eval_helpers::affine_mul_rational(res, prod_a.get(), af.get());
				prod_a = res;
			}
		} else if (v->is(ims_val::ETP::matrix)) {
			if (!v->is(ims_val::EST::rational)) {
				err_alg = true;
				break;
			}

			let dim_a = v->extent(0);

			if (!prod_a) {
				prod_a = v; v->add_ref();
				proj_id = a_proj_id;
			} else if (prod_a->is(ims_val::ETP::number)) {
				auto* res = eval_pool::ep.get_affine_int(dim_a);
				pool_ptr af(eval_helpers::to_affine3(prod_a.get(), dim_a));
				eval_helpers::affine_mul_rational(res, af.get(), v);
				prod_a = res;
				proj_id = a_proj_id;
			} else {
				assert(prod_a->is(ims_val::ETP::matrix));
				if (prod_a->extent(0) != dim_a) {
					err_alg = true;
					break;
				}

				if (proj_id != a_proj_id) {
					err_alg = true;
					break;
				}

				auto* res = eval_pool::ep.get_affine_int(dim_a);
				eval_helpers::affine_mul_rational(res, prod_a.get(), v);
				prod_a = res;
			}

		} else if (ims_val::is_geom(a.m_v->gt())) {
			//for example Moebius
			err_alg = true;
			break;
		}
	}
	if (err_alg) {
		return nullptr;
	}
	
	if (!prod_a) {
		prod_a = eval_pool::ep.get_scalar_int(1);
	}

	return prod_a.release();
}

void block_info::set_map(edge_map& emi, const ims_val* m, size_t dim_p)
{
	assert(m);

	const ims_val** sa;
	size_t na;
	if (m->is(ims_val::ETP::compos)) {
		sa = m->p_v();
		na = m->get_size();
	} else {
		sa = &m;
		na = 1;
	}

	auto& prod = emi.mg;

	for (size_t j = 0; j < na; ++j) {
		let* v = sa[j];

		if (v->is_id())continue;

		if (v->is(ims_val::ETP::style2)) {
			//TODO: should be reworked
			continue;
		}

		pool_ptr av;

		if (v->is(ims_val::ETP::number)) {
			if (dim_p == 0) {
				av = v; v->add_ref();
			} else {
				av = eval_helpers::to_affine3(v, dim_p);
			}
		} else if (v->is(ims_val::ETP::matrix)) {
			let d = v->extent(0);
			if (d == dim_p) {
				av = v; v->add_ref();
			} else if(d < dim_p) {
				av = eval_helpers::real_affine_increase_dim(v, dim_p);
			} else {//d > dim_p
				assert(false);
				dim_p = d;
				av = v; v->add_ref();
			}
		}

		if (av) {
			if (!prod) {
				prod = av.release();//move ownership
			} else if (dim_p == 0) {
				prod = eval_pool::ep.get_scalar_real(
					prod->get_real() * av->get_real());
			} else {
				auto* res = eval_pool::ep.get_affine_real(dim_p);
				eval_helpers::affine_mul_real(res, prod.get(), av.get());
				prod = res;
			}
		} else if (ims_val::is_geom(v->gt())) {
			prod.reset();//for example Moebius
			break;
		}
	}



	if (prod) {
		let eps = ims_num_traits<Real>::almost_zero();
		if (prod->is(ims_val::ETP::number)) {
			let n = prod->get_real();
			emi.det_rootn = std::abs(n);
			emi.neg_det = n<0 && emi.det_rootn > eps;
			emi.is_sim = true;
		} else {
			assert(dim_p == prod->extent(0));
			let sim = get_sim(prod->p_r(), dim_p);
			let det = matrix_helper::calc_det_ex(prod->p_r(), prod->extent(0));
			if (sim > 0) {
				emi.det_rootn = sim;//better precision 
				emi.is_sim = true;
			} else {
				emi.det_rootn = std::pow(std::abs(det), 1.0 / dim_p);
				emi.is_sim = false;
			}
			emi.neg_det = det<0 && emi.det_rootn > eps;
		}
	} else {
		prod = eval_pool::ep.get_scalar_real(1);
		emi.det_rootn = 1;
		emi.is_sim = true;
		emi.neg_det = false;
	}
}



void test_pool() 
{
	using alloc_type = eval_pool_allocator<boost::multiprecision::limb_type>;
	using my_backend = boost::multiprecision::cpp_int_backend<0, 0, boost::multiprecision::signed_magnitude, boost::multiprecision::unchecked, alloc_type>;
	using my_cpp_rational_backend = boost::multiprecision::rational_adaptor<my_backend>;
	using my_cpp_rational = boost::multiprecision::number<my_cpp_rational_backend>;

	my_cpp_rational p = 1;
	for (size_t i = 2; i <= 100; ++i) {
		p *= i;
	}
	my_cpp_rational c = p / 101;
	std::cout << "c = " << c << std::endl;
}



size_t block_info::get_dim_alg(alg_id_t id) const
{
	if (id < s_first_non_trivail_alg_id) {
		return id;
	}
	return m_proj_data.m_projs[id - s_first_non_trivail_alg_id].m_projector.dim_algebraic();
}

size_t block_info::get_dim_proj(alg_id_t id) const
{
	if (id < s_first_non_trivail_alg_id) {
		return id;
	}
	return m_proj_data.m_projs[id - s_first_non_trivail_alg_id].m_projector.dim_proj();
}


const proj_data& block_info::get_proj_data() const
{
	return m_proj_data;
}

const block_info::atom_data& block_info::get_atom(size_t idx, const ast_maps& am) const
{
	return m_atom_data[am.m_ixm.get_atom(idx)];
}


void block_info::any_to_used(std::span<const ims_edge> s)
{
	for (let& e : s) {

		auto& mi = m_map_info[e.m];

		if (mi.status != map_info::any) {
			continue;
		}

		if (m_ver_dim[e.first] == ims_max) {
			continue;
		}
		if (m_ver_dim[e.second] == ims_max) {
			continue;
		}
		

		mi.status = map_info::used;
	}
}

bool block_info::init_fg(const ims_graph& g, bool allow_invalid)
{
	m_fg.clear();
	m_cur_fg = &m_fg;

	for (let& e : g.m_edges) {

		let& m = m_map_info[e.m];
		if (!m.is_ok(allow_invalid)) {
			continue;
		}
		if (m_ver_dim[e.first] == ims_max) {
			continue;
		}
		if (m_ver_dim[e.second] == ims_max) {
			continue;
		}

		m_fg.create_edge(e.first, e.second, e.m);
	}

	return !m_fg.empty();
}


bool block_info::compute_metrics(affine_dim_calc& dc)
{
	dc.compute_all_dims(m_im, get_fg(), 
		ims_view(&m_em.data()->det_rootn, sizeof(edge_map)));

	if (ims_need_stop()) {
		return false;
	}

	return dc.compute_moments(m_im, m_em, get_fg(), m_ver_dim);
}


