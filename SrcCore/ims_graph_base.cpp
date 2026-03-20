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
#include "ims_graph_base.h"

int ims_graph_base::compare_vers(size_t v1, size_t v2) const
{
	let n1 = num_edges(v1);
	let n2 = num_edges(v2);
	if (n1 < n2)return -1;
	if (n1 > n2)return 1;
	

	for (size_t i = 0; i < n2; ++i) {
		let& e1 = get_edge(v1, i);
		let& e2 = get_edge(v2, i);
		if (e1.m < e2.m)return -1;
		if (e1.m > e2.m)return 1;
	}

	return 0;//identical
};

int ims_graph_base::compare_labs(size_t v1, size_t v2, 
	std::span<const size_t> lab)  const
{
	let ne = m_vers[v1].sz;

	assert(m_vers[v2].sz == ne);
	let edx1 = m_vers[v1].idx;
	let edx2 = m_vers[v2].idx;
	//compare 2 sets of edges
	for (size_t i = 0; i < ne; ++i) {
		let& e1 = m_edges[edx1 + i];
		let& e2 = m_edges[edx2 + i];

		assert(e1.m == e2.m);

		let l1 = lab[e1.second];
		let l2 = lab[e2.second];

		if (l1 < l2)return -1;
		if (l1 > l2)return 1;
	}

	return 0;//identical
};


size_t ims_graph_base::get_edge_idx(size_t ver, size_t edg) const
{
	assert(edg < num_edges(ver));
	return m_vers[ver].idx + edg;
}

ims_edge& ims_graph_base::get_edge(size_t ver, size_t edg)
{
	return m_edges[get_edge_idx(ver, edg)];
}

const ims_edge& ims_graph_base::get_edge(size_t ver, size_t edg) const
{
	return m_edges[get_edge_idx(ver, edg)];
}

bool ims_graph_base::empty() const
{
	return m_edges.empty();
}

void ims_graph_base::clear_base()
{
	m_edges.clear();
	m_vers.clear();
}

void ims_graph_base::set_vertex_index_sorted(size_t nv)
{
	for (let& e : m_edges) {
		nv = std::max(nv, e.first + 1);
		nv = std::max(nv, e.second + 1);
	};

	//build an index
	m_vers.resize(nv);
	std::fill(m_vers.begin(), m_vers.end(), vertex_index{ 0,0 });
	for (size_t i = 0; i < m_edges.size(); ++i) {
		auto& vi = m_vers[m_edges[i].first];
		if (vi.idx == 0 && vi.sz == 0) {//not initialized
			vi.idx = i;
		}
		vi.sz += 1;
	}
};

void ims_graph_base::set_vertex_index(size_t nv)
{
	std::sort(m_edges.begin(), m_edges.end());
	set_vertex_index_sorted(nv);
}

void ims_graph_base::create_edge(size_t vs, size_t vt, size_t m)
{
	m_edges.push_back({ vs,vt,m });
}

void ims_graph_base::color_refinement(color_refinement_data& crd)
{
	if (empty()) {
		crd.clear();
		return;
	}
	//original number of maps
	size_t num_maps = 0;
	for (let& e : m_edges) {
		num_maps = std::max(num_maps, e.m);
	}
	num_maps++;
	////////////////////////////////////////////////////////////////////////////
//Add reverse edges with the modified map
//Only by index, since we will be changing the array
	let nedg = m_edges.size();
	for (size_t i = 0; i < nedg; ++i) {
		let& q = m_edges[i];//must be after create_edge

		let m = q.first == q.second ? 
			q.m + 2 * num_maps : 
			q.m + num_maps;

		create_edge(q.second, q.first, m);//invalidates references to m_edges edges
	}
	////////////////////////////////////////////////////////////////////////////


	set_vertex_index(0);

	
	auto& lab = crd.lab;
	auto& ver = crd.ver;
	auto& lab_changed = crd.lab_changed;

	lab.resize(num_ver());
	ver.resize(num_ver());	
	lab_changed.resize(num_ver());

	////////////////////////////////////////////////////////////////////////////

	for(size_t i=0;i<num_ver();++i){
		ver[i] = i;
	};


	//sort vertices by number of edges + sets of maps
	std::sort(ver.begin(), ver.end(), [this](let& v1, let& v2) {
		return compare_vers(v1, v2) < 0;
	});

	//if two vertices with the same number of edges have edge sets with
	//different maps, then assign them different numbers
	//gradually, some vertices with the same labels become vertices with different ones

	//TODO: labels must be different if the sets belong to different components
	//but the order of these components must be reflected in the ver array
	
	lab[ver[0]] = 0;
	for (size_t i = 1; i < num_ver(); ++i) {
		
		let v1 = ver[i - 1];
		let v2 = ver[i];

		let res= compare_vers(v1, v2);
		assert(res <= 0);
		lab[v2] = lab[v1] + (res == 0 ? 0 : 1);
	};

	for (;;) 
	{
		
		//run through intervals with the same mark
		size_t ibeg = 0;

		auto lb = lab[ver[ibeg]];
		for (size_t iend = 1; iend <= num_ver(); ++iend) {
			if (iend < num_ver() && lb == lab[ver[iend]])continue;
			
			//found a new interval [ib;ie)
			//sort the edges of each vertex of the interval:
			let ne = num_edges(ver[ibeg]);

			
			if (ne > 1) {
				for (size_t i = ibeg; i < iend; ++i) {
					let v = ver[i];
					assert(num_edges(v) == ne);


					let edx = m_vers[v].idx;
					let it = m_edges.begin() + edx;


					//sort the interval of edges with the same map by the label of the outgoing vertices
					size_t ebeg = 0;
					auto em = it[ebeg].m;
					for (size_t eend = 1; eend <= ne; ++eend) {
						if (eend < ne && em == it[eend].m)continue;
						//found a new interval [ebeg;eend), sort
						std::sort(it + ebeg, it + eend, [&lab](let& e1, let& e2) {
							return lab[e1.second] < lab[e2.second];
						});

						ebeg = eend;
						if (eend < ne) {
							em = it[eend].m;
						}
					}
				}
			}
			
			//sort the vertices by the labels of the outgoing edges
			std::sort(ver.begin()+ibeg, ver.begin() + iend, [&](let& v1, let& v2) {
				return compare_labs(v1, v2, lab)<0;
			});

			ibeg = iend;
			if (iend < num_ver())lb = lab[ver[iend]];
		}
		//check the interval for changes
		ibeg = 0;
		lab_changed[0] = false;
		for (size_t i = 1; i < num_ver(); ++i) {
			let v1 = ver[ibeg];
			let v2 = ver[i];

			if (lab[v1] != lab[v2] || compare_labs(v1, v2, lab) != 0){
				lab_changed[i] = true;
				ibeg = i;	
			} else {
				lab_changed[i] = false;
			}
		}

		//renumbering labels
		size_t next_lb = 0;
		bool changed = false;
		for (size_t i = 0; i < num_ver(); ++i) {
			if (lab_changed[i]) {
				next_lb++;
			}
			if (lab[ver[i]] != next_lb) {
				lab[ver[i]] = next_lb;
				changed = true;
			}
			
		}
		if (!changed) {
			break;//nothing changed during the last iteration
		}
	}

	////////////////////////////////////////////////////////////////////////////
	//transform the first vertices in the groups into their labels, and delete the rest via edges
	size_t ib = 0;
	for (size_t i = 0; i < num_ver(); ++i) {
		let v1 = ver[ib];
		let v2 = ver[i];

		let ne = num_edges(v2);
		let edx = m_vers[v2].idx;

		if (lab[v1] != lab[v2] || i==0) {
			for (size_t j = 0; j < ne; ++j) {
				auto& e = m_edges[edx + j];
				e.first = lab[e.first];
				e.second = lab[e.second];
			}
			ib = i;
		} else {
			//remove edges v2
			for (size_t j = 0; j < ne; ++j) {
				m_edges[edx + j].m= num_maps;//deletion flag
			}
		}
	};

	//leave only the edges coming from the root vertices
	ims_erase(m_edges, [num_maps](let& e) {	return e.m >= num_maps;});

}


void ims_graph_base::dump(std::ostream& of, std::span<const ims_edge> ea)
{
	of << "digraph g {" << std::endl;
	for (let& e : ea) {
		of << e.first << "->" << e.second << std::endl;
	}
	of << "}" << std::endl;
}

size_t ims_graph_base::get_hash() const
{
	using boost::hash_combine;
	size_t h = 0;
	for (let& q : m_edges) {
		hash_combine(h, q.first);
		hash_combine(h, q.second);
		hash_combine(h, q.m);
	}
	return h;
}


