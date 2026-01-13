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
#include "big_array.h"


struct mesh
{
public:

	//simplification
	void post_process();

	bool save_ply(std::ostream& fs) const;

	
	
	//calculate normals at the vertices
	void calc_normals();


	//get the normal for the triangle
	void face_normal(size_t idx,point3d& nr) const;

	void face_normal(mesh_idx_t v1,mesh_idx_t v2,mesh_idx_t v3,point3d& nr) const;

	void clear();

	void clear_mem();

	bool empty();


	int get_uvw_dim(){return m_uvw_dim;};

	unsigned calc_actual_uvw() const;




	//used for tessellation
	big_array<mesh_tr_indexes>	m_tr_indexes;


	//TODO: write access functions
	//triangles by vertex indices
	big_array<mesh_triangle>	m_tr;
	//positions of vertices
	big_array<mesh_vertex_ex>	m_ver;
	//normal of vertices
	big_array<mesh_normal>		m_normals;
	//color of each vertex
	big_array<mesh_uvw>			m_uvw;


private:	

	int m_uvw_dim=0;

	big_array<mesh_edge>	m_edges;
	big_array<tr_info>		m_trinfo;

private:	

	
	uint32_t get_uniq(uint32_t vi) const
	{
		while(!m_ver[vi].get_uniq(&vi)){};
		return vi;
	};

	void get_uniq(mesh_triangle& dst, mesh_triangle& src) const
	{
		dst.v1=get_uniq(src.v1);
		dst.v2=get_uniq(src.v2);
		dst.v3=get_uniq(src.v3);
	}

	//remove duplicate vertices based on coordinates
	void del_dup_vertexes();

	//remove degenerate triangles
	void del_deg_triangles();

	//remove vertices that are not referenced by triangles
//requires that the vertex index field be correctly filled in
	void del_unref_vertexes();

	//the measure of flattening of a triangle, 0 means it is completely degenerate
	double degenerative_mes(size_t v1, size_t v2, size_t v3) const;

	//returns true if something has changed
	bool simplify(bool buse);

	void restore_info(mesh_idx_t i);

	
};
