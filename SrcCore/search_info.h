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

#include "cardinality.h"
#include "inter_type.h"



struct oper_block;
struct block_class;


enum class intersection_mes :uint8_t
{
	//all intersections have finite measure
	all_fin = 0,

	//there is an intersection of incomplete dimension and infinite measure
	has_inf = 1,

	//there is an intersection of the full dimension and the infinite measure
	max_inf = 2,
};

//allowed types of connectedness when searching
enum class connectedness:uint8_t
{
	disconnected = 0,		//any
	weak = 1,		//there are intersected parts
	regular = 2,	//cdim >=0
	positive = 3,	//cdim >0
	strong = 4,		//cdim = bdim
};

enum class point3intersections :uint8_t
{
	//no intersections by 3
	empty = 0,

	//maximum by finite number of points
	finite = 1,

	//by a countable number of points
	countable = 2,

	//by an uncountable number of points
	uncountable = 3,
};

//we consider those who have the same structure to be topologically identical
struct structure_info
{
	//the same structure implies the same graphs
	block_id_t m_base_block_id;


	size_t m_reduced_n_graph_hash;//hash of the reduced neighbor graph
	size_t m_ver_info_hash;//hash of information about each vertex of the reduced neighbor graph
	size_t m_ver_d0_fin;//number of vertices of dimension 0 of finite measure
	size_t m_ver_d0_inf;//number of vertices of dimension 0 of infinite measure
	size_t m_num_ver;//number of vertices of the neighbor graph
	size_t m_num_edg;//number of edges of the neighbor graph

	//hash of the connected component graph:
	size_t m_com_graph_hash;
	size_t m_num_comp;//number of connected components
	size_t m_com_graph_types;//number of types in the component graph


	bool operator==(const structure_info& t) const
	{
		return memcmp(this,&t,sizeof(structure_info))==0;
	};

	size_t get_hash() const
	{
		using boost::hash_combine;
		size_t h = 0;
		hash_combine(h, m_base_block_id);
		hash_combine(h, m_reduced_n_graph_hash);
		hash_combine(h, m_ver_info_hash);
		hash_combine(h, m_ver_d0_fin);
		hash_combine(h, m_ver_d0_inf);
		hash_combine(h, m_num_ver);
		hash_combine(h, m_num_edg);
		hash_combine(h, m_com_graph_hash);
		hash_combine(h, m_num_comp);
		hash_combine(h, m_com_graph_types);
		return h;
	};

	struct topo_hasher
	{
		//get the element's hash
		size_t operator()(const structure_info& ti) const
		{
			return ti.get_hash();
		}
		//compare two elements
		bool operator()(
			const structure_info& t1,
			const structure_info& t2) const
		{
			return t1 == t2;
		}
	};

	struct dim_data
	{
		//structure id = id of the first found set with this structure
		//the original element may no longer exist
		block_id_t id5 = block_id_max;

		//number of elements with such structure
		size_t	num_ref = 0;
	};


	using Map = UNAMESPACE::unordered_map<
		structure_info,
		dim_data,
		structure_info::topo_hasher,
		structure_info::topo_hasher
	>;

};

////////////////////////////////////////////////////////////////////////
struct ver_info_ex
{
	size_t lab = 0;
	size_t num_enter = 0;//number of incoming edges from other vertices
	size_t num_exit = 0;//number of outgoing edges to another vertices
	size_t num_self = 0;//number of edges to itself

	//size_t ver_in_comp;//number of vertices in the connectivity component
	//size_t num_collapsed;//how many vertices initially collapsed

	bool operator==(const ver_info_ex& t) const
	{
		return num_enter == t.num_enter &&  num_exit == t.num_exit && num_self == t.num_self;
	}

	bool operator<(const ver_info_ex& v2) const
	{

		let& v1 = *this;

		if (v1.lab != v2.lab) {
			return v1.lab < v2.lab;
		}

		if (v1.num_self != v2.num_self) {
			return v1.num_self < v2.num_self;
		}

		if (v1.num_enter != v2.num_enter) {
			return v1.num_enter < v2.num_enter;
		}

		if (v1.num_exit != v2.num_exit) {
			return v1.num_exit < v2.num_exit;
		}

		return false;//equal	
	};

	void hash_combine(size_t& h) const
	{
		boost::hash_combine(h, num_enter);
		boost::hash_combine(h, num_exit);
		boost::hash_combine(h, num_self);
	};

};


struct metric_key
{
	//second-order moment hash
	size_t mh;

	//space dimension
	size_t dim;

	//set dimension divided by space dimension [0;1]
	//useful if some maps are disabled
	double sd;

	//boundary dimension divided by space dimension [0;1]
	double db;

	//connectedness dimension divided by space dimension [0;1]
	double dc;

	//number of radii
	uint16_t nr;

	//infinity of intersections
	uint8_t mb;

	//cardinality of 0D intersections
	uint8_t card;

	//dimension of affine subspaces containing the boundary
	uint8_t pdim2;



	struct hasher
	{
		//compare two elements
		bool operator()(const metric_key& t1, const metric_key& t2) const
		{
			return
				t1.dim == t2.dim &&
				t1.mh == t2.mh &&
				t1.nr == t2.nr &&
				t1.mb == t2.mb &&
				t1.card == t2.card &&
				t1.pdim2 == t2.pdim2 &&
				metric_key::get_key(t1.sd) == metric_key::get_key(t2.sd) &&
				metric_key::get_key(t1.db) == metric_key::get_key(t2.db) &&
				metric_key::get_key(t1.dc) == metric_key::get_key(t2.dc);
				
		}

		//get the element's hash
		size_t operator()(const metric_key& ti) const
		{
			size_t seed = 0;
			boost::hash_combine(seed, ti.mh);
			boost::hash_combine(seed, ti.dim);
			boost::hash_combine(seed, ti.nr);
			boost::hash_combine(seed, ti.mb);
			boost::hash_combine(seed, ti.card);
			boost::hash_combine(seed, ti.pdim2);
			boost::hash_combine(seed, metric_key::get_key(ti.sd));
			boost::hash_combine(seed, metric_key::get_key(ti.db));
			boost::hash_combine(seed, metric_key::get_key(ti.dc));
			
			return seed;
		}
	};

	static uint64_t get_key(double v)
	{
		if (v < 0)v = 5.0/9;
		if (v > 1)v = 1 / v;
		//slightly decrease 1e12 to avoid values like 0.99999
		constexpr double mul = 0.3 * boost::math::constants::pi<double>() * 1e12;
		return static_cast<uint64_t>(v*mul);
	};

	struct metric_val
	{
		using arr = boost::container::small_vector<oper_block*, 2>;
		//metric id, equal to the id of the first detected element
		block_id_t id = block_id_max;
		//all with this metric
		arr elemetns;
	};

	using Map = UNAMESPACE::unordered_map<
		metric_key,
		metric_val,
		hasher,
		hasher
	>;
};




//information about intersection by N
struct inter_data
{
	using Real = double;

	//number of different positive dimensions
	size_t ndim;//NDIMn



	intersection_mes mes : 2;//INFMn
	
	cardinality card : 3;//NDIMn

	//maximal dimension
	Real dim;//DIMn

	void clear() 
	{
		dim = -1;
		ndim = 0;
		card = cardinality::empty;
		mes = intersection_mes::all_fin;		
	}
};


struct calc_time_ms 
{
	float ldim2;
	float r2;

	void clear() 
	{
		ldim2 = -1;
		r2 = -1;
	}
};

struct search_info
{	
	using Real = double;

	////////////////////////////////////////////////////////////////////////////
	typename structure_info::Map::value_type* m_structure;
	typename metric_key::metric_val* m_metric;


	//number of vertices of the neighbor graph
	size_t m_num_ver;
	//number of components of the neighbor graph, only those that depend on themselves
	size_t m_num_comp;
	//number of edges in the neighbor graph
	size_t m_num_edg;



	//how many times it was a prototype during search
	size_t m_num_mut;

	size_t m_proto_id;

	
	size_t m_cnum;

	//the maximum number of identical linear parts across all graph maps
	size_t m_melp;


	inter_result m_ir;


	//condition number for maps
	//Real m_cond;

	//dimension
	Real m_dim;



	////////////////////////////////////////////////////////////////////////////
	
	//intersections by 2, 3, ...
	boost::container::small_vector<inter_data, 2> m_inters;
	

	Real m_aspect_ratio;

	//maximum radii of sets
	Real m_max_rad;



	//accuracy at the time of finding (overflow_mode::real)
	double m_prec;
	

	//size_t num_smaller_to_full;

	//size_t m_resid;


#if defined(DEVELOPER_VERSION)
	calc_time_ms m_calc_time;
#endif

	//////////////////////////////////////////////////////////
	//type uint32_t


	uint32_t m_generation;
	//how many maps have changed relative to parent (negative if originated from virtual)
	int32_t m_changed_maps;

	
	//number of orientations
	uint32_t m_num_orientations;

	static constexpr decltype(m_num_orientations) s_inf_orientations =
		1000;
		//std::numeric_limits<decltype(m_num_orientations)>::max();

	//how many successful searches when it was a prototype
	uint32_t m_num_mut_success;

	//number of radii
	uint32_t m_num_rad;
	//number of empty
	//uint32_t m_num_empty;

	//number of different sets of neighbors
	uint32_t m_num_neighb;

	//the dimension of a finite number of subspaces containing intersections
	int32_t m_poly_dim2;
	//maximum number of subspaces containing any intersection
	uint32_t m_poly_mes2;
	//over all intersections of 2, we take the maximum over those whose Hausdorff dimension is equal to the dimension of the affine subspace in which the intersection lies
	int32_t m_ldim2;



	//how many reflections in the graph edge maps
	uint32_t m_refl;

	//dimensionality of space
	uint32_t m_dim_proj;



	struct pdata
	{
		
		bool has_reflections : 1;
		bool all_sim : 1;//all used maps are similarities
	//	bool ct3 : 1;
		connectedness con:3;
		//point3intersections i3p : 2;
	};

	pdata m_data;
	

	
	////////////////////////////////////////////////////////////////////////////

	Real get_dim(size_t i) const 
	{
		return i < m_inters.size() ? m_inters[i].dim : -1;
	}

	size_t get_ndim(size_t i) const
	{
		return i < m_inters.size() ? m_inters[i].ndim : 0;
	}

	intersection_mes get_inter_mes(size_t i) const
	{
		return i < m_inters.size() ? m_inters[i].mes : intersection_mes::all_fin;
	}

	cardinality get_cardinality(size_t i) const
	{
		return i < m_inters.size() ? m_inters[i].card : cardinality::empty;
	}


	void set_dim(size_t i, Real dim)
	{
		if (i >= m_inters.size())m_inters.resize(i + 1);
		m_inters[i].dim = dim;
	}

	void set_ndim(size_t i, size_t ndim)
	{
		if (i >= m_inters.size())m_inters.resize(i + 1);
		m_inters[i].ndim = ndim;
	}

	void set_inter_mes(size_t i, intersection_mes mes)
	{
		if (i >= m_inters.size())m_inters.resize(i + 1);
		m_inters[i].mes = mes;
	}

	void set_cardinality(size_t i, cardinality card)
	{
		if (i >= m_inters.size())m_inters.resize(i + 1);
		m_inters[i].card = card;
	}



	void clear()
	{
		m_structure = nullptr;
		m_metric = nullptr;

	
		m_num_rad = 0;
		m_dim = -1;

		m_ir = inter_result{};

		////////////////////////////////////////////////////////////////////////

		m_cnum = 0;


		m_data.has_reflections = false;
		m_data.all_sim = false;
		//m_data.ct3 = false;
		m_data.con = connectedness::disconnected;
		//m_data.i3p = point3intersections::empty;
		
		m_inters.clear();
		
		m_num_orientations = 0;
		m_prec = 0;
		m_refl = 0;
	

		//num_smaller_to_full = 0;

		m_num_mut = 0;
		m_num_mut_success = 0;
		m_generation = 0;
		m_proto_id = 0;
		m_changed_maps = 0;
		m_num_neighb = 0;
	
		m_aspect_ratio = -1;
		m_max_rad = -1;


		m_melp = 0;


		m_num_ver = 0;
		m_num_comp = 0;
		m_num_edg = 0;
		m_poly_dim2 = -1;
		m_poly_mes2 = 0;
		m_ldim2 = -1;
		

#if defined(DEVELOPER_VERSION)
		m_calc_time.clear();
#endif

	}

	bool has_aspect() const
	{
		return m_aspect_ratio >= 0;
	};

	Real get_aspect_ratio() const
	{
		return m_aspect_ratio;
	};

	

	size_t num_isomers() const
	{
		if (!m_metric)return 0;
		return m_metric->elemetns.size();
	};

	std::string create_name()
	{
		return 
			std::to_string(get_dim(0)) + "_" + 
			std::to_string(get_num_neighbors());
	}

	size_t topo_id()  const
	{
		if (!m_structure) {
			return 0;
		}
		return m_structure->second.id5;
	};

	//size of the intersection graph
	size_t get_num_neighbors()  const
	{
		if (!m_structure)return 0;
		return m_structure->first.m_num_ver;
	};

	size_t get_num_neighbors_comp()  const
	{
		if (!m_structure)return 0;
		return m_structure->first.m_num_comp;
	};

	size_t get_num_neighbors_edges()  const
	{
		if (!m_structure)return 0;
		return m_structure->first.m_num_edg;
	};

	size_t get_mid() const 
	{
		return m_metric?m_metric->id:0;
	};

	const oper_block* get_best() const
	{
		if (!m_metric)return nullptr;
		let& e = m_metric->elemetns;
		if (e.empty())return nullptr;
		return e.front();
	}

	//returns true if this is better than other
	int better_than(const search_info& other) const
	{

#if 0	//currently only applies to elements with the same metric
		//only makes sense for elements with different metrics (but the same structure)
		if (has_aspect() && other.has_aspect()) {	
			if (m_aspect_ratio > other.m_aspect_ratio + eps)return 1;
			if (m_aspect_ratio < other.m_aspect_ratio - eps)return -1;
		}
#endif		

		//the one who doesn't contain reflections always wins
		//the number of reflections itself doesn't matter
		if (m_refl == 0 && other.m_refl > 0)return 1;
		if (m_refl > 0 && other.m_refl == 0)return -1;
		
		//makes sense only for elements with different structures (but the same metrics)
		if (m_num_ver < other.m_num_ver)return 1;
		if (m_num_ver > other.m_num_ver)return -1;
#if 0	
		//suitable for any elements but may depend on the permutation of maps
		//depth is more important than the number of edges
		if (m_depth < other.m_depth)return 1;
		if (m_depth > other.m_depth)return -1;
#endif

		if (m_num_edg < other.m_num_edg)return 1;
		if (m_num_edg > other.m_num_edg)return -1;

		//the MORE components, the better (the graph is simpler)
		if (m_num_comp > other.m_num_comp)return 1;
		if (m_num_comp < other.m_num_comp)return -1;
#if 0	
		//suitable for any elements but may depend on the permutation of the maps
		if (m_gcx < other.m_gcx)return 1;
		if (m_gcx > other.m_gcx)return -1;
#endif
		return 0;
	};

	
};
