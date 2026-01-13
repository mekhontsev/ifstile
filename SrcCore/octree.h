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
#include "mesh_primitives.h"

// do not use depth 0

// at depth==1, the volume has a size of 2x2x2
// m_root - stores up to 8 data indices (level 1)
// m_nodes is empty

// at depth==2, the volume has a size of 4x4x4
// m_root - stores up to 8 indices of the m_nodes array (level 1)
// m_nodes store up to 8 data indices (level 2)



//cell numbers in volume
enum voxel_offset: uint32_t
{
	//the volume cell does not intersect with the set
	empty_ = boost::integer_traits<uint32_t>::const_max,

	//the cell is an interior point
	//i.e., it intersects with the set, as do all of its neighbors
	inside = empty_ - 1,
};


//octree of a given depth
class octree
{
public:

	//internal tree node
	struct node
	{
		node() { clear(); };
	
		void clear() {for (auto& q : childs)q = voxel_offset::empty_;}

		bool empty() const {
			for (auto q : childs)if (q != voxel_offset::empty_)return false;
			return true;
		}

		voxel_offset childs[8];//references to child nodes or cells
	};
	////////////////////////////////////////////////////////////////////////////

	void clear(){m_root.clear(); m_nodes.clear(); m_free= voxel_offset::empty_;};
	void clear_mem() { clear(); m_nodes.shrink_to_fit();  };

	void set_depth(unsigned depth) { m_depth = depth; };
	unsigned get_depth() const { return m_depth; };

	//is the tree empty?
	bool empty() const { return m_root.empty(); };

	//cube side size
	unsigned get_side() const { return 1u << m_depth; };

	//get the index of the cell/node in depth from the maximum
	voxel_offset get_cell_offset(const point3u& p, unsigned from = 0) const;

	//create a place for the cell/node index
	voxel_offset& create_cell(const point3u& p, bool is_ins);

	void create_ins(const point3u& p);

	
	//get the actually used parallelepiped
	void get_bound(point3u& v0, point3u& v1) const;


	//the cell is a border cell (there is an empty cell nearby)
	static bool is_reg(voxel_offset i) { return i < voxel_offset::inside; };
	//the cell contains a point of the set
	static bool is_empty(voxel_offset i) { return i == voxel_offset::empty_; };


	void clone_from(const octree& other);
private:
	
	unsigned		m_depth = 0;	//tree depth
	node			m_root;			//root node
	big_array<node>	m_nodes;		//all nodes

	//beginning of free list
	voxel_offset	m_free = voxel_offset::empty_;

	//insert at the beginning of the free list
	void to_free(voxel_offset v);

	//extract the first of the free ones
	voxel_offset from_free();
};
