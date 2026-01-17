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

#pragma once


struct ims_edge
{
	size_t first;
	size_t second;
	size_t m;

	bool operator==(const ims_edge& e) const
	{
		return first == e.first && second == e.second && m == e.m;
	}

	//this is the order used in several algorithms
	bool operator<(const ims_edge& e) const
	{
		if (first < e.first)return true;
		if (first > e.first)return false;

		if (m < e.m)return true;
		if (m > e.m)return false;

		return second < e.second;
	}
};

//Compressed Sparse Row
struct ims_graph_base
{
	std::vector<ims_edge> m_edges;

	struct vertex_index
	{
		size_t idx = 0;//starting edge
		size_t sz = 0;//number of edges
	};

	//all vertices
	std::vector<vertex_index> m_vers;

	////////////////////////////////////////////////////////////////////////////
	
	//(re)create the vertex index, sort the edges
	//nv - the minimum number of vertices in the graph (if there are vertices without outgoing edges)
	void set_vertex_index(size_t nv);

	void set_vertex_index_sorted(size_t nv);


	//number of graph vertices
	size_t num_ver() const { return m_vers.size(); }

	//number of edges for the vertex
	size_t num_edges(size_t ver) const { return  m_vers[ver].sz; };

	size_t get_edge_idx(size_t ver, size_t edg) const;

	//get the outgoing edge
	const ims_edge& get_edge(size_t ver, size_t edg) const;

	ims_edge& get_edge(size_t ver, size_t edg);

	bool empty() const;

	void clear_base();


	void create_edge(size_t vs, size_t vt, size_t m = 0);

	//recursively remove all edges that lead to vertices without edges
	//requires a vertex index
	//for large graphs, it's better to use topological sorting
	//for small ones, it works fast
//	void remove_isolated();	

	struct color_refinement_data
	{
		//label of the vertices of the original graph
		//if the label is different, then the vertices are definitely different
		std::vector<size_t> lab;

		//vertices in the graph in ascending order of labels
		std::vector<size_t> ver;

		//flags: it is necessary to increase the label relative to the previous one
		std::vector<bool> lab_changed;

		void clear() 
		{
			lab.clear();
			ver.clear();
			lab_changed.clear();

		}
	};

	//form a topology graph
	void color_refinement(color_refinement_data& crd);

	static void dump(std::ostream& of, std::span<const ims_edge> ea);

	size_t get_hash() const;


private:
	//sorting vertices by edge map
	int compare_vers(size_t v1, size_t v2) const;
	int compare_labs(size_t v1, size_t v2, std::span<const size_t> lab) const;
};

