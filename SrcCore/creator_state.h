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
#include "ims_graph.h"
#include "poly_roots.h"

struct ims_identifiers;


struct creator_state 
{
	struct graph
	{
		using Real = double;
		Real m_graph_sp = 0;

		//powers of the base matrix in edge maps
		std::vector<intptr_t> m_pows;

		ims_graph m_ig2;

		//text representation
		std::string m_str_poly;

		bool parse(std::string_view str);

		void clear()
		{
			m_pows.clear();
			m_ig2.clear();

			m_graph_sp = 0;
			m_str_poly.clear();
		}

	};

	struct group_info
	{
		//string for GUI
		std::string_view name;

		//polynomial of the main element of the group (the leading coefficient may not be equal to one)
		std::vector<int64_t> poly;

		//subspace (what roots do we take)
		std::vector<uint8_t> subspace;

		size_t degree;//group order, 0 for infinite

	};

	
	static std::span<const group_info> get_group_info();
	
	graph m_cg;


	////////////////////////////////////////////////////////////////////////////
	//search parameters
	
	

	//maximal degree of the polynomial
	uint8_t m_search_poly_degree = 4;

	//search radius (variance)
	double m_variance = 1;

	//required family dimension
	double m_search_hdim_min = 2;
	double m_search_hdim_max = 2;
	
	bool m_search_integer = true;

	////////////////////////////////////////////////////////////////////////////
	//graph a2.a3
	//variance = 32
	//poly degree  = [10,10]
	//int64_t: 22,000 items/s
	//big_int: 8,200 items/s
	//using CreatorPoly = BigPoly;
	using CreatorPoly = ims_polynomial<int64_t>;
	using RealType = double;
	using RootFinder = poly_roots::root_finder<CreatorPoly::value_type, RealType>;

	struct RationalPoly
	{
		CreatorPoly p;
		CreatorPoly::value_type d;//denominator


		void adjust() 
		{
			if (d == 1)return;
			auto dx = gcd_arr(p.data().data(), p.size());
			dx = boost::integer::gcd(dx, d);
			if (dx == 1)return;
			p /= dx;
			d /= dx;
		}
	};

	
	//roots of group polynomials
	std::vector<RootFinder::RootList> m_units;

	//number of checked since the start of the search
    uint64_t m_num_checked=0;
	
	std::string m_graph_str { "3a" };
	size_t m_cur_poly = 0;


	struct hasher
	{
		//get the element's hash
		size_t operator()(const RationalPoly& e) const
		{
			using boost::hash_combine;

			size_t h = 0;
			hash_combine(h, e.d);
			for (let& q : e.p.data()) {
				hash_combine(h, q);
			}
			return h;
		}
		//compare two elements
		bool operator()(
			const RationalPoly& e1,
			const RationalPoly& e2) const
		{
			return e1.d==e2.d && e1.p == e2.p;
		}
	};



	struct subspace_info
	{
		uint16_t cell = 0;//subspace index
		double ifs_dim;


		//only for those with found_key::cyc==0
		//polynomials of the base matrix that produce a group - generators
		struct group_info
		{
			//polynomial of the base matrix
			RationalPoly	p;

			//group element value
			std::complex<RealType> v;

			size_t deg = 0;

			bool dup = false;
		};

		
		std::vector<group_info> group;
	};

	//found polynomials
	struct found_poly
	{
		std::string title;

		//group index or 0
		size_t	cyc;		

		///////////////////////////////////////////////////////////////

		//only for those with found_key::cyc==0
		//possible subspace options

		std::vector<subspace_info> si;

		bool has_r = false;

		//only for those with found_key::cyc==0
		//roots of the polynomial
		RootFinder::RootList	r;

		void get_group_code(std::string& code) const;
	};

	//stable iterators required
	using PolyHash = boost::unordered_map<
		RationalPoly,
		std::unique_ptr<found_poly>,
		hasher,
		hasher
	>;

	PolyHash m_hash_base;
	std::vector<PolyHash> m_hash_cyc;

	//don't expand the table beyond this limit
	//except when a suitable one is found, always add it
	static const size_t s_max_hash = 100000;

	//to insert into the array to receive the result
	mutable std::mutex m_lock;
	
	//list of found
	std::vector<PolyHash::value_type*> m_found_poly;

	PolyHash::value_type* get_found(size_t idx) const;


	////////////////////////////////////////////////////////////////////////////
	void find_poly();

	void clear_hash();
	void clear();

	void init_units();

	

	static void create_block2(
		ims_identifiers& idf,
		struct oper_block& b,
		const ims_graph_base& ig,
		std::span<const intptr_t> pows,
		const RationalPoly& poly,
		const size_t cyc,
		const subspace_info* si);

};
