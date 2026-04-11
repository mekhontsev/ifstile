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
#include "pool_ptr.h"

struct block_info;
struct neighbors_data;

struct integer_ims
{	
	struct settings
	{
		//maximum number of elements in the search tree
		//the performance depends linearly on this parameter, but the result may be incomplete if it's too small
		size_t max_inters = 4000;//reasonable as starting point

		//maximum tree depth (ims_max for maximum, it should be default)
		size_t max_depth = ims_max;

		//max_bits: max rational precision during calculation
		//>63 - slow, because of big rational arithmetic, but more complete results are possible.
		size_t max_bits = 63;//fast

		//find_prec: usually 0, >0 only for floating point search, or for infinite neighbour graph. 0.3 is ok
		float prec = 0;

		//use orientation finding mode - ignores translates
		bool mode_ori = false;

		//stop processing the vertex if an overlap is found
		bool stop_on_overlap = true;

		//stop graph processing as soon as we couldn't process at least one vertex
		bool stop_on_incomplete = true;
	};

	//creates the set of all intersections
	//must be called before calling other algorithms
	//fills nb in breadth-first order
	inter_result calc_inter(
		neighbors_data& nb,
		const block_info& bi, 
		const settings& settings);

private:

	struct map
	{
		struct e
		{
			pool_ptr m;

			//number of bits required to represent any element
			//used only for rational arithmetic
			size_t bits;

			void set_bits();
		};

		//direct and inverse map
		std::array<e, 2> p;

		//for which algebraic id was it calculated
		size_t alg_id = ims_max;

		bool set(const ims_val* m, size_t dim);
	};


	////////////////////////////////////////////////////////////////////////////

	//for temporary needs
	pool_ptr m_taff, m_tvec;
	
	//used maps
	std::vector<map> m_maps;

	//for which block the maps were calculated
	size_t m_eval_id = 0;

	size_t m_cur_alg_id = 0;
	////////////////////////////////////////////////////////////////////////////
	

	static void init_elem_real(inter_elem& e);

	//check the intersection status
	inter_type intersect(
		const block_info& bi, 
		const inter_elem& e, 
		bool ori);

	//returns true if all intersections were determined
	bool create_for_ver(
		size_t s,
		inter_result& ir,
		neighbors_data& nb,
		const block_info& bi, 
		const settings& st);

};
