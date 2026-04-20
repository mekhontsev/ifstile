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

#include "palette.h"
#include "ims_bitmap.h"
#include "ims_stage.h"
#include "ims_graph.h"//init_cmaps
#include "edge_ball.h"
#include "big_array.h"
#include "geometry.h"
#include "ifs_metrics.h"
#include "box.h"

struct ims_graph;
struct ims_graph_base;
struct edge_map;


template<typename Real>
uint8_t color8bit(Real c)
{
	return static_cast<uint8_t>(ims_clamp(c * 255 + Real(.5), 0, 255));
};

//Draw a rectangular region inside rgba_view using information from draw_info
template<typename draw_info>
void to_bitmap(
	ims_bitmap& rgba_view,
	const draw_info& di,
	const size_t ax, const size_t ay,	//beginning of the result
	const size_t sx, const size_t sy,	//result size
	const size_t ovs)
{
	if (di.m_img.empty() || rgba_view.empty()) {
		return;//for example, there wasn't enough memory
	}

	auto& rnfo = ims_stage::get();

	let wr = 1.0 / (sx*sy);

	typename draw_info::pixel_RGBA px;

	for (size_t dy = 0; dy < sy; ++dy) {
		auto* it = rgba_view.row_begin(dy+ay)+ax;
		
		for (size_t dx = 0; dx < sx; ++dx) {

			if (ims_need_stop()) {
				return;//instant reaction
			}

			rnfo.work_add(wr);

			double fr = 0, fg = 0, fb = 0, fa = 0;
			
			for (size_t qy = 0; qy < ovs; ++qy) {
				for (size_t qx = 0; qx < ovs; ++qx) {

					if (!di.get_pixel(px, dx*ovs + qx, dy*ovs + qy)) {
						continue;//clean background
					};

					let a = ims_clamp(px(3),0,1);

					fr += (double)px(0)*a;
					fg += (double)px(1)*a;
					fb += (double)px(2)*a;
					fa += (double)a;
				}
			}

			auto& d = *it++;

			if (fa > 0) {//average color
				fr /= fa;
				fg /= fa;
				fb /= fa;
			}
			d.r = color8bit(fr);
			d.g = color8bit(fg);
			d.b = color8bit(fb);
			d.a = color8bit(fa / (ovs*ovs));
			
		}
	}
}


struct  builder
{
	using Real = double;

	static void adjust3d(
		camera<Real>& cam,
		const size_t tw,
		const size_t th,
		const float thickness,
		const subspace_info<Real>& si,
		std::span<const edge_map> ri,
		std::span<const edge_ball> vb,
		const ims_graph_base& ig,
		const size_t vroot);


	//select the most elongated directions, example:
	//k1 <= k2 <= k3 are singular values, so if k1 != k2, we take (k2, k3)
	//otherwise, we take (k1, k2)
	static void set_section(
		subspace_info<Real>& si,
		const ifs_metrics<Real>::metrics& me);


	//find the circumscribing parallelepiped with a given relative error
	static void adjust_box(
		box<Real>& dst,
		double prec,//relative error
		const subspace_info<Real>& si,
		std::span<const edge_map> ri,
		std::span<const edge_ball> vb,
		const ims_graph_base& ig,
		const size_t root);


	static void adjust2d(
		camera_ex& cc,
		const subspace_info<double>& si,
		std::span<const edge_map> ri,
		std::span<const edge_ball> vb,
		const ims_graph_base& ig,
		size_t root,
		size_t tw,
		size_t th,
		float thickness,
		bool is2d);


};


template<typename Real>
struct color_map
{
	using Vec3 = Eigen::Matrix<Real, 3, 1>;
	//kx+t
	Vec3 t;
	Real k;
	bool checked;

	void set_id()
	{
		t.setZero();
		k = 1;
		checked = false;
	}

	//k=1-a, k*(x-c)+c=kx+(1-k)*c
	void init(Real r, Real g, Real b, Real a)
	{
		k = 1 - a;
		t << r * a, g* a, b* a;
		
	};

	//k(krx+tr)+t=k*krx+k*tr+t
	void mul_color(const color_map& right)
	{
		t += right.t * k;
		k *= right.k;
	};

};

template<typename Real>
struct ims_cmap
{
	//2 coloring modes:
	//if the user specified a style
	//use maps: m_cmap
	//if the user did not specify a style
	//use edges: m_edge2pal->m_cmap_pal

	using cmap_type = color_map<Real>;
	//depends on the palette
	std::vector<cmap_type> m_cmap;//indexed by map number
	
	std::vector<size_t> m_edge2pal;//indexed by the global edge number
	std::vector<cmap_type> m_cmap_pal;//indexed by the value m_edge2pal

	colorize_params::EPAR m_epar;

	void init_cmaps(
		std::span<const style<Real>> mi,
		const ims_graph& gm,
		const palette& pal,
		double shift,
		colorize_params::EPAR epar)
	{
		m_epar = epar;
		m_cmap.resize(mi.size());
		for (size_t i = 0; i < mi.size(); ++i) {
			auto& d = m_cmap[i];
			let& s = mi[i];

			d.set_id();
			cmap_type tc;

			for (let& q : s.pal_idx) {
				palette::color c;
				pal.interpolate(c, q + shift);

				tc.init(c[0], c[1], c[2], c[3]);
				d.mul_color(tc);
			}
		}

		let sz = pal.data.size();
		m_cmap_pal.resize(sz);
		for (size_t i = 0; i < sz; ++i) {
			palette::color c;
			pal.interpolate(c, i + shift);
			m_cmap_pal[i].init(c[0], c[1], c[2], c[3]);
			m_cmap_pal[i].checked = pal.data[i].checked_p;
		}


		let pal_size = pal.data.size();

		m_edge2pal.resize(gm.m_edges.size());
		for (size_t v = 0; v < gm.num_ver(); ++v) {
			let& qv = gm.m_vers[v];

			for (size_t i = 0; i < qv.sz; ++i) {
				let eidx = qv.idx + i;//global edge number in the graph
				let& e = gm.m_edges[eidx];

				size_t color_idx;
				if (epar == colorize_params::e_vertex) {
					color_idx = e.second;
				} else {
					if (gm.m_comp[gm.m_ver2com[v]].has_self) {
						color_idx = eidx;
					} else {
						color_idx = i;
					}
				}
				m_edge2pal[eidx] = palette::adjust(color_idx, pal_size);
			}
		}
	}

	
};


struct state_stack
{
	using Real = double;

	struct elem
	{
		elem* m_next;

		size_t depth4;	//depth
		size_t index;
		size_t id3;
		size_t ver;		//the vertex on which the map acts

		Real mes;		//measure
		
		pool_ptr m;		//map
		edge_ball b;	//bounding ball

		Eigen::Matrix<Real, 3, 1> bc;//projected center
		Real ds;	//distance from camera to center (for surface only)

		////////////////////////////////////////////////////////////////////////
		color_map<Real> c;
		Real thickness;

		Real get_thickness(Real def_thickness) const
		{
			return thickness < 1? def_thickness: thickness * def_thickness;
		}

		//The style operator was not found on the element path
		bool auto_color;
		//already deep enough
		bool deep_color;
		////////////////////////////////////////////////////////////////////////
		
		elem* next();
		void set_next(elem* nxt);

		//stick the tail, you can also search for the last element
		//returns the one to whom the tail was stuck
		elem* append(elem* tail);

		void release();
	};

	////////////////////////////////////////////////////////////////////////////
	//never decreases or moves
	big_array<elem> m_elems;

	elem* m_heap = nullptr;
	size_t m_heap_size = 0;//How many elements are occupied in m_elems?

#ifdef DEVELOPER_VERSION
	size_t m_states_explored = 0;
#endif
	
	////////////////////////////////////////////////////////////////////////////
	//parameters must be set before construction
	const ims_graph_base* gm = nullptr;
	const subspace_info<Real>* m_psi;
	std::span<const edge_map> ri;
	std::span<const edge_ball> vb;
	std::span<const Real> mes_mul;
	const ims_cmap<Real>* icm = nullptr;
	double cdpx = 0;
	
	////////////////////////////////////////////////////////////////////////////
	size_t get_dim() const
	{
		return m_psi->get_dim_space();
	}

	elem* create_root(size_t root);
	elem* divide(elem* ce, size_t* id);

	elem* from_heap();
	void to_heap(elem* c);

	void release_elems();
};




