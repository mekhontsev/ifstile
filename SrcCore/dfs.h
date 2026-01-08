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

struct ims_graph_base;
struct ims_edge;

//depth-first search, post-order traversal
struct dfs_po
{
	//returns false if the edge cannot be traversed
	//allows control over the order of vertices
	//we'll traverse all vertices anyway
	using edge_pred = std::function<bool(const ims_edge&)>;

	size_t init(const ims_graph_base& g, edge_pred p = nullptr);
	size_t next(const ims_graph_base& g);

private:
	std::vector<std::pair<size_t, size_t>> m_stack;
	std::vector<bool> m_visited;
	edge_pred m_p;
	size_t m_vers_ready = 0;
};