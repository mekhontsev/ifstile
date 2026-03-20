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
#include "big_array.h"
#include "box.h"
#include "pool_ptr.h"

struct ims_graph;
struct edge_ball;
struct edge_map;

enum class cardinality : uint8_t;

//TODO: information about each vertex is required to ensure it is affine
//for nonlinear components, the bounding ball is unitary
struct affine_bound_calc
{
	using Real = double;

	struct affine_elem
	{
		pool_ptr m;//maps
		size_t vt;//target vertex, vs=m*vt

		Real nrm;//norm of matrix m
		Real d;//distance from the center to the center of the image

		affine_elem* next;
	};

	//list of parts into which the vertex is currently divided
	struct velem
	{
		affine_elem* first = nullptr;
		affine_elem* last = nullptr;//so that they can quickly add to the end
		size_t size7 = 0;

		Real r = 0;//lower bound for the vertex radius

		void clear();

		bool empty() const;

		affine_elem* get_next(affine_elem* p);

		//remove the element following p and return a reference to it
		//if p==0, then remove the first element
		affine_elem* remove_next(affine_elem* p);

		affine_elem* pop_front();

		void push_front(affine_elem* e);

		void push_back(affine_elem* e);

		//move everything from the beginning to e inclusive to the end
		void exchange(affine_elem* e);
	};

	//prepared elements
	big_array<affine_elem> m_elems_heap;

	//how many initial elements were used from the heap
	size_t m_num_used = 0;

	//indices of free elements (only those within the m_num_used range)
	std::vector<affine_elem*> m_free_elems;

	//managed to find the bounding balls
	std::vector<bool> m_comp_valid;

	//flag of finding a bounding sphere
	box<Real> m_box_temp;



	//one for each set
	std::vector<velem> m_elems;

	////////////////////////////////////////////////////////////////////////////

	//calculate set sizes
	//at the output of vb - bounding spheres
	//if !vb[i].defined() then normalization is required later
	void calc_bounds(
		std::span < edge_ball> vb,
		std::span<const cardinality> pcalc,
		std::span<const edge_map> ri,
		const ims_graph& g,
		const Real h = 0.3	//precision
	);

	void reset_heap();

	affine_elem& from_heap();

	void to_heap(affine_elem& e);

	void clear_list(velem& lst);
};