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
#include "eval_pool.h"
#include "graph_divider.h"

struct ims_graph;
struct edge_map;

struct geom_input_data 
{
	using Real = double;
	
	const ims_graph_base* gm;
	std::span<const edge_map> ri;
	std::span<const edge_ball> vb;
	Real eps;		//relative error
	size_t max_queue_size;
	size_t max_result_size;
	size_t root;	//for what set
};

template<size_t N>
struct geom_solver 
{
	using Real = double;
	using Vec = DynVec<Real>;

	std::vector<std::array<Vec, N>> m_result;

protected:

	using elem = graph_divider::elem;
	
	struct state
	{
		//upper bound for diameter (caching)
		Real	m_h;

		//how many elements from the result array exactly did not match
		size_t	m_next_to_check;
		std::array<elem, N> m_elem;//parts forming the diameter

		void release() 
		{
			for (auto& e : m_elem) {
				e.m.reset();
				e.b.reset();
			}
		};
	};


	//initial dividing
	graph_divider m_gd;

	
	void release_maps()
	{
		m_gd.release_maps();
	};

	void reset()
	{
		release_maps();
		m_result.clear();
		m_q.clear();
	};

	struct compare
	{
		bool operator()(const state& s1, const state& s2) const
		{
			return s1.m_h < s2.m_h;
		}
	};


	struct MyQueue :
		public std::priority_queue<state, std::vector<state>, compare>
	{
		void clear() { this->c.clear(); }
	};
	
	MyQueue m_q;
};