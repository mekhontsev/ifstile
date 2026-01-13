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
#include "ast_maps.h"


//rebuild if:
//1) at least one variable included in the edges is no longer geometric
// checked during geometric computation of the graph edges
//2) new topological variables have appeared (contain unions or cycles)
// checked by geometric computation of ALL new variables
//3) at least one topological variable is overlapped (by anything)
// checked easily, because they are classified
struct block_graph
{
	//graph corresponding to operators
	ims_graph m_g1;

	//geometric dependency graph
	ims_graph m_deps;

	ast_maps m_am;

	////////////////////////////////////////////////////////////////////////////

	size_t num_vars() const { return m_var2ver.size(); };

	size_t ref2fg(size_t ref) const
	{
		return ref < num_vars() ? m_var2ver[ref] : ims_max;
	};

	bool closed2(size_t ref) const
	{
		return ref2fg(ref) != ims_max;
	}

	friend struct graph_builder;

private:

	//by the index of the variable gives the vertex of the graph
	std::vector<size_t> m_var2ver;
};