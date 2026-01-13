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

#include "mesh_primitives.h"
#include "box.h"
#include "octree.h"
#include "voxel.h"

struct mesh;

struct voxel_volume
{
public:

	

	using Vec3 = Eigen::Matrix<double, 3, 1>;

	using voxel = voxel_t<10>;


public:

	//update volume information based on the next ball
	//returns <0 if the point is out of bounds, 0 if nothing has been updated,
	//>0 if an update has occurred
	//may throw an out-of-memory exception
	double update(const Vec3& bc, double br, Vec3& uvw);

	//triangulation
	void triangulate(mesh& m) const;

	//translates vertex coordinates back into space
	//does not change normals
	void revert_to_space(mesh& m) const;


	//get the radius of the ball to which the object should be divided
	double get_radius() const;

	void init(const box<double>& bx, size_t resolution, bool use_colors);

	void clear();

	void clear_mem();

private:

	



private:

	bool m_use_colors = true;

	octree m_octree;

	//all used cells
	big_array<voxel> m_cells;

	//data linked to cells (indices are the same as m_cells)
	big_array<uvw_sum> m_data;

	Vec3	m_origin;		//the point corresponding to the corner of the parallelepiped
	double	m_voxel_size=0;	//voxel size by coordinates


	//precomputed tables for marching cubes
	static uint16_t	EdgeTable[256];
	static int8_t	TriTable[256][16];

	

	//beginning of free list
	voxel_offset	m_free = voxel_offset::empty_;

	//insert at the beginning of the free list
	void to_free(voxel_offset v);

	//extract the first of the free ones
	voxel_offset from_free();


};