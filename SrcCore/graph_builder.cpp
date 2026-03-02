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
#include "graph_builder.h"
#include "ims_val.h"
#include "variable.h"
#include "block_graph.h"
#include "indexed_maps.h"


size_t graph_builder::add_val_atom(ast_maps& am, const ims_val* v)
{
	let ur = m_umap.emplace(v, m_umap.size() + am.m_num_refs);
	if (ur.second) {
		v->add_ref();
		am.m_atoms.emplace_back(v);
	}
	return ur.first->second;
};
bool graph_builder::create(
	block_graph& dst, size_t user_vars, std::span<const variable> ec)
{
	auto& dg = dst.m_g1;
	auto& edges = dg.m_edges;
	auto& ixm = dst.m_am.m_ixm;
	auto& maps = ixm.m_maps;

	edges.clear();
	dst.m_am.clear();
	dst.m_deps.clear();
	dst.m_var2ver.clear();

	m_pixm = &ixm;

	IMS_SCOPE([&] {//clear all after completion
		m_imp.clear();
		m_temp.clear();
		m_umap_compos.clear();
		m_vertex_map.clear();
		m_stack.clear();
		m_umap.clear();
		m_pixm = nullptr;
	});

	//initial tasks
	for (size_t i = 0; i < user_vars; ++i) {
		let* q = ec[i].get_topo_val();
		if (!q)continue;

		m_stack.emplace_back(q, s_empty_ver, s_empty_ver - 1);
			
		//assign the vertex immediately - for numbering stability
		let res = m_vertex_map.emplace(
			data_pair(q, s_empty_ver), m_vertex_map.size());
		let idx = res.first->second;
		if (idx >= m_imp.size()) {
			m_imp.resize(idx + 1);
		}
		m_imp[idx] = 1;//requires processing
	}

	//we will store here the number of contexts in which the operator participates
	ims_resize(m_temp, ec.size());

	dst.m_am.m_num_refs = ec.size();

	//graph creation cycle
	while (!m_stack.empty()) {

		if (ims_need_stop()) {
			return false;
		}

		auto q = m_stack.back();//copy
		m_stack.pop_back();


		bool is_ins;
		size_t v;

		if (q.vt == s_empty_ver &&
			q.val->is(ims_val::ETP::ast_ptr) &&
			q.val->gp<ast_context>()->h.tt != ETYPE::reference)
		{
			v = s_empty_ver;
			is_ins = false;
		} else {
			//add the vertex immediately (this will be used in depth recursively)
			let res = m_vertex_map.emplace(data_pair(q.val, q.vt), m_vertex_map.size());
			v = res.first->second;
			if (v < m_imp.size() && m_imp[v]) {
				m_imp[v] = 0;
				is_ins = true;
			} else {
				is_ins = res.second;
			}
		}

		if (q.vs == s_empty_ver) {
			m_stack.back().vt = v;//information for the parent task
		} else if (q.vs != s_empty_ver - 1) {
			dg.create_edge(q.vs, v, ims_max);//identity edge from parent task
		}

		if (!is_ins) {
			continue;//created before with all edges
		}
		/////////////////////////////////////////////

		//only 3 types are possible here: ast_ptr, uni, compos
		switch (q.val->gt()) {
		case ims_val::ETP::ast_ptr:
		{
			let* ast = q.val->gp<ast_context>();

			if (ast->h.tt == ETYPE::reference) {
				let idx = ast->get_ref_idx();

				if (ec[idx].is_geom()) {
					dg.create_edge(v, q.vt, idx);
					continue;
				}

				//go deeper into the operator
				auto& num_contexts = m_temp[ast->get_ref_idx()];
				if (++num_contexts > s_max_contexts) {
					ims_error("IFS is too complex");
					return false;
				}
				m_stack.emplace_back(ec[idx].v[1].get(), q.vt, v);
				assert(m_stack.back().val);
				continue;
			}

			if (ast->h.is_closed()) {
				//TODO: create a special vertex for such cases...
				q.vt = 0;
			}
			//don't create tasks here, just add an edge
			if (q.vt != s_empty_ver) {
				dg.create_edge(v, q.vt, add_val_atom(dst.m_am, q.val));
			} 
			continue;
		}
		case ims_val::ETP::uni:
		{
			let na = q.val->get_size();
			let* a = q.val->p_v();
			//insert in reverse order so they are processed in forward order
			for (size_t i = na; i > 0; --i) {
				m_stack.emplace_back(a[i - 1], q.vt, v);
				assert(m_stack.back().val);
			}
			continue;
		}
		case ims_val::ETP::compos:
		{
			let na = q.val->get_size();
			let* a = q.val->p_v();
			for (size_t i = 0; i < na; ++i) {
				m_stack.emplace_back(a[i], q.vt, i == 0 ? v : s_empty_ver);
				assert(m_stack.back().val);
			}
			continue;
		}
		default:
		{
			if (q.vt != s_empty_ver) {
				dg.create_edge(v, q.vt, add_val_atom(dst.m_am, q.val));
			}
			continue;
		}
		}
	}

	//create maps
	let num_atoms = dst.m_am.m_atoms.size() + dst.m_am.m_num_refs;
	maps.resize(num_atoms + 1);//+ identity map at the end
	ixm.m_compos.resize(num_atoms);
	for (size_t i = 0; i < num_atoms; ++i) {
		maps[i] = { i, 1 };
		ixm.m_compos[i] = i;
	}
	for (auto& e : edges) {
		if (e.m == ims_max) {
			e.m = num_atoms;//identity map
		}
	}
	////////////////////////////////////////////////////////////////////////////
	let num_ver = m_vertex_map.size();

	//important vertices
	ims_resize(m_imp, num_ver);
	
	for (size_t i = 0; i < user_vars; ++i) {
		let* q = ec[i].get_topo_val();
		if (!q)continue;
		let it = m_vertex_map.find({ q, s_empty_ver });
		if (it != m_vertex_map.end() && it->second != s_empty_ver) {
			m_imp[it->second] = 1;
		}
	}

	auto refresh_graph = [&dg]()
	{
		std::stable_sort(dg.m_edges.begin(), dg.m_edges.end(),
		[] (let& e1, let& e2)
		{
			if (e1.first < e2.first)return true;
			if (e1.first > e2.first)return false;

			return e1.second < e2.second;

		});
		dg.set_vertex_index_sorted(0);
	};

	refresh_graph();
	//assert(num_ver == dg.num_ver());

	for (size_t v = 0; v < dg.num_ver(); ++v) {

		let ne = dg.num_edges(v);
		for (size_t j = 0; j < ne; ++j) {
			auto& e = dg.get_edge(v, j);//we will change it if necessary

			//fill the chain without cycles
			m_temp.clear();

			size_t cv = e.second;
			
			m_temp.emplace_back(v);
			
			for (;;) {
				if (cv == ims_max) {
					e.first = ims_max;
					break;
				}

				if (m_imp[cv]) {
					break;
				}
				let sz = dg.num_edges(cv);
				if (sz > 1) {
					break;
				}
				if (sz == 0) {
					//edge e has degenerated - erase it
					e.first = ims_max;
					break;
				}

				for (size_t k = 0; k < m_temp.size(); ++k) {
					if (cv == m_temp[k]) {
						m_temp.resize(k);//leave the beginning of the loop
						goto lab_exit_loop;
					}
				}

				m_temp.emplace_back(cv);

				cv = dg.get_edge(cv, 0).second;
			}

		lab_exit_loop:

			for (auto& q : m_temp) {
				q = dg.get_edge(q, 0).m;
			}
			m_temp[0] = e.m;

			e.m = ixm.mul_maps(m_temp.data(), m_temp.size());
			e.second = cv;
		}
	}

	//take into account the removed edges
	ims_erase(edges, [](let& e) {return e.first == ims_max; });
	
	refresh_graph();

	//process vertices with one edge, to which no one leads
	bool changed = false;
	for (size_t v = 0; v < dg.num_ver(); ++v) {
		//by important vertices from which only one edge emerges that does 
		//not lead to an important vertex
		if (dg.num_edges(v) != 1 || !m_imp[v])continue;

		let& e = dg.get_edge(v, 0);
		let vt = e.second;
		if (m_imp[vt])continue;

		let ne = dg.num_edges(vt);
		//e is the only edge leading to e.second
		//create compositions leading further instead

		changed = true;

		let sz = edges.size();
		edges.resize(sz + ne);

		for (size_t i = 0; i < ne; ++i) {
			let& he = dg.get_edge(vt, i);

			auto& ex = edges[sz + i];
			ex.first = v;
			ex.second = he.second;

			size_t arr[2] = { dg.get_edge(v, 0).m ,he.m };
			ex.m = ixm.mul_maps(arr, 2);
		}
		dg.get_edge(v, 0).first = ims_max;//clear
	}

	if (changed) {//take into account the removed edges
		ims_erase(edges, [](let& e) {return e.first == ims_max; });
		refresh_graph();
	}

	///////////////////////////////////////////////////////////////////////////
	//remove unreachable vertices from important ones (after the previous step)

	m_temp.clear();//list of important vertices to process
	for (size_t v = 0; v < dg.num_ver(); ++v) {
		if (m_imp[v])m_temp.emplace_back(v);
	}

	//we parse the children for important ones, marking them as important too
	while (!m_temp.empty()) {
		let v = m_temp.back();
		m_temp.pop_back();
		let ne = dg.num_edges(v);
		for (size_t j = 0; j < ne; ++j) {
			let& e = dg.get_edge(v, j);
			if (!m_imp[e.second]) {
				m_imp[e.second] = 1;
				m_temp.emplace_back(e.second);
			}
		}
	}

	//remove edges of unreachable vertices, edge sorting is preserved
	ims_erase(edges, [&](let& e) {return m_imp[e.first] != 1; });

	dg.m_vers.clear();//information has become outdated
	////////////////////////////////////////////////////////////////////////////

	//renumbering vertices
	m_temp.resize(num_ver);//the vertex number will give the variable number

	size_t nd = 0;//how many different ones have already been found

	//the edges here must be sorted!
	if (!edges.empty()) {
		size_t v = edges[0].first;
		m_temp[v] = nd++;

		for (size_t i = 1; i < edges.size(); ++i) {
			assert(edges[i - 1].first <= edges[i].first);
			let vs = edges[i].first;
			if (v == vs)continue;
			v = vs;
			m_temp[v] = nd++;
		}
	}

	//renumber the vertices in the graph
	for (auto& q : edges) {
		q.first = m_temp[q.first];
		q.second = m_temp[q.second];
	};

	//fill + renumber vertices for variables
	dst.m_var2ver.resize(user_vars);
	for (size_t i = 0; i < user_vars; ++i) {
		dst.m_var2ver[i] = ims_max;
		let* q = ec[i].get_topo_val();
		if (!q)continue;
		let it = m_vertex_map.find({ q, s_empty_ver });
		if (it != m_vertex_map.end() && it->second != s_empty_ver) {
			dst.m_var2ver[i] = m_temp[it->second];
		}
	}
	////////////////////////////////////////////////////////////////////////////	
	//fill in the final maps
	for (auto& e : edges) {
		let res = m_umap_compos.emplace(e.m, m_umap_compos.size());
		if (res.second) {
			let m = maps[e.m];//copy
			maps.emplace_back(m);
		}
		e.m = res.first->second;
	}

	maps.erase(maps.begin(), maps.end() - m_umap_compos.size());

	return true;
}

bool graph_builder::ahasher::operator()
(const ims_val* v1, const ims_val* v2) const
{
	if (v1->is(ims_val_b::ETP::ast_ptr) && v2->is(ims_val_b::ETP::ast_ptr)) {
		return ast_context::lexic_compare(
			*v1->gp<ast_context>(), *v2->gp<ast_context>()) == 0;
	}
	return v1 == v2;
}

size_t graph_builder::ahasher::operator()(const ims_val* v) const
{
	size_t ret = 0;
	if (v->is(ims_val_b::ETP::ast_ptr)) {
		ast_context::hash_combine(*v->gp<ast_context>(), ret);
	} else {
		boost::hash_combine(ret, v);
	}
	return ret;
}

bool graph_builder::ihasher::operator()(size_t m1, size_t m2) const
{
	return (*m_im)->equal(m1, m2);
}

size_t graph_builder::ihasher::operator()(size_t m) const
{
	return (*m_im)->get_hash(m);
}
