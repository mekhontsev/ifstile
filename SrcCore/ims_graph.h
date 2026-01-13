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
#include "ims_graph_base.h"

struct graph_init_data;


//can be copied
struct  ims_graph: public ims_graph_base
{
	//clear all
	void clear();

	//fill in all the information
	//nv - the minimum number of vertices in the graph
	//if some edges were removed from the initialized graph
	//(and set_vertex_index was called),
	//then there is no need to call init again since the connected components contain
	//correct (but perhaps not completely accurate) information
	void init(graph_init_data& idata, size_t nv = 0, bool remove_edge_dups = false);

	
	//remove components of dimension 0;
	//the graph must be initialized before the call (and after, so some vertices are removed)
	void remove_dim_zero();


	//does the vertex have a dimension of 0?
	bool is_dim_zero(size_t v) const;

	size_t num_own_edges(size_t comp_idx) const;
	

	//get the vertex number from the component
	size_t get_ver(size_t comp_idx, size_t ver_in_comp) const;

	//removes edges leading to other components, reinitializes
	void remove_non_strong_edges(graph_init_data& idata);

	size_t get_comp_hash() const;

	//number of types of connectivity components
	size_t num_comp_types() const;

	void dump_graph(std::ostream& of) const;

	void dump_comp(std::ostream& of) const;

	struct component_info
	{
		//start of component in array m_ver_sorted
		size_t idx_sorted = 0;

		//number of component vertices
		size_t num_ver = 0;

		//0 for components that depend only on themselves
		//otherwise, the maximum for those they depend on + 1
		size_t depth = 0;

		//component type
		//same for components with the same num_ver, depth, has_self, and dim_zero
		size_t type = 0;

		//there are references to itself
		bool has_self = false;
		//consists of no more than a countable set of points
		bool countable  = false;
		//own part of countable
		bool own_countable = false;
		//another component depends on a component
		bool need_norm = false;

		//comparison is invariant under graph automorphism
		static int compare(const component_info& c1, const component_info& c2);

		//combination of hash
		void hash_combine(size_t& h) const;

		//does it refer to others?
		bool has_other() const { return depth > 0; };

		//contains an infinite number of points
		bool zmes_inf() const
		{
			return !countable  || (has_self && has_other());
		}
	};


	bool is_ver_empty(size_t v) const
	{
		return m_vers[v].sz == 0;
	}
public:

	//gives a connectivity component for each vertex
	std::vector<size_t> m_ver2com;

	//stores set numbers in topological sorting order
	std::vector<size_t> m_ver_sorted;

	//gives the ordinal number inside the component
	std::vector<size_t> m_ver_in_comp;

	size_t m_ver_d0_fin;//number of vertices of dimension 0 of finite measure
	size_t m_ver_d0_inf;//number of vertices of dimension 0 of infinite measure

	//connected components (topologically sorted)
	//sorting is consistent with the component hash
	//vertices without outgoing edges do not belong to any component
	std::vector<component_info> m_comp;
};
