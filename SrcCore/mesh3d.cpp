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
#include "mesh3d.h"
#include "ims_file.h"
#include "version.h"



void mesh::post_process()
{
	assert(m_uvw.empty() || m_uvw.size() == m_ver.size());

	//assign indices (the triangles already contain vertex numbers)
	//assume the number of vertices is less than 4 billion (32 bits)
	size_t nv = m_ver.size();
	for (size_t i = 0; i < nv; ++i)m_ver[i].index = (mesh_idx_t)i;

	//sort by coordinates
	std::sort(m_ver.begin(), m_ver.end(),
		[](mesh_vertex_ex& e1, mesh_vertex_ex& e2)->bool {
			if (e1.p.x < e2.p.x)return true;
			if (e1.p.x > e2.p.x)return false;
			if (e1.p.y < e2.p.y)return true;
			if (e1.p.y > e2.p.y)return false;
			if (e1.p.z < e2.p.z)return true;
			if (e1.p.z > e2.p.z)return false;
			return false;//the same
		}
	);

	if (ims_need_stop()) {
		return;
	}

	//set duplicates
	mesh_vertex_ex* last_uniq = &m_ver[0];
	for (size_t i = 1; i < nv; ++i) {
		mesh_vertex_ex& v = m_ver[i];
		if (v.p.eq(last_uniq->p)) {
			v.set_dup(last_uniq->index);
		} else {
			last_uniq = &v;
		}
	}

	//restore order
	std::sort(m_ver.begin(), m_ver.end(),
		[](mesh_vertex_ex& e1, mesh_vertex_ex& e2) {
			return e1.index < e2.index;
		}
	);


	if (ims_need_stop()) {
		return;
	}


	del_dup_vertexes();
	del_deg_triangles();


	///////////////////////////////////////////////////////////////////////
	//create a list of all edges
	///////////////////////////////////////////////////////////////////////

	for (mesh_idx_t i = 0; i < (mesh_idx_t)m_tr.size(); ++i) {
		mesh_triangle& mt = m_tr[i];
		{ m_edges.emplace_back(); mesh_edge& e = m_edges.back(); e.set_vertex(mt.v1, mt.v2); e.t1 = i; e.t2 = i; }
		{ m_edges.emplace_back(); mesh_edge& e = m_edges.back(); e.set_vertex(mt.v1, mt.v3); e.t1 = i; e.t2 = i; };
		{ m_edges.emplace_back(); mesh_edge& e = m_edges.back(); e.set_vertex(mt.v2, mt.v3); e.t1 = i; e.t2 = i; };
	}

	std::sort(m_edges.begin(), m_edges.end(),
		[](const mesh_edge& e1, const mesh_edge& e2)->bool {
			if (e1.v1 < e2.v1)return true;
			if (e1.v1 > e2.v1)return false;
			return e1.v2 < e2.v2;
		}
	);

	//remove duplicates, filling in information about adjacent triangles
	size_t ne = 1;
	mesh_edge* euniq = &m_edges[0];

	//start with the second edge
	for (size_t i = 1; i < m_edges.size(); ++i) {
		mesh_edge& e = m_edges[i];


		if (euniq->v1 == e.v1 && euniq->v2 == e.v2) {
#ifndef NDEBUG
			//look ahead, may ve we have three triangles adjacent to one edge...

#if 0
			if (i + 1 < m_edges.size()) {
				mesh_edge& enx = m_edges[i + 1];
				assert(e.v1 != enx.v1 || e.v2 != enx.v2);
			}
#endif

#endif
			assert(euniq->t2 != e.t1);
			euniq->t2 = e.t1;

		} else {
			euniq = &m_edges[ne++];
			*euniq = e;
		}
	}
	m_edges.resize(ne);

	//reserve information about the edges
	for (size_t i = 0; i < m_tr.size(); ++i) {
		m_trinfo.emplace_back();
		tr_info& nfo = m_trinfo.back();
		//to distinguish triangles adjacent to two others
		nfo.e12 = nfo.e13 = nfo.e23 = max_mesh_idx_t;
		nfo.m_data = max_mesh_idx_t;//no information about the component
	}

	//fill in the information about the edges in the triangles
	for (mesh_idx_t i = 0; i < m_edges.size(); ++i) {
		mesh_edge& e = m_edges[i];
		*m_tr[e.t1].get_edge(m_trinfo[e.t1], e.v1, e.v2) = i;
		*m_tr[e.t2].get_edge(m_trinfo[e.t2], e.v1, e.v2) = i;
	}

	if (ims_need_stop()) {
		return;
	}


	///////////////////////////////////////////////////////////////////////
	//break the triangles into connected components
	big_array<mesh_idx_t> trc;//stack with triangle indices
	unsigned cn = 0;//current component index
	mesh_idx_t from = 0;//possible index of the first unprocessed triangle

	while (from < m_trinfo.size()) {

		for (; from < m_trinfo.size(); ++from) {
			if (m_trinfo[from].m_data == max_mesh_idx_t)break;
		}

		if (from < m_trinfo.size()) {
			m_trinfo[from].m_data = cn;
			trc.emplace_back(from);
		}

		//the trc array will grow during iterations!
		for (mesh_idx_t i = 0; i < trc.size(); ++i) {

			tr_info& nfo = m_trinfo[trc[i]];

			ASSUME(nfo.m_data == cn);

			mesh_idx_t q;

			if (nfo.e12 != max_mesh_idx_t) {
				q = m_edges[nfo.e12].t1;
				if (m_trinfo[q].m_data == max_mesh_idx_t) { m_trinfo[q].m_data = cn; trc.emplace_back(q); }
				q = m_edges[nfo.e12].t2;
				if (m_trinfo[q].m_data == max_mesh_idx_t) { m_trinfo[q].m_data = cn; trc.emplace_back(q); }
			}

			if (nfo.e23 != max_mesh_idx_t) {
				q = m_edges[nfo.e23].t1;
				if (m_trinfo[q].m_data == max_mesh_idx_t) { m_trinfo[q].m_data = cn; trc.emplace_back(q); }
				q = m_edges[nfo.e23].t2;
				if (m_trinfo[q].m_data == max_mesh_idx_t) { m_trinfo[q].m_data = cn; trc.emplace_back(q); }
			}

			if (nfo.e13 != max_mesh_idx_t) {
				q = m_edges[nfo.e13].t1;
				if (m_trinfo[q].m_data == max_mesh_idx_t) { m_trinfo[q].m_data = cn; trc.emplace_back(q); }
				q = m_edges[nfo.e13].t2;
				if (m_trinfo[q].m_data == max_mesh_idx_t) { m_trinfo[q].m_data = cn; trc.emplace_back(q); }
			}
		}

		trc.resize(0);
		cn++;
	}

	assert(trc.empty());

	struct comp_info
	{
		mesh_idx_t x1, x2, y1, y2, z1, z2;//numbers of extreme vertices

		//normals at extreme vertices
		point3d x1_nr, x2_nr, y1_nr, y2_nr, z1_nr, z2_nr;

		comp_info()
		{
			constexpr float mn = std::numeric_limits<float>::lowest();
			constexpr float mx = std::numeric_limits<float>::max();

			p1.set(mx, mx, mx);
			p2.set(mn, mn, mn);
			m_orient = 0;

			x1_nr.clear();	x2_nr.clear();
			y1_nr.clear();	y2_nr.clear();
			z1_nr.clear();	z2_nr.clear();
		}

		//extreme coordinates
		point3f p1, p2;

		void update(mesh_idx_t idx, const point3f& p)
		{
			if (p.x < p1.x) { x1 = idx; p1.x = p.x; };
			if (p.y < p1.y) { y1 = idx; p1.y = p.y; };
			if (p.z < p1.z) { z1 = idx; p1.z = p.z; };

			if (p.x > p2.x) { x2 = idx; p2.x = p.x; };
			if (p.y > p2.y) { y2 = idx; p2.y = p.y; };
			if (p.z > p2.z) { z2 = idx; p2.z = p.z; };
		};

		//>0 for correct orientation, <0 for incorrect
		double m_orient;

	};

	//analyze the components
	if (cn > 0) {
		//find extreme vertices
		std::vector<comp_info> comp(cn);
		for (mesh_idx_t i = 0; i < m_tr.size(); ++i) {
			const mesh_triangle& mt = m_tr[i];
			auto& ci = comp[m_trinfo[i].m_data];

			ci.update(mt.v1, m_ver[mt.v1].p);
			ci.update(mt.v2, m_ver[mt.v2].p);
			ci.update(mt.v3, m_ver[mt.v3].p);
		}

		//sum the normals over the extreme vertices
		for (mesh_idx_t i = 0; i < m_tr.size(); ++i) {
			const mesh_triangle& mt = m_tr[i];
			auto& ci = comp[m_trinfo[i].m_data];

			point3d nr;
			face_normal(mt.v1, mt.v2, mt.v3, nr);
			if (mt.v1 == ci.x1 || mt.v2 == ci.x1 || mt.v3 == ci.x1)ci.x1_nr.add(nr);
			if (mt.v1 == ci.x2 || mt.v2 == ci.x2 || mt.v3 == ci.x2)ci.x2_nr.add(nr);
			if (mt.v1 == ci.y1 || mt.v2 == ci.y1 || mt.v3 == ci.y1)ci.y1_nr.add(nr);
			if (mt.v1 == ci.y2 || mt.v2 == ci.y2 || mt.v3 == ci.y2)ci.y2_nr.add(nr);
			if (mt.v1 == ci.z1 || mt.v2 == ci.z1 || mt.v3 == ci.z1)ci.z1_nr.add(nr);
			if (mt.v1 == ci.z2 || mt.v2 == ci.z2 || mt.v3 == ci.z2)ci.z2_nr.add(nr);
		}

		for (unsigned i = 0; i < cn; i++) {
			auto& ci = comp[i];
			ci.x1_nr.normalize();
			ci.x2_nr.normalize();
			ci.y1_nr.normalize();
			ci.y2_nr.normalize();
			ci.z1_nr.normalize();
			ci.z2_nr.normalize();

			ci.m_orient = ci.x2_nr.x - ci.x1_nr.x +
				ci.y2_nr.y - ci.y1_nr.y +
				ci.z2_nr.z - ci.z1_nr.z;

			//cout<<ci.m_orient<<endl;
		}

		//remove triangles from incorrectly oriented components
		for (uint32_t i = 0; i < m_tr.size(); ++i) {
			mesh_triangle& mt = m_tr[i];
			if (comp[m_trinfo[i].m_data].m_orient < 0) {
				mt.v3 = mt.v2 = mt.v1;
			}
		}

		for (auto& e : m_edges) {
			if (comp[m_trinfo[e.t1].m_data].m_orient < 0) {
				e.v2 = e.v1;
			}
		}

	}



	///////////////////////////////////////////////////////////////////////
	//simplification
	///////////////////////////////////////////////////////////////////////


	//set indexes
	for (size_t i = 0; i < m_ver.size(); ++i)m_ver[i].index = (mesh_idx_t)i;


	for (unsigned i = 0; i < 10; ++i) {
		if (!simplify(false))break;
		if (ims_need_stop()) {
			return;
		}
	}

	simplify(true);


	//edges and additional information are no longer needed
	m_edges.resize(0);
	m_trinfo.resize(0);

	del_deg_triangles();
	del_dup_vertexes();
	del_unref_vertexes();
};


void mesh::face_normal(size_t idx, point3d& nr) const
{
	const mesh_triangle& t = m_tr[idx];
	face_normal(t.v1, t.v2, t.v3, nr);
}



void mesh::face_normal(mesh_idx_t v1, mesh_idx_t v2, mesh_idx_t v3, point3d& nr) const
{
	nr = point3d::face_normal(point3d(m_ver[v1].p.data()),
		point3d(m_ver[v2].p.data()),
		point3d(m_ver[v3].p.data()));
}


void mesh::calc_normals()
{
	m_normals.resize(m_ver.size());
	//initialization of normals
	for (size_t i = 0; i < m_ver.size(); ++i) {
		m_normals[i].clear();
	}

	//sum
	for (auto& t : m_tr) {

		point3d p1(m_ver[t.v1].p.data());
		point3d p2(m_ver[t.v2].p.data());
		point3d p3(m_ver[t.v3].p.data());


		point3d p21 = p2 - p1, p31 = p3 - p1, p32 = p3 - p2;
		double 	n21 = 1 / p21.norm(), n31 = 1 / p31.norm(), n32 = 1 / p32.norm();

		//find the angles of the triangle
		double c1, c2, c3;
		c1 = acos((p21.scalar(p31)) * n21 * n31);		//angle at vertex 1
		c2 = acos((p32.scalar(p21)) * -n32 * n21);	//angle at vertex 2
		//c3=acos((p31.scalar(p32))*n31*n32);
		c3 = boost::math::constants::pi<double>() - c1 - c2;

		ASSUME(c1 >= 0 && c2 >= 0 && c3 >= 0);

		//normal to the edge
		const point3d n = point3d::face_normal(p1, p2, p3);

		{
			auto& vn = m_normals[t.v1];
			vn.x += float(n.x * c1);
			vn.y += float(n.y * c1);
			vn.z += float(n.z * c1);
		}
		{
			auto& vn = m_normals[t.v2];
			vn.x += float(n.x * c2);
			vn.y += float(n.y * c2);
			vn.z += float(n.z * c2);
		}
		{
			auto& vn = m_normals[t.v3];
			vn.x += float(n.x * c3);
			vn.y += float(n.y * c3);
			vn.z += float(n.z * c3);
		}
	}

	//normalize
	for (auto& nrm : m_normals) {
		nrm.normalize();
	}
}


double mesh::degenerative_mes(size_t v1i, size_t v2i, size_t v3i)  const
{
	const mesh_vertex_ex& v1 = m_ver[v1i];
	const mesh_vertex_ex& v2 = m_ver[v2i];
	const mesh_vertex_ex& v3 = m_ver[v3i];

	point3d p1(v1.p.data());
	point3d p2(v2.p.data());
	point3d p3(v3.p.data());

	double n21 = p2.dist(p1);
	double n31 = p3.dist(p1);
	double n32 = p3.dist(p2);

	return (n31 + n32) / n21 - 1;
}


void mesh::del_dup_vertexes()
{
	// assign references to triangles that are NOT duplicates
	for (auto& mt : m_tr) {
		mt.v1 = get_uniq(mt.v1);
		mt.v2 = get_uniq(mt.v2);
		mt.v3 = get_uniq(mt.v3);
	}
	//assign final indexes to unique vertices
	size_t num_ver = 0;
	for (auto& v : m_ver) {
		if (v.get_uniq())v.index = static_cast<mesh_idx_t>(num_ver++);
	}

	//assign final vertex indexes to the triangles
	for (auto& mt : m_tr) {
		mt.v1 = m_ver[mt.v1].index;
		mt.v2 = m_ver[mt.v2].index;
		mt.v3 = m_ver[mt.v3].index;
	}

	//collapse the vertices, removing duplicates from the array
	num_ver = 0;
	for (size_t i = 0; i < m_ver.size(); ++i) {
		mesh_vertex_ex& v = m_ver[i];
		if (v.get_uniq()) {
			m_ver[num_ver] = v;
			if (!m_uvw.empty())m_uvw[num_ver] = m_uvw[i];
			num_ver++;
		}
	}

	//remove duplicated vertices
	m_ver.resize(num_ver);
	if (!m_uvw.empty()) {
		m_uvw.resize(num_ver);
	}
};


void mesh::del_deg_triangles()
{
	uint32_t dst = 0;
	for (auto& mt : m_tr) {
		if (!mt.is_deg()) {
			m_tr[dst++] = mt;
		}
	}

	m_tr.resize(dst);
}

void mesh::del_unref_vertexes()
{
	if (m_tr.empty()) {
		m_ver.resize(0);
		m_uvw.resize(0);
		return;
	}

	//store the number of links in the index
	for (auto& v : m_ver) {
		v.index = 0;
	}
	for (auto& t : m_tr) {
		m_ver[t.v1].index++;
		m_ver[t.v2].index++;
		m_ver[t.v3].index++;
	}


	uint32_t num_unref = 0;

	//a vertex that is definitely in use...
	//those that aren't used will become references to it
	uint32_t vi = m_tr[0].v1;

	for (uint32_t i = 0; i < m_ver.size(); ++i) {
		mesh_vertex_ex& v = m_ver[i];
		if (v.index == 0) {
			v.set_dup(vi);
			num_unref++;
		}
		v.index = i;//restore the index
	}

	if (num_unref) {
		del_dup_vertexes();
	}

}

void mesh::restore_info(mesh_idx_t i)
{
	mesh_edge* e = &m_edges[i];
	if (e->is_deg())return;

	mesh_triangle* t1 = &m_tr[e->t1];
	mesh_triangle* t2 = &m_tr[e->t2];

	bool t1d = t1->is_deg();
	bool t2d = t2->is_deg();

	if ((t1d && t2d) || (!t1d && !t2d))return;

	//we rarely go below...

	mesh_idx_t t1i = e->t1;
	mesh_idx_t t2i = e->t2;

	if (t1d) {//swap
		mesh_triangle* swp = t1; t1 = t2; t2 = swp;
		mesh_idx_t swi = t1i; t1i = t2i; t2i = swi;
	}
	//t1 - ok, t2 - degenerate

	mesh_idx_t ei = i;

	while (t2->is_deg()) {
		//find another non-degenerate edge in t2
		//mesh_edge* ne=0;

		//find information about edges t2
		tr_info& ti = m_trinfo[t2i];

		if (ti.e12 == max_mesh_idx_t || ti.e13 == max_mesh_idx_t || ti.e23 == max_mesh_idx_t) {

		}

		mesh_edge* e12, * e13, * e23;
		e12 = ti.e12 != max_mesh_idx_t ? &m_edges[ti.e12] : 0;
		e13 = ti.e13 != max_mesh_idx_t ? &m_edges[ti.e13] : 0;
		e23 = ti.e23 != max_mesh_idx_t ? &m_edges[ti.e23] : 0;

		e->v2 = e->v1;//degenerate the current edge (otherwise we'll get stuck)

		//find the edge
		if (e12 && e12 != e && !e12->is_deg()) { e = e12; ei = ti.e12; } else if (e13 && e13 != e && !e13->is_deg()) { e = e13; ei = ti.e13; } else if (e23 && e23 != e && !e23->is_deg()) { e = e23; ei = ti.e23; } else {
			//assert(false);
			break;//strange...
		}

		//fall over the edge
		if (e->t1 == t2i)	t2i = e->t2;
		else			t2i = e->t1;

		t2 = &m_tr[t2i];
	}

	if (e->t2 == t2i)	e->t1 = t1i;
	else			e->t2 = t1i;

	tr_info& nfo = m_trinfo[t1i];
	if (nfo.e12 == i)nfo.e12 = ei;
	else if (nfo.e13 == i)nfo.e13 = ei;
	else				nfo.e23 = ei;
};

bool mesh::simplify(bool buse)
{
	bool changed = false;

	//////////////////////////////////////////////////////////////////////
	//collapse nearby vertices if they belong to the same edge
	//////////////////////////////////////////////////////////////////////
	for (auto& e : m_edges) {
		if (e.is_deg())continue;

		uint32_t v1i, v2i;
		v1i = get_uniq(e.v1);
		v2i = get_uniq(e.v2);

		if (v1i == v2i)continue;//degenerated edge

		//not duplicates
		mesh_vertex_ex* v1, * v2;
		v1 = &m_ver[v1i];
		v2 = &m_ver[v2i];

		point3d p1(v1->p.data());
		point3d p2(v2->p.data());

		double por = 0.2;
		double por2 = por * por;

		if (p2.dist2(p1) < por2) {
			//gluing vertices
			if (v1i < v2i)	v2->set_dup(v1i);
			else				v1->set_dup(v2i);

			changed = true;
		}
	}

	if (changed) {//restore uniqueness in links to vertices
		for (auto& t : m_tr) {
			t.v1 = get_uniq(t.v1);
			t.v2 = get_uniq(t.v2);
			t.v3 = get_uniq(t.v3);

		}
		for (auto& e : m_edges) {
			e.v1 = get_uniq(e.v1);
			e.v2 = get_uniq(e.v2);
		}
	}

	//restore information about the edges adjacent to the collapsed ones
	for (mesh_idx_t i = 0; i < (mesh_idx_t)m_edges.size(); ++i) {
		restore_info(i);
	}

	//////////////////////////////////////////////////////////////////////
	//remove pairs of identical non-degenerate triangles
	//////////////////////////////////////////////////////////////////////

	for (auto& e : m_edges) {
		if (e.is_deg())continue;

		mesh_triangle& t1 = m_tr[e.t1];
		mesh_triangle& t2 = m_tr[e.t2];



		if (t1.is_deg() || t2.is_deg())continue;

		mesh_idx_t t1vi, t2vi;

		if (!t1.get_third_vertex(e.v1, e.v2, &t1vi)) {
			continue;
		}

		if (!t2.get_third_vertex(e.v1, e.v2, &t2vi)) {
			continue;
		}

		if (t1vi != t2vi)continue;

		tr_info& nfo1 = m_trinfo[e.t1];
		tr_info& nfo2 = m_trinfo[e.t2];

		//find an edge in triangle t1 that does not belong to t2
		mesh_idx_t* ei = nfo1.get(nfo2.e12);
		if (!ei || *ei == max_mesh_idx_t)ei = nfo1.get(nfo2.e13);
		if (!ei || *ei == max_mesh_idx_t)ei = nfo1.get(nfo2.e23);
		if (!ei || *ei == max_mesh_idx_t) {
			//assert(false);
			continue;
		}

		e.set_vertex(e.v1, e.v1);
		t1.replace(e.v2, e.v1);
		t2.replace(e.v2, e.v1);

		restore_info(*ei);

		changed = true;

	}

	//////////////////////////////////////////////////////////////////////
	//reshuffle the skinny triangles
	//////////////////////////////////////////////////////////////////////

#if 0
	vector<bool> used_tr;
	if (buse) {
		used_tr.resize(m_tr.size(), false);
	}
#endif

	for (mesh_idx_t i = 0; i < (mesh_idx_t)m_edges.size(); ++i) {
		mesh_edge& e = m_edges[i];
		if (e.is_deg())continue;

		mesh_triangle& t1 = m_tr[e.t1];
		mesh_triangle& t2 = m_tr[e.t2];


		if (t1.is_deg() || t2.is_deg())continue;

		tr_info& nfo1 = m_trinfo[e.t1];
		tr_info& nfo2 = m_trinfo[e.t2];

		mesh_idx_t t1vi, t2vi;

		if (!t1.get_third_vertex(e.v1, e.v2, &t1vi)) {
			continue;
		}

		if (!t2.get_third_vertex(e.v1, e.v2, &t2vi)) {
			continue;
		}


		//old block

		const double epsilon = 0.1;

		double msh1 = degenerative_mes(e.v1, e.v2, t1vi);
		double msh2 = degenerative_mes(e.v1, e.v2, t2vi);
		if (msh2 < msh1)msh1 = msh2;//place the minimum in msh1

		//degeneracy after rearrangement
		double msh1n = degenerative_mes(t1vi, t2vi, e.v1);
		double msh2n = degenerative_mes(t1vi, t2vi, e.v2);
		if (msh2n < msh1n)msh1n = msh2n;//minimum

		//removing the "saddle" feature
		if (buse && msh1 > epsilon && msh1n > epsilon) {

			//	if (used_tr[e.t1] || used_tr[e.t2])continue;

			if (!nfo1.valid() || !nfo2.valid())continue;

			mesh_idx_t v1, v2, v3, v4;
			v1 = e.v1;
			v2 = e.v2;
			v3 = t1vi;
			v4 = t2vi;


			mesh_idx_t e13, e23, e24, e14;

			if (m_edges[nfo1.e12].is(v1, v3))e13 = nfo1.e12;
			else if (m_edges[nfo1.e13].is(v1, v3))e13 = nfo1.e13;
			else if (m_edges[nfo1.e23].is(v1, v3))e13 = nfo1.e23;
			else {
				continue;
			}

			if (m_edges[nfo1.e12].is(v2, v3))e23 = nfo1.e12;
			else if (m_edges[nfo1.e13].is(v2, v3))e23 = nfo1.e13;
			else if (m_edges[nfo1.e23].is(v2, v3))e23 = nfo1.e23;
			else {
				continue;
			}

			if (m_edges[nfo2.e12].is(v2, v4))e24 = nfo2.e12;
			else if (m_edges[nfo2.e13].is(v2, v4))e24 = nfo2.e13;
			else if (m_edges[nfo2.e23].is(v2, v4))e24 = nfo2.e23;
			else {
				continue;
			}

			if (m_edges[nfo2.e12].is(v1, v4))e14 = nfo2.e12;
			else if (m_edges[nfo2.e13].is(v1, v4))e14 = nfo2.e13;
			else if (m_edges[nfo2.e23].is(v1, v4))e14 = nfo2.e23;
			else {
				continue;
			}


			assert(e13 != e23 && e24 != e14);

			mesh_idx_t t13, t23, t24, t14;

			t13 = m_edges[e13].t1; if (t13 == e.t1)t13 = m_edges[e13].t2;
			t23 = m_edges[e23].t1; if (t23 == e.t1)t23 = m_edges[e23].t2;
			t24 = m_edges[e24].t1; if (t24 == e.t2)t24 = m_edges[e24].t2;
			t14 = m_edges[e14].t1; if (t14 == e.t2)t14 = m_edges[e14].t2;

			assert(m_edges[e13].t1 == e.t1 || m_edges[e13].t2 == e.t1);
			assert(m_edges[e23].t1 == e.t1 || m_edges[e23].t2 == e.t1);
			assert(m_edges[e24].t1 == e.t2 || m_edges[e24].t2 == e.t2);
			assert(m_edges[e14].t1 == e.t2 || m_edges[e14].t2 == e.t2);


			mesh_idx_t v5, v6, v7, v8;

			if (!m_tr[t13].get_third_vertex(v1, v3, &v5))continue;
			if (!m_tr[t23].get_third_vertex(v2, v3, &v6))continue;
			if (!m_tr[t24].get_third_vertex(v2, v4, &v8))continue;
			if (!m_tr[t14].get_third_vertex(v1, v4, &v7))continue;

			point3d p1, p2, p3, p4, p5, p6, p7, p8;
			p1 = m_ver[v1].gp();	p2 = m_ver[v2].gp();
			p3 = m_ver[v3].gp();	p4 = m_ver[v4].gp();
			p5 = m_ver[v5].gp();	p6 = m_ver[v6].gp();
			p7 = m_ver[v7].gp();	p8 = m_ver[v8].gp();


			//degree of badness
			double mes1 = 0;
			{
				point3d n1 = point3d::face_normal(p1, p2, p3);
				point3d n2 = point3d::face_normal(p1, p2, p4);

				double d5, d6, d7, d8;
				point3d q5, q6, q7, q8;

				q5 = p5 - p1; q6 = p6 - p1;
				d5 = n1.scalar(q5); d6 = n1.scalar(q6);
				if (n1.scalar(p4 - p1) > 0) { d5 = -d5; d6 = -d6; }

				q7 = p7 - p1; q8 = p8 - p1;
				d7 = n2.scalar(q7); d8 = n2.scalar(q8);
				if (n2.scalar(p3 - p1) > 0) { d7 = -d7; d8 = -d8; }

				if (d5 < 0 || d6 < 0)continue;
				if (d7 < 0 || d8 < 0)continue;

				if (d5 > 0)mes1 = std::max(mes1, d5 / q5.norm());
				if (d6 > 0)mes1 = std::max(mes1, d6 / q6.norm());
				if (d7 > 0)mes1 = std::max(mes1, d7 / q7.norm());
				if (d8 > 0)mes1 = std::max(mes1, d8 / q8.norm());
			}

			if (mes1 == 0)continue;

			//rotate
			point3d t;

			t = p1; p1 = p4; p4 = p2; p2 = p3; p3 = t;
			t = p5; p5 = p7; p7 = p8; p8 = p6; p6 = t;

			//degree of badness after permutation
			double mes2 = 0;
			{
				point3d n1 = point3d::face_normal(p1, p2, p3);
				point3d n2 = point3d::face_normal(p1, p2, p4);

				double d5, d6, d7, d8;
				point3d q5, q6, q7, q8;

				q5 = p5 - p1; q6 = p6 - p1;
				d5 = n1.scalar(q5); d6 = n1.scalar(q6);
				if (n1.scalar(p4 - p1) > 0) { d5 = -d5; d6 = -d6; }

				q7 = p7 - p1; q8 = p8 - p1;
				d7 = n2.scalar(q7); d8 = n2.scalar(q8);
				if (n2.scalar(p3 - p1) > 0) { d7 = -d7; d8 = -d8; }

				if (d5 > 0)mes2 = std::max(mes2, d5 / q5.norm());
				if (d6 > 0)mes2 = std::max(mes2, d6 / q6.norm());
				if (d7 > 0)mes2 = std::max(mes2, d7 / q7.norm());
				if (d8 > 0)mes2 = std::max(mes2, d8 / q8.norm());
			}

			if (mes2 >= mes1)continue;

			//used_tr[e.t1]=true;used_tr[e.t2]=true;
		} else {

			if (msh1 > epsilon)continue;//the triangle is ok


			if (msh1n <= msh1) {//it will get even worse...
				continue;
			}
		}



		///////////////////////////////////////////////////////////////////
		//information about adjacent triangles has been invalidated in adjacent edges
		//it needs to be restored



		//edges that will jump to another triangle as a result of the exchange
		mesh_idx_t* pe1i;//edge [t1vi,e.v2] jump in 2nd
		mesh_idx_t* pe2i;//edge [t2vi,e.v1] jump in 1st

		pe1i = t1.get_edge(nfo1, t1vi, e.v2);
		pe2i = t2.get_edge(nfo2, t2vi, e.v1);

		mesh_idx_t	e1i = *pe1i;
		mesh_idx_t	e2i = *pe2i;

		if (e1i != max_mesh_idx_t) {
			mesh_edge& e1 = m_edges[e1i];
			if (e1.t1 == e.t1)e1.t1 = e.t2;
			else			e1.t2 = e.t2;
			*t2.get_edge(nfo2, t2vi, e.v1) = i;
			*t2.get_edge(nfo2, e.v1, e.v2) = e1i;
		}

		if (e2i != max_mesh_idx_t) {
			mesh_edge& e2 = m_edges[e2i];
			if (e2.t1 == e.t2)e2.t1 = e.t1;
			else			e2.t2 = e.t1;
			*t1.get_edge(nfo1, t1vi, e.v2) = i;
			*t1.get_edge(nfo1, e.v1, e.v2) = e2i;
		}

		///////////////////////////////////////////////////////////////////


		//rearrange
		t1.replace(e.v2, t2vi);
		t2.replace(e.v1, t1vi);



		e.set_vertex(t1vi, t2vi);

		changed = true;

	}


	return changed;
}

unsigned mesh::calc_actual_uvw() const
{
	bool has_uvw[3] = { false,false,false };

	for (size_t i = 0; i < m_uvw.size(); ++i) {
		const mesh_uvw& v = m_uvw[i];
		if (v[0] > 0)has_uvw[0] = true;
		if (v[1] > 0)has_uvw[1] = true;
		if (v[2] > 0)has_uvw[2] = true;
	}

	unsigned num_uvw = 0;
	if (has_uvw[0])num_uvw = 1;
	if (has_uvw[1])num_uvw = 2;
	if (has_uvw[2])num_uvw = 3;

	return num_uvw;
}

static uint8_t color8bit(double c)
{
	return static_cast<uint8_t>(ims_clamp(c * 255 + .5, 0.0, 255.0));
};


template<typename T>
static void write_var(std::ostream& fs, T v)
{
	fs.write(reinterpret_cast<const char*>(&v), sizeof(T));
};

bool mesh::save_ply(std::ostream& fs) const
{

	fs << "ply\n";
	fs << "format binary_little_endian 1.0\n";
	fs << "comment " << APPLICATION_TITLE << "\n";
	fs << "element vertex " << m_ver.size() << "\n";
	fs << "property float x\n";
	fs << "property float y\n";
	fs << "property float z\n";

	auto num_uvw = calc_actual_uvw();
	if (num_uvw > 0) {
		fs << "property uchar red\n";
		fs << "property uchar green\n";
		fs << "property uchar blue\n";
	}
	fs << "element face " << m_tr.size() << "\n";
	fs << "property list uchar int vertex_indices\n";
	fs << "end_header\n";

	//save vertices and colors
	for (size_t i = 0; i < m_ver.size(); ++i) {
		const auto& v = m_ver[i].p;

		write_var<float>(fs, v.x);
		write_var<float>(fs, v.y);
		write_var<float>(fs, v.z);

		if (num_uvw > 0) {
			const auto& c = m_uvw[i];
			write_var<uint8_t>(fs, color8bit(c.x));
			write_var<uint8_t>(fs, color8bit(c.y));
			write_var<uint8_t>(fs, color8bit(c.z));
		}
	}

	//save triangles
	for (size_t i = 0; i < m_tr.size(); ++i) {
		const auto& t = m_tr[i];
		write_var<uint8_t>(fs, 3);
		write_var<uint32_t>(fs, t.v1);
		write_var<uint32_t>(fs, t.v2);
		write_var<uint32_t>(fs, t.v3);
	}

	return true;
}

void  mesh::clear_mem()
{
	clear();

	m_normals.shrink_to_fit();

	m_ver.shrink_to_fit();
	m_tr.shrink_to_fit();
	m_uvw.shrink_to_fit();

	m_edges.shrink_to_fit();
	m_trinfo.shrink_to_fit();
	m_tr_indexes.shrink_to_fit();
};


void mesh::clear()
{
	m_normals.clear();

	m_ver.clear();
	m_tr.clear();
	m_uvw.clear();

	m_edges.clear();
	m_trinfo.clear();
	m_tr_indexes.clear();

	m_uvw_dim = 0;
}

bool mesh::empty()
{
	return m_tr.empty();
}

