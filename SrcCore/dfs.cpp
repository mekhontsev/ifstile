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

#include "pch.h"
#include "dfs.h"
#include "ims_graph_base.h"

size_t dfs_po::init(const ims_graph_base& g, edge_pred p)
{
	ims_resize(m_visited, g.num_ver());
	m_stack.clear();
	m_vers_ready = 0;
	m_p = p;
	return next(g);
}

size_t dfs_po::next(const ims_graph_base& g)
{
	while (m_vers_ready < g.num_ver()) {
		while (!m_stack.empty()) {
			auto& s = m_stack.back();
			let vs = s.first;
			let ne = g.num_edges(vs);
			for (; s.second < ne; ++s.second) {
				let& e = g.get_edge(vs, s.second);
				if (m_p && !m_p(e))continue;
				if (!m_visited[e.second]) {
					m_visited[e.second] = true;
					m_stack.push_back({ e.second, 0 });
					goto lab_next_iter;
				}
			};
			m_stack.pop_back();
			return vs;//completed child traversal

		lab_next_iter:;

		}
		if (m_visited[m_vers_ready]) {
			++m_vers_ready;
		} else {
			m_visited[m_vers_ready] = true;
			m_stack.push_back({ m_vers_ready, 0 });
		}
	}
	return m_vers_ready;//completed graph traversal
}
