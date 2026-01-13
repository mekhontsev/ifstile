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
#include "point.h"


using uvw_type_t=uint16_t;
using mesh_idx_t=uint32_t;

using point3d=point3t<double>;
using point3f=point3t<float>;
using point3u=point3t<unsigned>;

using mesh_vertex=point3f;
using mesh_normal=point3f;
using mesh_uvw=point3f;

static constexpr mesh_idx_t max_mesh_idx_t = 
	std::numeric_limits<mesh_idx_t>::max();


struct mesh_tr_indexes
{
	mesh_idx_t m_v1,m_n1,m_t1;
	mesh_idx_t m_v2,m_n2,m_t2;
	mesh_idx_t m_v3,m_n3,m_t3;
	float		m_square;
};



//16 bytes
struct uvw_sum
{
	float	r,g,b;
	float	a;

	void clear()
	{
		r = g = b = a = 0;
	};

	void update(double px, double py, double pz,  double d)
	{
		r+=(float)(px*d);
		g+=(float)(py*d);
		b+=(float)(pz*d);
		a+=(float)d;
	}

	point3d get_color() const
	{
		return point3d(r/a,g/a,b/a);
	}
};

static_assert(sizeof(uvw_sum)==16,"uvw_sum: invalid size");

struct mesh_vertex_ex
{	
	mesh_vertex_ex(){};
	mesh_vertex_ex(const point3f &p_):p(p_){};
	
	//index in the array that contains the vertex
	mesh_idx_t	index;
	
	//if p.x<-1, then it's a duplicate, p.y stores the index
	point3f		p;

	//get the unique index
	//returns true if the element is unique
	bool	get_uniq(mesh_idx_t* t=nullptr) const
	{
		if (p.x<-1){
			if(t){*t=*reinterpret_cast<const mesh_idx_t*>(&p.y);};
			return false;
		}else	{if(t)*t=index;return true;}
	};

	//turn into a duplicate and set who it refers to
	void	set_dup(const mesh_idx_t ui)
	{
		p.x=-2;
		*reinterpret_cast<mesh_idx_t*>(&p.y)=ui;
	};


	point3d gp() const {return point3d(p.data());}

};

static_assert(sizeof(mesh_vertex_ex)==16,"mesh_vertex_ex: invalid size");

struct tr_info


{
	//triangle edge numbers [v1,v2], [v2,v3], [v1,v3]
	mesh_idx_t e12,e23,e13;

	//number of the component that contains the triangle
	mesh_idx_t m_data;
	
	mesh_idx_t* get(mesh_idx_t e)
	{
		if (e==e12)return &e12;
		if (e==e13)return &e13;
		if (e==e23)return &e23;
		return nullptr;
	}

	bool valid()
	{
		return	e12!=max_mesh_idx_t &&
				e13!=max_mesh_idx_t &&
				e23!=max_mesh_idx_t;

	}

};

struct mesh_triangle
{
	//vertice numbers in the array
	mesh_idx_t v1,v2,v3;

	mesh_triangle(){};
	mesh_triangle(mesh_idx_t i1,mesh_idx_t i2,mesh_idx_t i3):v1(i1),v2(i2),v3(i3){};
	
	bool get_third_vertex (mesh_idx_t vv1, mesh_idx_t vv2, mesh_idx_t* ret) const
	{
		if(is_deg())return false;//degenerated

		if	(vv1==v1 && vv2==v2 || vv1==v2 && vv2==v1){*ret=v3;return true;};
		if	(vv1==v1 && vv2==v3 || vv1==v3 && vv2==v1){*ret=v2;return true;};

		assert(vv1 == v2 && vv2 == v3 || vv1 == v3 && vv2 == v2);

		 *ret = v1; 
		 return true;
	}

	mesh_idx_t* get_edge(tr_info& nfo, mesh_idx_t vv1, mesh_idx_t vv2)
	{
		if	(vv1==v1 && vv2==v2 || vv1==v2 && vv2==v1)return &nfo.e12;
		if	(vv1==v1 && vv2==v3 || vv1==v3 && vv2==v1)return &nfo.e13;
		if	(vv1==v2 && vv2==v3 || vv1==v3 && vv2==v2)return &nfo.e23;
		return nullptr;
	}

	bool is_deg() const {return v1==v2 || v1==v3 || v2==v3;}


	//replace edge v with edge to
	void replace(mesh_idx_t v, mesh_idx_t to)
	{
		if (v==v1)v1=to;
		else if (v==v2)v2=to;
		else v3=to;
	};

	
};


struct mesh_edge
{
	//vertices that form an edge
	//make sure v1<=v2
	mesh_idx_t v1,v2;

	//triangles adjacent to the edge
	//we assume there are no more than two of them
	mesh_idx_t t1,t2;

	void set_vertex(mesh_idx_t vv1,mesh_idx_t vv2)
	{
		if (vv1<=vv2)	{v1=vv1;v2=vv2;}
		else			{v1=vv2;v2=vv1;}
	}


	bool is(mesh_idx_t vv1,mesh_idx_t vv2) const
	{
		return vv1==v1 && vv2==v2 || vv1==v2 && vv2==v1;
	}
	

	//degeneracy
	bool is_deg() const {return v1==v2;};
};


