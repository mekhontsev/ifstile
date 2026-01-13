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
#include "point.h"
#include "octree.h"


//get the subtree index [0..7]
static uint8_t get_idx(const point3u& p, unsigned s)
{
	auto x = (p.x >> s) & 1;
	auto y = (p.y >> s) & 1;
	auto z = (p.z >> s) & 1;
	return static_cast<uint8_t>((z << 2) | (y << 1) | x);
};

voxel_offset
octree::get_cell_offset(const point3u& p, unsigned from) const
{
	assert(p.x < get_side() && p.y < get_side() && p.z < get_side());

	auto* q = &m_root;

	//working with nodes
	for (unsigned i = m_depth - 1;i > from;--i) {
		auto offset = q->childs[get_idx(p, i)];
		if (!is_reg(offset))return offset;
		q = &m_nodes[offset];
	};

	return q->childs[get_idx(p, from)];
};

voxel_offset&
octree::create_cell(const point3u& p, bool is_ins)
{
	assert(p.x < get_side() && p.y < get_side() && p.z < get_side());

	auto* q = &m_root;

	voxel_offset* arr[32];

	//working with nodes
	for (unsigned i = m_depth - 1;i > 0;--i) {
		auto& offset = q->childs[get_idx(p, i)];
		assert(offset != voxel_offset::inside);//need to be checked in advance

		arr[i] = &offset;
		if (offset == voxel_offset::empty_) {
			//first we create a node (may take a long time)
			
			offset = from_free();

			if (offset != voxel_offset::empty_) {
				m_nodes[offset].clear();
			} else {
				m_nodes.emplace_back();
				offset = voxel_offset(m_nodes.size() - 1);//and only then atomically fill the link
			}
		}
		q = &m_nodes[offset];
	};

	arr[0] = &q->childs[get_idx(p, 0)];
	assert(*arr[0] != voxel_offset::inside);//need to be checked in advance

	if (is_ins) {

		for (unsigned i = 0; i < m_depth; ++i) {

			auto& a = *arr[i];

			if (i>0 && a!= voxel_offset::empty_) {
				to_free(a);
			}
			a = voxel_offset::inside;

			bool all_ins = true;
			if (i + 1 == m_depth) {
				q = &m_root;
			} else {
				q = &m_nodes[*arr[i + 1]];
			}
			for (let h : q->childs) {
				if (h != voxel_offset::inside) {
					all_ins = false;
					break;
				}
			}
	
			if (!all_ins)break;
		}
	};

	return *arr[0];
};




void octree::get_bound(point3u& v0, point3u& v1) const
{
	let sz = get_side();

	v0.x = v0.y = v0.z = (unsigned)sz;
	v1.x = v1.y = v1.z = 0;

	for (unsigned z = 0; z < sz; ++z) {
		for (unsigned y = 0; y < sz; ++y) {
			for (unsigned x = 0; x < sz; ++x) {
				let q = get_cell_offset(point3u(x, y, z));
				if (q== voxel_offset::empty_)continue;

				v0.x = std::min(v0.x, x);
				v0.y = std::min(v0.y, y);
				v0.z = std::min(v0.z, z);

				v1.x = std::max(v1.x, x);
				v1.y = std::max(v1.y, y);
				v1.z = std::max(v1.z, z);
			}
		}
	}
}

void octree::clone_from(const octree& other)
{
	m_free = other.m_free;
	m_depth = other.m_depth;
	m_root = other.m_root;
	auto sz = other.m_nodes.size();
	m_nodes.resize(sz);
	for (unsigned i = 0; i < sz; ++i) {
		m_nodes[i] = other.m_nodes[i];
	};
}

void octree::to_free(voxel_offset v)
{
	assert(v < m_nodes.size());
	auto& q = m_nodes[v];
	q.childs[0] = m_free;
	m_free = v;
}

voxel_offset octree::from_free()
{
	let v = m_free;

	if (v != voxel_offset::empty_) {
		m_free = m_nodes[v].childs[0];
	}

	return v;
}

#if 0
////////////////////////////////////////////////////////////////////////////////
//caches paths in octree
struct voxel_iter
{

	void init(const voxel_octree& vol)
	{
		m_cur_depth = 0;
		auto& q = m_path[m_cur_depth];
		q.offset = 0;
		q.x1 = q.y1 = q.z1 = 0;
		q.x2 = q.y2 = q.z2 = vol.size();
	};

	//goes as deep into the tree as possible in the direction of the cell
	/*
	unsigned get_cell(const voxel_octree& vol, unsigned x, unsigned y, unsigned z)
	{

	};
	*/
	struct layer
	{
		index offset;
		unsigned x1, x2, y1, y2, z1, z2;
	};

	//maximum tree depth
	static const unsigned s_max_depth = 32;

	layer m_path[s_max_depth];

	unsigned m_cur_depth;

};
#endif