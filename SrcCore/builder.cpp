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
#include "builder.h"
#include "eval_pool.h"
#include "graph_divider.h"
#include "eval_helpers.h"
#include "ims_val.h"
#include "edge_map.h"

struct ims_graph_base;

using elem = state_stack::elem;

elem* state_stack::from_heap()
{
#ifdef DEVELOPER_VERSION
	++m_states_explored;
#endif
	
	elem* ret;

	if (m_heap) {
		ret = m_heap;
		m_heap = m_heap->m_next;
	} else {
		++m_heap_size;

		if (m_elems.size() < m_heap_size) {
			m_elems.resize(m_heap_size);
		}

		ret = &m_elems[m_heap_size - 1];		
	}



	return ret;
	
}


void state_stack::to_heap(elem* c)
{
	c->release();

	c->m_next = m_heap;
	m_heap = c;
}

void state_stack::release_elems()
{
	for (size_t i = 0; i < m_heap_size; ++i) {
		m_elems[i].release();
	}
	m_heap_size = 0;
	m_heap = nullptr;
}

elem* state_stack::create_root(size_t root)
{
	release_elems();//quick reset

#ifdef DEVELOPER_VERSION
	m_states_explored = 0;
#endif

	////////////////////////////////////////////////////////////////////////
	auto& q = *from_heap();

	q.m = eval_pool::ep.get_id_val();
	
	let& rb = vb[root];
	
	if (rb.defined2()) {
		q.b = rb.get();
		rb->add_ref();
	} else {
		assert(!q.b);
	}

	q.m_next = nullptr;
	q.ver = root;
	q.depth4 = 0;
	q.index = 0;
	q.mes = 1;
	q.id3 = 0;
	q.thickness = 0;//automatic
	
	if (icm) {
		q.auto_color = true;
		q.deep_color = cdpx>=1.0;
		q.c = icm->m_cmap_pal[0];
	}

	return &q;
}

elem* state_stack::divide(elem* ce, size_t* id)
{
	//those who eventually became smaller
	elem* res_list = nullptr;

	ce->m_next = nullptr;
	auto* div_list = ce;//who else are we going to divide?

	constexpr Real min_ratio = 0.9;
	let div_threshold = std::max(min_ratio, cdpx);
	const Real rt = ce->b.defined2() ?
		ce->b.radius() * div_threshold :
		std::numeric_limits<Real>::max();

	while (div_list) {
		if (ims_need_stop()) {
			return nullptr;
		}
		ce = div_list;
		div_list = div_list->m_next;

		let& qv = gm->m_vers[ce->ver];

		////////////////////////////////////////////////////////////////
		let pal_sz = icm ? icm->m_cmap_pal.size() : 1;
		let base_idx = ce->index * qv.sz;
		for (size_t i = 0; i < qv.sz; ++i) {

			auto& ne = *from_heap();

			let eidx = qv.idx + i;
			let& e = gm->m_edges[eidx];

			ne.index = (base_idx +i) % pal_sz;
			ne.ver = e.second;
			ne.thickness = ce->thickness;
			ne.mes = ce->mes * mes_mul[eidx];

			let* m = ri[e.m].m.get();
			if (m->is(ims_val::ETP::matrix)) {
				ne.m = eval_helpers::edge_mul(ce->m.get(), m);
			} else {

				const ims_val* const* sb;
				size_t nb;
				if (m->is(ims_val::ETP::compos)) {
					sb = m->p_v();
					nb = m->get_size();
				} else {
					sb = &m;
					nb = 1;
				}

				ne.m = ce->m;
				
				for (size_t j = 0; j < nb; ++j) {
					let* sbj = sb[j];
					switch (sbj->gt()) {
					case ims_val::ETP::style2:

						continue;
					case ims_val::ETP::thickness:
						ne.thickness = sbj->get_real();//override
						continue;
					default:
						ne.m = eval_helpers::edge_mul(ne.m.get(), m);
					}
				}
			}
#
			assert(ne.m->is_normal());
		
			Real  ratio;

			let& bt = vb[e.second];
			if (bt.defined2()) {
				ne.b = eval_helpers::mul_ball(ne.m.get(), bt.get());

				if (!ne.b) {//cut off
					to_heap(&ne);
					continue;
				}

				ratio = std::min(ne.b.radius() / bt.radius(), min_ratio);
			} else {
				ne.b.reset();
				ratio = min_ratio;
			}

			////////////////////////////////////////////////////////////
			if (icm) {
				ne.id3 = ce->id3;

				if (ce->deep_color) {
					ne.deep_color = true;
				} else {
					ne.deep_color = ratio < cdpx;

					if (ne.deep_color) {

						if (id)ne.id3 = (*id)++;
						//Checking clipping by palette color
						let ip = icm->m_epar == colorize_params::EPAR::e_graph ?
							ne.index : icm->m_edge2pal[eidx];
						if (!icm->m_cmap_pal[ip].checked) {
							to_heap(&ne);
							continue;
						}
					}
				}

				ne.c = ce->c;

				let& cm = icm->m_cmap[e.m];
				if (cm.k < 1) {//there is $style in the edge
					ne.auto_color = false;
					if (ce->auto_color) {
						ne.c = cm;//override
					} else {
						ne.c.mul_color(cm);
					}
				} else {
					ne.auto_color = ce->auto_color;
					if (ce->auto_color && ne.deep_color) {

						let ip = icm->m_epar == colorize_params::EPAR::e_graph ?
							ne.index : icm->m_edge2pal[eidx];
						if (ce->deep_color) {
							ne.c.mul_color(icm->m_cmap_pal[ip]);
						} else {
							ne.c = icm->m_cmap_pal[ip];
						}
					}
				}
			}


			//the new ball is defined and is smaller than the old defined one
			if (ne.b.defined2() && ne.b.radius() <= rt) {
				ne.m_next = res_list;
				res_list = &ne;
			} else {//divide further
				ne.m_next = div_list;
				div_list = &ne;
			}
		};

		to_heap(ce);
	}


	return res_list;
}

elem* elem::next()
{
	return m_next;
}

void elem::set_next(elem* nxt)
{
	m_next = nxt;
}

elem* elem::append(elem* tail)
{
	//merge the list
	for (auto* q = this; q; q = q->m_next) {
		if (!q->m_next) {
			q->m_next = tail;
			return q;
		}
	}
	return nullptr;
}

void elem::release()
{
	m.reset();
	b.reset();
}


void builder::adjust3d(
	camera<Real>& cam, 
	const size_t tw, 
	const size_t th, 
	const float thickness, 
	const subspace_info<Real>& si,
	std::span<const edge_map> ri,
	std::span<const edge_ball> vb, 
	const ims_graph_base& ig, 
	const size_t vroot)
{
	boost::ignore_unused(thickness);
	//boost::timer::auto_cpu_timer t;
	cam_proj<Real> cp;
	cp.init(cam, tw, th);

	Real tfv, tfh, cfv, cfh;
	Real minz = std::numeric_limits<Real>::max();

	tfv = cp.m_tf2_ver;
	tfh = tfv * tw / th;
	cfv = sqrt(1 + tfv * tfv);
	cfh = sqrt(1 + tfh * tfh);

	graph_divider gd;
	gd.init_gd(ri, vb, ig, vroot);

	box<Real> bx;
	gd.get_box(bx);
	let rad = bx.max_size() / 2 * 0.05;

	gd.divide_prec(ri, vb, ig, si,
		[&](const ims_val* b)
		{
		
			let r = edge_ball::radius(b);
			
			if (r > rad)return false;

			//TODO: optimize as for 2D

			let p = cam.m * (edge_ball::center(b) - cam.m_loc);
			//let w = sqrt(p(0)*p(0) + p(1)*p(1));
			let dz = std::max(
				(std::abs(p(0)) + r * cfh) / tfh,
				(std::abs(p(1)) + r * cfv) / tfv);

			minz = std::min(minz, p(2) - dz);

			return true;
		});

	cam.m_loc += minz * cam.m.row(2);
}



void builder::set_section(subspace_info<Real>& si, 
	const ifs_metrics<Real>::metrics& me)
{
	si.origin = me.C;

	auto& b = si.basis_user;

	size_t start_idx = me.Q.cols() - b.cols();

	let eps = ims_num_traits<Real>::almost_zero();

	while (start_idx > 0 &&
		std::abs(me.I(start_idx) - me.I(start_idx - 1)) < eps)
	{
		--start_idx;
	}

	for (int c = 0; c < b.cols(); ++c) {
		b.col(b.cols() - 1 - c) = me.Q.col(start_idx + c);
	}
}

void builder::adjust_box(
	box<Real>& dst, 
	double prec,
	const subspace_info<Real>& si, 
	std::span<const edge_map> ri,
	std::span<const edge_ball> vb, 
	const ims_graph_base& ig, 
	const size_t root)
{
	dst.clear();

	graph_divider gd;
	gd.init_gd(ri, vb, ig, root);
	gd.get_box(dst);
	let rad = dst.max_size() / 2 * prec;
	dst.clear();

	gd.divide_prec(ri, vb, ig, si,
		[&](const ims_val* b)
		{
			let r = edge_ball::radius(b);

			let& pc = edge_ball::center(b);

			if (r > rad) {
				return dst.contain(pc, r);
			}

			dst.add_point(pc, r);
			return true;
		});
}



void builder::adjust2d(
	camera_ex& cc,
	const subspace_info<Real>& si,
	std::span<const edge_map> ri,
	std::span<const edge_ball> vb,
	const ims_graph_base& ig,
	size_t root,
	size_t tw,
	size_t th,
	float thickness,
	bool is2d)
{
	box<Real> dst;
	auto& sd = cc.m_sd;

	if (cc.empty(2)) {
		sd.a = 0;
	}

	Eigen::Matrix2<Real> rot;

	auto si_a = si;//copy
	if (is2d) {
		let a = sd.a * boost::math::constants::pi<Real>() / 180;
		let c = cos(a);
		let s = sin(a);
		rot << c, s, -s, c;
		si_a.basis = si_a.basis * rot;
	}

	builder::adjust_box(
		dst,
		0.01,
		si_a,
		ri,
		vb,
		ig,
		root);

	if (ims_need_stop()) {
		return;
	}

	assert(!dst.empty());

	dst.adjust();

	auto vc = dst.get_center();

	if (is2d) {
		vc = rot * vc;
	}

	sd.c[0] = vc(0);
	Real bw = dst.size(0);
	Real bh;
	if (vc.size() > 1) {
		sd.c[1] = vc(1);
		bh = dst.size(1);
	} else {
		sd.c[1] = 0;
		bh = 0;
	}

	let twh = std::min(tw, th);
	let ps = std::max(bw / tw, bh / th);
	sd.r = ps * twh / 2;
	//expand by about 3 pixels
	sd.r *= 1 + (1 + 2 * thickness) / twh;

	cc.m_2d_empty = false;
}

