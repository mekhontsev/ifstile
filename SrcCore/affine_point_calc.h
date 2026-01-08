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

struct edge_map;
struct edge_ball;
struct ims_graph;
struct ims_edge;
struct ver_info;

struct affine_point_calc
{
	using Real = double;

	//if a component is independent of itself, then it consists of a single set
	//if a component is dependent on itself, then two situations are possible
	//1) each set of a component is a single point
	//2) each set of a component is an infinite number of points (countable or uncountable)
	//in any case, all sets of a component contain the same number of points
	//this means we can store information about the number only in components

	std::vector<cardinality> m_points_in_comp;

	//find the cardinality of the largest set among at most countable ones
	cardinality get_status() const;

	//find one point from each set,
	//determine the cardinality of the sets
	void process(
		std::vector<edge_ball>& vb,
		std::span<const edge_map> ri,
		const ims_graph& g
	);

private:

	std::vector<ims_edge> m_edg_cycle;
};

