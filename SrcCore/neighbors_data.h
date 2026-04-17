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

#include "inter_type.h"


struct ims_graph_base;


struct neighbors_data 
{
	neighbors_data() : m_hash(0, hasher(*this), hasher(*this)) {};


	//r^-1*f: r*s0, f*s1
	struct neghbour_map
	{
		//parent index
		//ims_max - if not present (root)
		size_t par;

		//maps that are added to the parent maps
		//ims_max - if none (identity)
		//r^-1*f
		size_t r, f;

		bool ready() const
		{
			return r != ims_max || f != ims_max;
		}

		void init()
		{
			r = f = par = ims_max;
		}
	};

	struct relators
	{
		struct elem
		{
			//m-> m+1
			//m^-1 -> -(m+1)
			//m^0 -> 0
			std::vector<intptr_t> prod;
		};

		std::vector<elem> m_data;

		void clear()
		{
			m_data.clear();
		}
	};


	struct root_inter_ref
	{
		//index in the m_childs array into which elements were split
		size_t idx = ims_max;
		//how many edges
		size_t num_ed;

		bool invalid() const 
		{
			return idx == ims_max;
		}
		void set_invalid() 
		{
			idx = ims_max;
		}
	};

	struct inter_result : public inter_elem
	{

		size_t idx = 0;//index in the m_elem_refs array of elements split into
		size_t sz8 = 0;//how many elements split into (left and right)

		//sequence number among those with intertype_left or ims_max
		size_t idx_graph;

		//the next edge we will split
		uint32_t next_edge;
		//depth at which it was first found
		uint32_t depth;
	};

	std::vector<root_inter_ref> m_root_inters;

	bool ver_invalid(size_t v)const;

	
	//stores intersections in BREADTH-first order,
	//i.e., the list is ordered by increasing depth
	//TODO: The WASM version crashes on emplace_back at GCX=6 million (irrational rotations)
	std::vector<inter_result> m_data;
	
	//stores references to those on whom the elements are divided
	std::vector<size_t> m_childs;

	//for temporary needs
	std::vector<size_t> m_idxs;


	//precision
	double m_prec = 1;

	struct hasher
	{
		hasher(const neighbors_data& th) : m_nd(th) {};

		//get the element's hash
		size_t operator()(size_t i) const
		{
			let& e = m_nd.m_data[i];
			return inter_elem::get_hash(e, 1 / m_nd.m_prec);
		}
		//compare two elements
		bool operator()(size_t i1, size_t i2) const
		{
			let& e1 = m_nd.m_data[i1];
			let& e2 = m_nd.m_data[i2];
			return inter_elem::is_eq(e1, e2, m_nd.m_prec);
		}

		const neighbors_data& m_nd;
	};

	//inverse function for m_data
	ankerl::unordered_dense::set<size_t, hasher, hasher> m_hash;

	////////////////////////////////////////////////////////////////////////////
	//helps roll back calculation results
	void revert(size_t data_size, size_t child_size);

	size_t num_ver() const { return m_root_inters.size(); };
	size_t num_edges(size_t ver) const { return m_root_inters[ver].num_ed; };


	void collapse_empty();

	void clear();

	//get the index of the element si & mi^-1*mj(sj)
	size_t get_root_inter(size_t ver, size_t ei, size_t ej) const;

	bool append_childs(std::vector<size_t>& dst, size_t child_idx) const;
	//add element idx to list dst in correct orientation
	bool append_item(std::vector<size_t>& dst, size_t child_idx) const;

	size_t num_neighbours() const;
	size_t set_idx_graph(bool all = false);

	//uses the idx_graph property
	bool create_boundary(
		const ims_graph_base& dig,
		ims_graph_base& boundary,
		size_t full_map_base = 0);

	void get_neighbor_maps(
		std::vector<neghbour_map>& nbm, 
		relators* rel,//may be nullptr
		const ims_graph_base& dig) const;

private: 

	void set_relator(
		std::vector<intptr_t>& dst,
		std::span<const neghbour_map> nbm,
		size_t nid,
		size_t hr, size_t hf, size_t par) const;

};

