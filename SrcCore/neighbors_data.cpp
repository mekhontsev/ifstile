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
#include "neighbors_data.h"
#include "ims_graph_base.h"
#include "ims_val.h"



void neighbors_data::clear()
{
	m_childs.clear();
	m_root_inters.clear();
	m_data.clear();
	m_idxs.clear();
	m_hash.clear();
};

size_t neighbors_data::get_root_inter(size_t ver, size_t ei, size_t ej) const
{
	let& ri = m_root_inters[ver];
	return ri.idx + ri.num_ed * ei + ej;
};

//add element idx to list dst in correct orientation
bool neighbors_data::append_childs(std::vector<size_t>& dst, size_t child_idx) const
{
	let nid = m_childs[child_idx];
	if (nid == ims_max) {
		return true;
	}

	auto from = dst.size();//before this, all the elements are ready
	dst.emplace_back(child_idx);

	size_t num_iter = 0;

	while (from < dst.size()) {
		++num_iter;
		if (num_iter > m_data.size()) {
			//loop from inter_type::right
			//not often, with approximate search
			return false;
		}


		let& e = m_data[m_childs[dst[from]]];

		if (e.inter_type_left()) {
			++from;
			continue;
		}

		assert(e.res != inter_type::empty);

		//divide s1
		assert(e.div_ver() == e.s1);
		let ne = num_edges(e.s1);

		for (size_t j = 0; j < ne; ++j) {
			let idx = e.idx + j;
			let qi = m_childs[idx];
			if (qi != ims_max) {
				dst.emplace_back(idx);
			}
		}

		std::swap(dst[from], dst.back());
		dst.pop_back();
	}
	return true;
};



bool neighbors_data::append_item(std::vector<size_t>& dst, size_t child_idx) const
{
	auto i = dst.size();//before this, all the elements are ready
	if (!append_childs(dst, child_idx)) {
		return false;
	};

	for (; i < dst.size(); ++i) {
		dst[i] = m_childs[dst[i]];
	}
	return true;
};

size_t neighbors_data::num_neighbours() const
{
	size_t ret = 0;
	for (let& q : m_data) {
		if (q.inter_type_left()) {
			++ret;
		}
	}
	return ret;
}

size_t neighbors_data::set_idx_graph(bool all)
{
	size_t num_ver = 0;
	for(auto& q :m_data) {
		if (all || q.inter_type_left()) {
			q.idx_graph = num_ver++;
		} else {
			q.idx_graph = ims_max;
		}
	}
	return num_ver;
}


bool neighbors_data::create_boundary(
	const ims_graph_base& dig,
	ims_graph_base& boundary,
	size_t full_map_base)
{

	auto& dst = boundary.m_edges;
	dst.clear();


	for (let& q : m_data) {

		if (ims_max == q.idx_graph) {
			continue;
		}

		let base_left = q.inter_type_left();
		let s = base_left ? q.s0 : q.s1;

		let ne = dig.num_edges(s);

		for (size_t i = 0; i < ne; ++i) {

			m_idxs.clear();

			auto m = dig.get_edge(s, i).m;

			let qe = q.idx + i;

			if (full_map_base == 0) {
				if (!append_childs(m_idxs, qe)) {
					return false;
				}

			}else{
				let nid = m_childs[qe];
				if (nid != ims_max) {
					m_idxs.emplace_back(qe);
					if (!base_left) {
						m += full_map_base;//shift the maps indices
					}
				}
			}

			for (let r : m_idxs) {
				auto& t = dst.emplace_back();
				t.first = q.idx_graph;
				t.second = m_data[m_childs[r]].idx_graph;
				assert(t.first != ims_max && t.second != ims_max);
				t.m = m;
			}
		}

	};
	return true;
};

void neighbors_data::set_relator(
	std::vector<intptr_t>& dst,
	std::span<const neghbour_map> nbm,
	size_t nid,
	size_t hr, size_t hf, size_t pidx) const
{
	//TODO: optimize, remove memory allocation
	std::vector<size_t> mapf;
	std::vector<size_t> mapr;

	////////////////////////////////////////////////////////////////////////////
	if (ims_max != hf)mapf.emplace_back(hf);
	if (ims_max != hr)mapr.emplace_back(hr);


	while (ims_max != pidx) {
		let& ph = nbm[pidx];
		if (ims_max != ph.f)mapf.emplace_back(ph.f);
		if (ims_max != ph.r)mapr.emplace_back(ph.r);

		pidx = ph.par;
	};
	////////////////////////////////////////////////////////////////////////////
	//std::reverse(mapr.begin(), mapr.end());
	std::reverse(mapf.begin(), mapf.end());

	dst.resize(mapr.size() + mapf.size());
	size_t di = 0;
	for (size_t i = 0; i < mapr.size();++i) {
		dst[di++] = -intptr_t(mapr[i] + 1);
	}
	for (size_t i = 0; i < mapf.size(); ++i) {
		dst[di++] = intptr_t(mapf[i] + 1);
	}

	//second pass, TODO: figure out what's going on
	////////////////////////////////////////////////////////////////////////////
	mapf.clear();
	mapr.clear();

	let& q = nbm[nid];
	pidx = q.par;
	hr = q.r;
	hf = q.f;

	////////////////////////////////////////////////////////////////////////////
	if (ims_max != hr)mapr.emplace_back(hr);
	if (ims_max != hf)mapf.emplace_back(hf);



	while (ims_max != pidx) {
		let& ph = nbm[pidx];
		if (ims_max != ph.f)mapf.emplace_back(ph.f);
		if (ims_max != ph.r)mapr.emplace_back(ph.r);

		pidx = ph.par;
	};
	////////////////////////////////////////////////////////////////////////////

	//std::reverse(mapf.begin(), mapf.end());
	std::reverse(mapr.begin(), mapr.end());

	dst.resize(di + mapr.size() + mapf.size());

	for (size_t i = 0; i < mapf.size(); ++i) {
		dst[di++] = -intptr_t(mapf[i] + 1);
	}
	for (size_t i = 0; i < mapr.size(); ++i) {
		dst[di++] = intptr_t(mapr[i] + 1);
	}



	//remove fragments of the form f^k*f^-k
	bool cont = true;
	while (cont) {
		cont = false;
		let N = dst.size();
		for (size_t i = 0; i < N; ++i) {
			size_t j = (i + 1) % N;
			if (dst[i] == -dst[j]) {
				dst[i] = dst[j] = 0;
				std::erase_if(dst, [](let& e) {return e == 0; });
				cont = true;
				break;
			}
		}
	}

}


//relations that differ by a shifts
//or with the reverse order and changed signs are equivalent
struct relator_hasher
{
	using E = neighbors_data::relators::elem;

	//get the element's hash
	size_t operator()(const E& r) const
	{
		size_t res = 0;
		let sz = r.prod.size();

		//for all shifts
		for (size_t shift = 0; shift < sz; ++shift) {
			//in direct order
			size_t v = 0;
			for (size_t i = 0; i < sz; ++i) {
				let idx = (shift + i) % sz;
				boost::hash_combine(v, r.prod[idx]);
			}
			res ^= v;

			//in reverse order with the sign changed
			v = 0;
			for (size_t i = 0; i < sz; ++i) {
				let idx = (sz + shift - i) % sz;
				boost::hash_combine(v, -r.prod[idx]);
			}
			res ^= v;
		}

		boost::hash_combine(res, sz);//take the size into account

		return res;
	}
	//compare two elements
	bool operator()(const E& r1, const E& r2) const
	{
		if (r1.prod.size() != r2.prod.size()) {
			return false;
		}
		let sz = r1.prod.size();
		for (size_t shift = 0; shift < sz; ++shift) {
			//in direct order
			bool eq = true;
			for (size_t i = 0; i < sz; ++i) {
				let idx = (shift + i) % sz;
				if (r1.prod[i] != r2.prod[idx]){
					eq = false;
					break;
				}
			}
			if (eq) {
				return true;
			}

			//in reverse order with the sign changed
			eq = true;
			for (size_t i = 0; i < sz; ++i) {
				let idx = (sz + shift - i) % sz;
				if (r1.prod[i] != -r2.prod[idx]) {
					eq = false;
					break;
				}
			}
			if (eq) {
				return true;
			}
		}
		return false;
	}
};

void neighbors_data::get_neighbor_maps(
	std::vector<neghbour_map>& nbm,
	relators* rel,
	const ims_graph_base& dig) const
{
	nbm.resize(m_data.size());
	for (auto& h : nbm) {
		h.init();
	}

	if (rel)rel->clear();

	//initial intersection set
	for (size_t s = 0; s < dig.num_ver(); ++s) {

		if (ver_invalid(s)) {
			continue;
		}

		let sne = dig.num_edges(s);

		for (size_t i = 0; i < sne; ++i) {
			let& qi = dig.get_edge(s, i);//left: fi^-1

			for (size_t j = 0; j < sne; ++j) {
				let& qj = dig.get_edge(s, j);//right: fj

				if (j == i) {//do not intersect the element with itself
					continue;
				}


				let sij = m_childs[get_root_inter(s, i, j)];
				if (sij == ims_max)continue;//empty

				size_t hr = qi.m;
				size_t hf = qj.m;


				auto& h = nbm[sij];

				if (h.ready()) {//visited
					if (rel) {
						set_relator(rel->m_data.emplace_back().prod,
							nbm, sij, hr, hf, ims_max);
					}
					continue;
				}

				h.par = ims_max;
				h.r = hr;
				h.f = hf;
			}
		}
	}

	for (size_t i = 0; i < m_data.size(); ++i) {
		//we must go through all of them because we need to visit all child elements

		let& ev = m_data[i];

		let left = ev.inter_type_left();


		let vdiv = left ? ev.s0 : ev.s1;
		let	ne = dig.num_edges(vdiv);


		for (size_t ediv = 0; ediv < ne; ++ediv) {

			let idx = m_childs[ev.idx + ediv];
			if (idx == ims_max)continue;//empty edge
			let& qd = dig.get_edge(vdiv, ediv);

			auto& h = nbm[idx];

			size_t hr, hf;

			if (left) {//qd.m -> reverse
				hr = qd.m;
				hf = ims_max;
			} else {//qd.m -> forward
				hr = ims_max;
				hf = qd.m;
			}

			if (h.ready()) {//visited
				if (rel) {
					set_relator(rel->m_data.emplace_back().prod,
						nbm, idx, hr, hf, i);
				}
				continue;
			}

			h.par = i;
			h.r = hr;
			h.f = hf;
		}
	}

	if (rel) {

		using Map = ankerl::unordered_dense::set<
			relators::elem,
			relator_hasher,
			relator_hasher
		>;

		Map rm;
		for (auto& r : rel->m_data) {
			auto res = rm.emplace(r);
			if (!res.second) {
				r.prod.clear();
			}
		}

		std::erase_if(rel->m_data, [](auto& r) {return r.prod.empty(); });

		std::sort(rel->m_data.begin(), rel->m_data.end(), [](let& e1, let& e2) {
			return e1.prod.size() < e2.prod.size();
		});
	}
}


size_t neighbors_data::get_neighbor_graph(
	ims_graph_base& dst,
	std::vector<neighbor_edge_label>& labels,
	const ims_graph_base& dig) const
{
	labels.clear();
	dst.clear_base();

	//initial intersection set
	for (size_t s = 0; s < dig.num_ver(); ++s) {

		if (ver_invalid(s)) {
			continue;
		}

		let sne = dig.num_edges(s);

		for (size_t i = 0; i < sne; ++i) {
			let qi = dig.get_edge_idx(s, i);//left: fi^-1

			for (size_t j = 0; j < sne; ++j) {

				if (j == i) {//do not intersect the element with itself
					continue;
				}

				let sij = m_childs[get_root_inter(s, i, j)];
				if (sij == ims_max)continue;//empty

				let qj = dig.get_edge_idx(s, j);//right: fj

				let map_idx = labels.size();
				auto& m = labels.emplace_back();
				m.ef = qj;
				m.er = qi;

				dst.create_edge(m_data.size() + s, sij, map_idx);
			}
		}
	}

	for (size_t i = 0; i < m_data.size(); ++i) {
		//we must go through all of them because we need to visit all child elements
		let& ev = m_data[i];

		if (ev.res == inter_type::overlapped) {
			continue;//ignore
		}

		let left = ev.inter_type_left();


		let vdiv = left ? ev.s0 : ev.s1;
		let	ne = dig.num_edges(vdiv);

		for (size_t ediv = 0; ediv < ne; ++ediv) {

			let idx = m_childs[ev.idx + ediv];
			if (idx == ims_max)continue;//empty edge

			let qd = dig.get_edge_idx(vdiv, ediv);

			let map_idx = labels.size();
			auto& m = labels.emplace_back();

			if (left) {//qd.m -> reverse
				m.er = qd;
				m.ef = ims_max;
			} else {//qd.m -> forward
				m.er = ims_max;
				m.ef = qd;
			}

			dst.create_edge(i, idx, map_idx);
		}
	}

	dst.set_vertex_index(m_data.size() + dig.num_ver());

	//remove unused disbalanced neighbors
	let nv = dst.num_ver();
	for (size_t v = 0; v < nv; ++v) {
		let ne = dst.num_edges(v);
		for (size_t i = 0; i < ne; ++i) {
			auto& e = dst.get_edge(v, i);

			let v2 = e.second;

			let& ev = m_data[v2];
			if (ev.inter_type_left()) {
				continue;//overlapped and left neighbors are always balanced
			}

			let em = labels[e.m];//copy

			let ne2 = dst.num_edges(v2);

			bool can_be_replaced = true;
			for (size_t j = 0; j < ne2; ++j) {
				let& e2 = dst.get_edge(v2, j);
				assert(e2.first == v2);

				neighbor_edge_label jm = em;
				if (!jm.join(labels[e2.m])) {
					can_be_replaced = false;
					break;
				}
			}

			if (!can_be_replaced) {
				continue;
			}
			let e_first = e.first;//save before create_edge may reallocate m_edges
			e.m = ims_max;//mark as removed

			for (size_t j = 0; j < ne2; ++j) {
				let& e2 = dst.get_edge(v2, j);
				assert(e2.first == v2);

				neighbor_edge_label jm = em;
				[[maybe_unused]] const bool join_ok = jm.join(labels[e2.m]);
				ASSUME(join_ok);

				let map_idx = labels.size();
				labels.emplace_back() = jm;

				dst.create_edge(e_first, e2.second, map_idx);
			}
		}
	}
	std::erase_if(dst.m_edges, [](let& e) {
		return e.m == ims_max;
	});

	//adjust overlaps
	for (auto& q : dst.m_edges) {
		let& ev = m_data[q.second];
		if (ev.res == inter_type::overlapped) {
			assert(ev.s0 == ev.s1);
			q.second = m_data.size() + ev.s0;
		}
	}

	dst.set_vertex_index(dst.num_ver());
	m_idxs.clear();
	ims_resize(m_visited, dst.num_ver());
	//start from the dig graph vertices
	for (size_t v = 0; v < dig.num_ver(); ++v) {
		m_idxs.emplace_back(v + m_data.size());
		while (!m_idxs.empty()) {
			let cv = m_idxs.back();
			m_idxs.pop_back();
			if (m_visited[cv])continue;
			m_visited[cv] = true;
			let ne = dst.num_edges(cv);
			for (size_t i = 0; i < ne; ++i) {
				let& e = dst.get_edge(cv, i);
				m_idxs.emplace_back(e.second);
			}
		}
	}
	std::erase_if(dst.m_edges, [&](let& e) {
		return !m_visited[e.first];
	});

	//remap proper vertices
	m_idxs.resize(dst.num_ver());
	size_t idx = 0;
	for(size_t v = 0; v < m_data.size(); ++v) {//do not touch dig vertices
		if (!m_visited[v]) {
			continue;
		}
		m_idxs[v] = idx++;
	}
	for (size_t v = 0; v < dig.num_ver(); ++v) {
		m_idxs[v + m_data.size()] = idx + v;
	}

	for (auto& e : dst.m_edges) {
		assert(m_visited[e.first]);
		assert(m_visited[e.second]);
		e.first = m_idxs[e.first];
		e.second = m_idxs[e.second];
	}

	return idx;
}

bool neighbors_data::ver_invalid(size_t v) const
{
	return m_root_inters[v].invalid();
}

void neighbors_data::revert(size_t data_size, size_t child_size)
{
	std::erase_if(m_hash, [data_size](let v) {return v >= data_size; });
	m_data.resize(data_size);
	m_childs.resize(child_size);
}

void neighbors_data::collapse_empty()
{
	size_t dst_idx = 0;

	m_idxs.resize(m_data.size());
	for (size_t i = 0; i < m_data.size(); ++i) {
		auto& ev = m_data[i];

		if (ev.res == inter_type::empty) {
			m_idxs[i] = ims_max;
		} else {
			m_idxs[i] = dst_idx;
			m_data[dst_idx++] = ev;
		}

	}
	m_data.resize(dst_idx);

	for (auto& q : m_childs) {
		if (q != ims_max)q = m_idxs[q];
	}
}
