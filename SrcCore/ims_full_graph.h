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
#include "combinator.h"

struct neighbors_data;
struct ims_edge;
struct ims_graph_base;
	
struct ims_inter_lists: public boost::noncopyable
{
	struct elem
	{
		size_t ver; //base node
		size_t idx; //beginning of the list in m_lists (must be sorted!)
		size_t sz; //list length
	};


	using index_t = size_t;

	size_t get_inter_direct(size_t idx) const { return m_lists[idx]; };

	const elem& get_elem(index_t idx) const { return m_data[idx].h->first; }

	size_t num_elem() const { return m_data.size(); };

	bool exists(const elem& e) const;

	size_t m_max_list_size = 100'000'000;//maximum total length of lists

protected:

	ims_inter_lists() : m_hash(0, hasher(m_lists), hasher(m_lists)) {};

	using array_index = std::vector<index_t>;

	struct hasher
	{
		hasher(const array_index& arr) :m_arr(arr) {};
		const array_index& m_arr;

		//get the element's hash
		size_t operator()(const elem& e) const
		{
			using boost::hash_combine;
			size_t h = 0;
			hash_combine(h, e.ver);
			hash_combine(h, e.sz);
			for (size_t i = 0; i < e.sz; ++i) {
				hash_combine(h, m_arr[e.idx + i]);
			}
			return h;
		}
		//compare two elements
		bool operator()(const elem& e1, const elem& e2) const
		{
			if (e1.sz != e2.sz || e1.ver != e2.ver)return false;
			for (size_t i = 0; i < e1.sz; ++i) {
				if (m_arr[e1.idx + i] != m_arr[e2.idx + i])return false;
			}
			return true;
		}
	};

	using HashTable = UNAMESPACE::unordered_map<
		elem,
		size_t,
		hasher,
		hasher
	>;

	void clear();

	//first we need to fill the interval in m_lists starting from idx and to the end
	elem prepare(size_t ver, size_t idx);
	
	//returns the inserted/found element, restores the size of m_lists
	//if the insertion did not occur
	HashTable::value_type& insert(const elem& q);


	bool need_stop() const;

	HashTable m_hash;

	//stores the lists themselves
	array_index m_lists;

	//stores the elements into which the lists are divided
	array_index m_elem_refs;


	struct data_el 
	{
		HashTable::value_type* h;
		size_t idx_child;	//index of the lists into which the original element was split
		size_t next_edge;	//next edge to be processed
	};

	std::vector<data_el> m_data;

	//returns true if e1 is contained in e2 (sorted lists)
	bool first_in_second(const elem& e1, const elem& e2) const;

};

struct ims_full_graph: public ims_inter_lists
{
public:

	//find all neighborhoods
	bool find_neighborhoods(
		const ims_graph_base& dig, 
		const neighbors_data& nb, 
		bool from_emtpy);

	//get a graph of all found neighborhoods
	void get_nbh_graph(const ims_graph_base& dig, std::vector<ims_edge>& dst);

	

};

////////////////////////////////////////////////////////////////////////////////
struct ims_full_inter : public ims_inter_lists
{
public:

	

	//adds an initial set of intersections by 1
	void init0(const ims_graph_base& dig);

	//adds the initial set of intersections by 1 and 2
	void init1(const neighbors_data& nb);

	//find all intersections, returns the total number of intersections by 3, 4, ..., max_num
	size_t calc_inters(const size_t max_num);


	using interval = std::pair<index_t, index_t>;
	interval get_interval(const size_t num);

	size_t num_intervals() const { return m_intervals.size(); };
	

	size_t get_inter(size_t idx) const { return m_nid[m_lists[idx]]; };

	
	void get_graph_x(
		std::vector<ims_edge>& dst,
		const ims_graph_base& dig, 
		const size_t num);


	//check the first-order connectivity graph for the input intersection e
	//before this, all intersections with a greater number of elements than one must be found
	void get_con_graph(std::vector<ims_edge>& dst, index_t eidx);


	//get the number of edges for vertex v
	size_t num_edges(size_t v) const;

	size_t num_child(const data_el& ev) const;

	size_t num_ver() const;

	//get the vertex to which the edge qi leads from the vertex v
	size_t target_ver(size_t v, size_t qi) const;

	
private:

	void clear();

	//adds the initial intersection set
	bool init();

	//creates the remaining intersections, closes the graph
	bool step(index_t from);

	//remove intersections that ended up empty
	bool clear_empty(index_t from);



	HashTable::value_type* insert_ex(size_t ver, size_t idx);

	std::vector<size_t> m_nid;

	//stores intersection list indices in m_elem_refs
	struct edge
	{
		size_t idx3;//start in m_elem_refs
		size_t sz3;//length
	};
	std::vector<edge> m_edges;

	const edge& get_root_inter(size_t ver, size_t ei, size_t ej) const;

	struct root_info 
	{
		//root pairwise intersections
		//reference to m_edges, which stores references to m_elem_refs
		size_t root;

		//number of edges in the original graph
		size_t nedg;
	};
	std::vector<root_info> m_roots;

	//beginning of the intersection intervals by n
	std::vector<index_t> m_intervals;



	combinator1 m_comb;
	combinator2 m_comb2;
	combinator3 m_comb3;

	std::vector<size_t> m_idxs;

	std::vector<size_t> m_temp_temp;
};
