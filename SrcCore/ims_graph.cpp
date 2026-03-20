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
#include "ims_graph.h"
#include "graph_init_data.h"

#if 0
#include "dfs.h"
#endif

////////////////////////////////////////////////////////////////////////////////


bool ims_graph::is_dim_zero(size_t v) const
{
	return m_comp[m_ver2com[v]].countable ;
}

////////////////////////////////////////////////////////////////////////////////

void ims_graph::clear()
{
	clear_base();

	m_ver2com.clear();
	m_ver_sorted.clear();
	m_comp.clear();
	m_ver_in_comp.clear();
}


size_t ims_graph::get_ver(size_t comp_idx, size_t ver_in_comp) const
{
	return m_ver_sorted[ver_in_comp + m_comp[comp_idx].idx_sorted];
};


void ims_graph::remove_non_strong_edges()
{
	let nv = num_ver();

	ims_erase(m_edges, [this](let& e) {
		return m_ver2com[e.first] != m_ver2com[e.second];
	});

	init(nv);
};

void ims_graph::init(size_t nv0, bool remove_edge_dups)
{
	auto& idata = graph_init_data::get();

	std::sort(m_edges.begin(), m_edges.end());

	//remove duplicates (only after sorting)
	if (remove_edge_dups) {
		m_edges.erase(std::unique(m_edges.begin(), m_edges.end()), m_edges.end());
	}
	set_vertex_index_sorted(nv0);

#ifndef NDEBUG
	bool has_empty_edges = false;
	for (let& e : m_edges) {
		if (m_vers[e.second].sz == 0) {
			has_empty_edges = true;
			//assert(false);//it seems that this doesn't happen
			break;
		}
	}
#endif // !NDEBUG

#if 0
	if (has_empty_edges) {//remove edges leading to empty vertices
		auto& dfs = idata.dfs;
		auto pred = [&](const ims_edge& e) {return m_vers[e.second].sz > 0; };
		bool changed = false;
		for (size_t v = dfs.init(*this, pred); v < m_vers.size(); v = dfs.next(*this)) {
			if (m_vers[v].sz == 0)continue;
			let ne = num_edges(v);
			bool all_empty = true;
			for (size_t e = 0; e < ne; ++e) {
				let& qe = get_edge(v, e);
				if (m_vers[qe.second].sz > 0) {
					all_empty = false;
					break;
				}
			}
			if (all_empty) {
				changed = true;
				m_vers[v].sz = 0;
			}
		}

		if (changed) {
			ims_erase(m_edges, [&](let& e) {return m_vers[e.second].sz == 0; });
			set_vertex_index(nv0);
		}
		
	}
#endif	
	////////////////////////////////////////////////////////////////////////////
	let nv = m_vers.size();

	ims_resize(m_ver2com, nv);
	ims_resize(m_ver_in_comp, nv);

	//edges are sorted by set_vertex_index
	//the only requirement here is to sort by first
	idata.g.m_forward.assign_from_sorted_edges(
		m_edges.begin(),
		m_edges.end(),
		boost::typed_identity_property_map<size_t>(), //renumbering the first vertices
		boost::keep_all(),
		nv,
		0);

	let nc0 = boost::strong_components(
		idata.g,
		boost::make_iterator_property_map(
			m_ver2com.begin(), boost::get(boost::vertex_index, idata.g)
		)
	);
#ifndef NDEBUG
	//boost::strong_components sorts components topologically
	//I didn't find it in the documentation, but I tested it on complex tile boundaries
	//even with random vertex renumbering, the condition is met
	for (auto& e : m_edges) {
		assert(m_ver2com[e.first] >= m_ver2com[e.second]);
	}
#endif
	////////////////////////////////////////////////////////////////////////
	//each vertex without edges has its own component
	//we get rid of such components
	size_t num_comp = nc0;

	//temporarily used to renumber components
	//gives a new component based on the old one
	m_ver_sorted.resize(nc0);
	for (size_t i = 0; i < nc0; ++i) {
		m_ver_sorted[i] = i;
	};

	bool changed = false;//minor optimization
	for (size_t i = 0; i < nv; ++i) {
		if (m_vers[i].sz == 0) {
			changed = true;
			m_ver_sorted[m_ver2com[i]] = ims_max;
		}
	};


	if (changed) {
		num_comp = 0;
		for (size_t i = 0; i < nc0; ++i) {
			if (m_ver_sorted[i] != ims_max) {
				m_ver_sorted[i] = num_comp++;
			}
		}
		for (auto& q : m_ver2com) {
			q = m_ver_sorted[q];
		}
	}


	////////////////////////////////////////////////////////////////////////
	m_ver_sorted.resize(nv);
	for (size_t i = 0; i < nv; ++i) {
		m_ver_sorted[i] = i;
	};
	std::sort(m_ver_sorted.begin(), m_ver_sorted.end(), [&](auto i1, auto i2) {
		return m_ver2com[i1] < m_ver2com[i2];
	});

	////////////////////////////////////////////////////////////////////////

	//fill in detailed information about the connectivity components
	m_comp.resize(num_comp);

	size_t ids = 0;//index in the m_ver_sorted array	
	for (size_t i = 0; i < m_comp.size(); ++i) {
		auto& c = m_comp[i];
		c.idx_sorted = ids;

		while (ids < nv && m_ver2com[m_ver_sorted[ids]] == i) {
			ids += 1;
		}
		c.num_ver = ids - c.idx_sorted;
	}

	////////////////////////////////////////////////////////////////////////////
	for(let& c: m_comp){
		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = m_ver_sorted[j + c.idx_sorted];
			m_ver_in_comp[v] = j;
		}
	}

	////////////////////////////////////////////////////////////////////////////

	//find out whether components reference themselves and others
	//find components of dimension 0, which is when simultaneously:
	//1) a component is independent of others or depends only on those with dimension 0
	//2) no more than one (0 or 1) edge leading to the same component originates from each vertex of the component
	for (size_t i = 0; i < m_comp.size(); ++i) {//for all components
		auto& c = m_comp[i];

		c.depth = 0;

		c.countable  = true;
		c.own_countable = true;
		c.has_self = false;
		
		//what is the maximum number of edges leading from any vertex to the component
		size_t max_num_self = 0;

		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = m_ver_sorted[j + c.idx_sorted];
			let ne = num_edges(v);

			size_t num_self = 0;//how many edges of the vertex lead to the same component

			for (size_t k = 0; k < ne; ++k) {//by all edges of the vertex

				let& e = get_edge(v, k);
				
				let ct = m_ver2com[e.second];
				if (ct == ims_max)continue;



				if (ct == i) {
					num_self += 1;
				} else {

					let& cx = m_comp[ct];

					c.depth = std::max(c.depth, cx.depth+1);

					if (!cx.countable) {
						c.countable  = false;
					}
				}
			}
			max_num_self = std::max(max_num_self, num_self);
			
		}

		if (max_num_self > 0) {
			c.has_self = true;
		}
		if (max_num_self > 1) {
			c.countable  = false;
			c.own_countable = false;
		}
	}

	////////////////////////////////////////////////////////////////////
	//fill the need_norm flag for the components
	
	for (auto& c : m_comp) {
		c.need_norm = false;
	}
	for (auto& e : m_edges) {
		let c = m_ver2com[e.second];
		if (c == ims_max)continue;
		m_comp[c].need_norm = true;
	}

	////////////////////////////////////////////////////////////////////////////
	//statistics by dimension 0
	m_ver_d0_inf = 0;
	m_ver_d0_fin = 0;
	for (let& q : m_comp) {
		if (!q.countable)continue;
		if (q.zmes_inf()) {
			m_ver_d0_inf += q.num_ver;
		} else {
			m_ver_d0_fin += q.num_ver;
		}
	};

	if (num_comp < 2) {
		return;
	}

	////////////////////////////////////////////////////////////////////////////
	//classify components

	auto& topo_map = idata.topo_map;
	auto& comp_sorted = idata.comp_sorted;

	
	comp_sorted.resize(num_comp);
	for (size_t i = 0; i < num_comp; ++i) {
		comp_sorted[i] = i;
	}
	std::sort(comp_sorted.begin(), comp_sorted.end(), [this](let& c1, let& c2) {
		return component_info::compare(m_comp[c1], m_comp[c2])<0;
	});

	//reverse map for comp_sorted
	topo_map.resize(num_comp);
	for (size_t i = 0; i< num_comp; ++i) {
		topo_map[comp_sorted[i]] = i;
	}

	//renumbering references from vertices to components
	for (auto& q : m_ver2com) {
		if (q == ims_max) {
			continue;
		}
		q = topo_map[q];
	};

	//renumbering components
	m_comp.resize(num_comp * 2);
	for (size_t i = 0; i < num_comp; ++i) {
		m_comp[num_comp + i] = m_comp[comp_sorted[i]];
	}
	m_comp.erase(m_comp.begin(), m_comp.begin() + num_comp);

	//type assignment
	m_comp[0].type = 0;
	for (size_t i = 1; i < num_comp; ++i) {

		let& c1 = m_comp[i - 1];
		auto& c2 = m_comp[i];

		let res = component_info::compare(c1, c2);
		
		assert(res <= 0);

		c2.type = c1.type + (res == 0 ? 0 : 1);
	};
	
}

void ims_graph::remove_dim_zero()
{
	let nv = num_ver();

	//renumbering vertices
	std::vector<size_t> remap(nv);

	size_t next_num = 0;
	for (size_t i = 0; i < nv; ++i) {
		if (!is_dim_zero(i)) {
			remap[i] = next_num++;
		};
	}
	
	//remove edges
	ims_erase(m_edges, [this](let& e) {
		return is_dim_zero(e.first) || is_dim_zero(e.second);
	});

	for (auto& e : m_edges) {
		e.first = remap[e.first];
		e.second = remap[e.second];
	}
}



////////////////////////////////////////////////////////////////////////////////

int ims_graph::component_info::compare(
	const component_info& c1, const component_info& c2)
{
	if (c1.depth < c2.depth)return -1;
	if (c1.depth > c2.depth)return 1;

	if (c1.num_ver < c2.num_ver)return -1;
	if (c1.num_ver > c2.num_ver)return 1;

	if (!c1.has_self && c2.has_self)return -1;
	if (c1.has_self && !c2.has_self)return 1;

	if (!c1.countable  && c2.countable )return -1;
	if (c1.countable  && !c2.countable )return 1;

	return 0;//same type
}

size_t ims_graph::get_comp_hash() const
{
	using boost::hash_combine;
	size_t h = 0;
	for (let& q : m_comp) {
		q.hash_combine(h);
	}
	return h;
}

size_t ims_graph::num_comp_types() const
{
	if (m_comp.empty())return 0;
	return m_comp.back().type + 1;
}

void ims_graph::dump_graph(std::ostream& of) const
{
	of << "digraph g {" << std::endl;
	for (let& e : m_edges) {
		//of << e.first <<"_"<<m_ver2com[e.first] << "->" << e.second << "_" << m_ver2com[e.second] << std::endl;
		of << e.first << "->" << e.second << " [" << "label = " << e.m << "]" << std::endl;
	}
	of << "}" << std::endl;
}

void ims_graph::dump_comp(std::ostream& of) const
{
	of << "digraph comp {" << std::endl;
	std::vector<size_t> ca;
	for (size_t i = 0; i < m_comp.size(); ++i) {//for all components
		ca.clear();
		let& c = m_comp[i];
		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = m_ver_sorted[j + c.idx_sorted];
			let ne = num_edges(v);
			for (size_t k = 0; k < ne; ++k) {
				let tc = m_ver2com[get_edge(v, k).second];
				ca.push_back(tc);
			}
		}

		std::sort(ca.begin(), ca.end());
		ca.erase(std::unique(ca.begin(), ca.end()), ca.end());

		for (let& q : ca) {
			of << i << "_" << c.num_ver;

			if (c.countable) {
				of << "z";
			}
			of << "->" << q << "_" << m_comp[q].num_ver;
			if (m_comp[q].countable ) {
				of << "z";
			}
			of << std::endl;
		}
	}
	of << "}" << std::endl;
}

size_t ims_graph::num_own_edges(size_t comp_idx) const
{
	let& c = m_comp[comp_idx];

	let nv = c.num_ver;

	size_t ret = 0;//how many edges are in the component
	for (size_t j = 0; j < nv; ++j) {//over all vertices of the component
		let v = m_ver_sorted[j + c.idx_sorted];
		let ne = num_edges(v);
		for (size_t k = 0; k < ne; ++k) {
			let& e = get_edge(v, k);
			if (m_ver2com[e.second] != comp_idx)continue;
			++ret;
		}
	}
	return ret;
};

void ims_graph::component_info::hash_combine(size_t& h) const
{
	boost::hash_combine(h, num_ver);
	boost::hash_combine(h, depth);
	boost::hash_combine(h, has_self ? 1 : 0);
	boost::hash_combine(h, countable  ? 1 : 0);
}
