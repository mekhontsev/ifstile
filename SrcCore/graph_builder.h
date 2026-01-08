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

struct ims_val;
struct block_graph;
struct ast_context;
struct variable;
struct indexed_maps;

//operators can act not only on sets in the space,
//but also on the vertices of the graph, generating new vertices
//sometimes you can get an infinite graph, which is also interesting!
struct graph_builder
{
	bool create(
		block_graph& dst, 
		size_t user_vars, 
		std::span<const variable> ec);

	graph_builder() : m_umap_compos(0, ihasher(&m_pixm), ihasher(&m_pixm)) {}

private:

	using vertex = size_t;

	//operator + the vertex it acts on
	using data_pair = std::pair<const ims_val*, vertex>;
	

	//the task is to act on the vertex with an operator, creating a part of the graph
	struct task 
	{
		const ims_val* val;
		vertex vt;

		//optional: create edge vs->... except in the following cases:
		//if vs==s_empty_ver, then set the previous task::vt
		//if vs==s_empty_ver - 1, do nothing
		vertex vs;
	};

	struct ahasher
	{
		bool operator()(const ast_context* c1, const ast_context* c2) const;
		size_t operator()(const ast_context* c) const;
	};

	struct ihasher
	{
		ihasher(const indexed_maps** im) : m_im(im) {};
		bool operator()(size_t m1, size_t m2) const;
		size_t operator()(size_t m) const;
		const indexed_maps** m_im;
	};

	//a specially allocated vertex corresponding to the empty set
	static constexpr vertex s_empty_ver = ims_max;

	//the maximum number of vertices that the operator can act on
	static constexpr size_t s_max_contexts = 100'000;

	////////////////////////////////////////////////////////////////////////////

	const indexed_maps* m_pixm = nullptr;
	std::vector<uint8_t> m_imp;//important vertices
	std::vector<size_t> m_temp;//for different needs
	std::vector<task> m_stack;

	//stores the action of all operators - which vertex it translates to which
	ankerl::unordered_dense::map<data_pair, vertex> 
		m_vertex_map;
	//to recognize equivalent maps
	//key - map, value - new map number
	ankerl::unordered_dense::map<size_t, size_t, ihasher, ihasher> 
		m_umap_compos;
	//to eliminate duplicates within compositions
	ankerl::unordered_dense::map<const ast_context*, size_t, ahasher, ahasher> 
		m_umap;
};
