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
#include "edge_ball.h"
#include "geometry.h" //subspace_info
#include "box.h"
#include "pool_ptr.h"

struct ims_graph_base;
struct edge_map;

struct graph_divider
{
	using Real = double;
	struct elem
	{
		size_t		ver;	//the vertex on which the map acts
		pool_ptr	m;		//map
		edge_ball	b;		//circumscribing ball
	};

	std::vector<elem> stack;

	const size_t max_depth = 100000;

	//breaks up parts until their size is determined
	void init_gd(
		std::span<const edge_map> ri,
		std::span<const edge_ball> vb,
		const ims_graph_base& dig,
		const size_t root);

	//get a parallelepiped describing the elements from the stack
	void get_box(box<Real>& dst) const;

	//divede to the specified precision
	//func should return false if the depth is insufficient and deeper is needed
	void divide_prec(
		std::span<const edge_map> ri,
		std::span<const edge_ball> vb,
		const ims_graph_base& dig,
		const subspace_info<Real>& si,
		const std::function<bool(const ims_val* ball)>& func);

	void release_maps();

	~graph_divider() { release_maps(); };
};


