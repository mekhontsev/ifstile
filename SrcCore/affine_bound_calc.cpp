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
#include "affine_bound_calc.h"
#include "matrix_helper.h"
#include "eval_pool.h"
#include "eval_helpers.h"
#include "edge_ball.h"
#include "ims_val.h"
#include "ims_num_traits.h"
#include "ims_graph.h"
#include "edge_map.h"
#include "cardinality.h"


////////////////////////////////////////////////////////////////////
//components that depend on themselves must be normalized
//the rest - according to a simplified scheme, using only external normalization
////////////////////////////////////////////////////////////////////

//break up all portions of the component until the compression ratios are less than 0.5
//or until we move to another component
//the goal is to break up finely enough to refine the centers

////////////////////////////////////////////////////////////////////

//let's fix some h (relative error)
//let ri=max|A*xj+b-xi| - estimates of balls from below for portion i
//denote Ri=ri*(1+h)
//split everyone so that the following happens:
// if i,j is from one component: |A|rj<=ri*h/(1+h)
// if i,j are different: |A*xj+b-xi|+|A|Rj <= ri(1+h)
//then we get for the case of one component:
//|A*xj+b-xi|+|A|Rj <= ri +|A|Rj =ri +|A|rj*(1+h)<=ri+ri*h=Ri
//that is, in any case |A*xj+b-xi|+|A|Rj <= Ri
//therefore Ri - the required radii
//it's better to split up those who have the maximum (|A*xj+b-xi|+|A|Rj)/Ri

//if the set | A*xj + b - xi | <eps for all elements
//then we consider the set to be a point set
////////////////////////////////////////////////////////////////

//if the centers of the component sets are given, find the radii

//first, split all edges leading to the same component
//until the compression ratios are less than eps

//find an estimate of the radius of the spheres around each component vertex
//ui - the upper bound for each sphere
//|A*xj+b- xi| +|A|uj < ui
//|A*xj+b- xi| +eps*uj < ui
//|A*xj+b- xi| +eps*u < u - a single estimate for all portions
//|A*xj+b- xi| < u(1 - eps)
//u = max(|A*xj+b-xi|)/(1-eps)
//for existing components
//|A*xj+b-xi|+|A|Rj < ui
//ui=max(|A*xj+b-xi|+|A|Rj )

//split all parts of the component until the compression ratios are less than 0.5
//or until we move to another component




void affine_bound_calc::calc_bounds(
	std::span <edge_ball> vb,
	std::span<const cardinality> pcalc,
	std::span<const edge_map> ri,
	const ims_graph& g,
	const Real h)
{

	IMS_SCOPE([&] {reset_heap(); });

	m_comp_valid.resize(g.m_comp.size());
	std::fill(m_comp_valid.begin(), m_comp_valid.end(), true);

	if (g.empty()) {
		return;//ready
	}

	reset_heap();

	////////////////////////////////////////////////////////////////////

	//we'll set the flag that we'll find the size later, as needed
	for (size_t i = 0; i < g.num_ver(); ++i) {
		let comp_idx = g.m_ver2com[i];
		if (comp_idx == ims_max)continue;
		if (pcalc[comp_idx] > cardinality::point &&
			!g.m_comp[comp_idx].need_norm)
		{
			vb[i].set_undef2();
		}
	}

	constexpr size_t max_list = 100000;
	constexpr Real threshold = 0.5;

	m_elems.resize(g.num_ver());
	for (size_t i = 0; i < g.num_ver(); ++i) {
		m_elems[i].clear();
	}

	
	for (size_t comp_idx = 0; comp_idx < g.m_comp.size(); ++comp_idx) {
		let card = pcalc[comp_idx];

		if (card == cardinality::error) {
			m_comp_valid[comp_idx] = false;
			continue;
		}

		let& c = g.m_comp[comp_idx];
		if (!c.need_norm)continue;
		if (card <= cardinality::point) {
			continue;//the component is normalized
		}

		////////////////////////////////////////////////////////////////////////
		//check dependencies
		
		//over all vertices of the component
		for (size_t j = 0; j < c.num_ver; ++j) {
			let v = g.m_ver_sorted[j + c.idx_sorted];
			let ne = g.num_edges(v);
			for (size_t e = 0; e < ne; ++e) {
				let ct = g.m_ver2com[g.get_edge(v, e).second];
				if (!m_comp_valid[ct]) {
					m_comp_valid[comp_idx] = false;
					break;
				}
			}
			if (!m_comp_valid[comp_idx]) {
				break;
			}
		}

		if (!m_comp_valid[comp_idx]) {
			continue;
		}
		////////////////////////////////////////////////////////////////////////
		//initial fragmentation, up to compression ratios < threshold
		//across all component vertices
		for (size_t j = 0; j < c.num_ver && m_comp_valid[comp_idx]; ++j) {
			let v = g.m_ver_sorted[j + c.idx_sorted];

			auto& lst = m_elems[v];
			lst.clear();

			//initial filling of component parts
			{
				auto& e = from_heap();

				e.vt = v;
				e.nrm = 1;
				e.d = 0;
				e.m = eval_pool::ep.get_id_val();
				
				lst.push_back(&e);
			}

			velem ready_list;

			while (!lst.empty()) {

				if (ims_need_stop()) {
					return;
				};

				auto* q = lst.pop_front();

				assert(g.m_ver2com[q->vt] == comp_idx);

				let sne = g.num_edges(q->vt);


				for (size_t e = 0; e < sne; ++e) {
					let& qe = g.get_edge(q->vt, e);

					auto& ne = from_heap();
					ne.vt = qe.second;
					ne.m = eval_helpers::edge_mul(q->m.get(), ri[qe.m].m.get(), true);
					
					//assert(ne.m.m_v2->is(ims_val_b::ETP::affine, ims_val_b::EST::Real));

					ne.nrm = eval_helpers::norm_adet(ne.m.get());

					if (ne.vt == v
						&& ne.nrm + ims_num_traits<Real>::almost_zero() >= 1
					) {
						//possibly an expanding loop, checking the eigenvalues
						let dim = vb[v].dim();
						if (matrix_helper::is_not_contracting(ne.m->get_affine(), dim)) {
							m_comp_valid[comp_idx] = false;
							to_heap(ne);
							break;
						}
					}

					if (ready_list.size7 + lst.size7 > max_list) {
						m_comp_valid[comp_idx] = false;//exceeded the limit
						to_heap(ne);
						break;
					}

					let other_com = g.m_ver2com[ne.vt] != comp_idx;
					if (other_com || ne.nrm < threshold) {
						ready_list.push_front(&ne);
					} else {
						lst.push_front(&ne);
					}
				}

				if (!m_comp_valid[comp_idx]) {
					clear_list(ready_list);
					clear_list(lst);
					break;//continue, excluding the unsuitable ones
				}
			}

			lst = ready_list;
			ready_list.clear();

		}//over all vertices of the component

		if (ims_need_stop()) {
			return;
		};

		if (!m_comp_valid[comp_idx]) {
			continue;
		}

		////////////////////////////////////////////////////////////////////
		//now the positions of the centers are known for all sets
		//find the radii of the balls around

		//step 1: unified estimate of the radius of all component sets
		let eps3 = 0.5;
		Real u = 0;
		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = g.m_ver_sorted[j + c.idx_sorted];

			let& ve = m_elems[v];
			let& bs = vb[v];

			for (auto* q = ve.first; q; q = q->next) {

				let& bt = vb[q->vt];

				edge_ball b(eval_helpers::mul_ball(q->m.get(), bt.get(), true));
				if (b->get_size() < bs->get_size()) {
					b.reset(eval_helpers::extend_real_vector_size(b.get(), bs->get_size()));
				}
				let d = b.center() - bs.center();
				q->d = d.norm();

				let other_com = g.m_ver2com[q->vt] != comp_idx;
				if (other_com) {
					let r = q->d + q->nrm * bt.radius();
					u = std::max(u, r);
				} else {
					u = std::max(u, q->d / (1 - eps3));
				}
			}
		}


		//Step 2: We sequentially fit the spheres into each other without splitting the portions
		//As a result, we obtain our own estimate of the radius for each component set

		const Real prec = 1.05;

		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = g.m_ver_sorted[j + c.idx_sorted];
			vb[v].set_radius(u);
		}
		//total radius of the component's spheres
		Real sum_rad = u * c.num_ver;


		for (size_t iter_idx = 0;; iter_idx++) {

			if (ims_need_stop()) {
				return;
			};

			//specify the radii with a fixed center
			for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
				let v = g.m_ver_sorted[j + c.idx_sorted];

				Real ma = 0;
				for (let* q = m_elems[v].first; q; q = q->next) {
					let mar = q->d + q->nrm * vb[q->vt].radius();
					ma = std::max(ma, mar);
				}
				vb[v].set_radius(ma);

#if 0
				if(bs.r > ma * prec && bs.r > ims_num_traits<Real>::almost_zero()){
					ready = false;
				}
#endif
			};


			Real sum = 0;

			//shift the centers
			for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
				let v = g.m_ver_sorted[j + c.idx_sorted];

				m_box_temp.clear();
				auto& bs = vb[v];
				//as the dimension increases, the complexity grows exponentially
				//it's better to use a variation of the MEB algorithm
				for (let* q = m_elems[v].first; q; q = q->next) {
					let& bt = vb[q->vt];

					edge_ball b(eval_helpers::mul_ball(q->m.get(), bt.get(), true));
					m_box_temp.add_point(b.center(), q->nrm* bt.radius());
					
					
				}

				let cn = m_box_temp.get_center();
				let r = bs.radius() + (bs.center() - cn).norm();
				bs.set_radius(r);
				bs.center() = cn;

				sum += bs.radius();
			}

			//restore information about offsets
			for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
				let v = g.m_ver_sorted[j + c.idx_sorted];
				let& bs = vb[v];
				for (auto* q = m_elems[v].first; q; q = q->next) {

					let& bt = vb[q->vt];

					edge_ball b(eval_helpers::mul_ball(q->m.get(), bt.get(), true));
					if (b->get_size() < bs->get_size()) {
						b.reset(eval_helpers::extend_real_vector_size(b.get(), bs->get_size()));
					}
					let d = b.center() - bs.center();
					q->d = d.norm();					
				}
			}

			if (sum * prec >= sum_rad && iter_idx > 1) {
				break;//done - couldn't reduce the radius much
			}
			sum_rad = sum;
		}


		//step 3: further subdivide the portions, refining the radii without changing the centers

		//fill in the estimates from below
		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = g.m_ver_sorted[j + c.idx_sorted];
			auto& ve = m_elems[v];
			ve.r = 0;
			for (let* q = ve.first; q; q = q->next) {
				ve.r = std::max(ve.r, q->d);
			}
			//	assert(vb[v].r*eps <= ve.r*prec);
		}


		for (;;) {

			//has any component sphere increased?
			bool divided = false;

			for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
				let v = g.m_ver_sorted[j + c.idx_sorted];

				if (ims_need_stop()) {
					return;
				};

				auto& lst = m_elems[v];
			

				//element to be split
				auto* q = lst.first;
				affine_elem* prev = nullptr;
				for (; q; q = lst.get_next(prev)) {
					let& bt = vb[q->vt];
					let dc = q->d + q->nrm * bt.radius();

					if (dc > (1 + h) * lst.r) {
						break;//needs to split
					};

					if (dc < lst.r) {//the element can be removed
						lst.remove_next(prev);
						to_heap(*q);
					} else {
						prev = q;
					}
				};


				if (!q) {
					continue;//nobody split up, next vertex
				};

				if (prev) {
					lst.exchange(prev);
				};


				//remove q from the list
				lst.pop_front();

				let& bs = vb[v];

				let sne = g.num_edges(q->vt);

				for (size_t e = 0; e < sne; ++e) {
					let& qe = g.get_edge(q->vt, e);

					auto& ne = from_heap();
					ne.vt = qe.second;
					ne.m = eval_helpers::edge_mul(q->m.get(), ri[qe.m].m.get(), true);

					//find singular values and determinant
				
					ne.nrm = eval_helpers::norm_adet(ne.m.get());

					let& nvt = vb[ne.vt];

					edge_ball b(eval_helpers::mul_ball(ne.m.get(), nvt.get(), true));
					if (b->get_size() < bs->get_size()) {
						b.reset(eval_helpers::extend_real_vector_size(b.get(), bs->get_size()));
					}
					let d = b.center() - bs.center();
					ne.d = d.norm();
					//update the lower bound
					if (lst.r < ne.d) {
						lst.r = ne.d;
					};

					//It's important not to insert it at the beginning, otherwise it might freeze.
					//For example, when all the edges of a vertex map the center
					//of the bounding sphere to itself (graph directed systems)
					lst.push_back(&ne);
				}

				to_heap(*q);

				if (lst.size7 > max_list) {
					m_comp_valid[comp_idx] = false;
					clear_list(lst);
					break;//will lead to transition to the next component
				}

				divided = true;
			};


			if (!divided) {
				break;
			}

		};//for by component vertices

		//fill the radii
		if (m_comp_valid[comp_idx]) {
			for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
				let v = g.m_ver_sorted[j + c.idx_sorted];

				vb[v].set_radius(m_elems[v].r* (1 + h));
				clear_list(m_elems[v]);
			};
		}
	}//by components
}

////////////////////////////////////////////////////////////////////////////////

void affine_bound_calc::reset_heap()
{
	for (size_t i = 0; i < m_num_used; ++i) {
		m_elems_heap[i].m.reset();
	}

	m_free_elems.clear();
	m_num_used = 0;
}

affine_bound_calc::affine_elem& affine_bound_calc::from_heap()
{
	affine_elem* ret = nullptr;

	if (m_free_elems.empty()) {//get from heap
		if (m_num_used == m_elems_heap.size()) {
			m_elems_heap.emplace_back();
		}
		ret = &m_elems_heap[m_num_used++];
	} else {
		ret = m_free_elems.back();
		m_free_elems.pop_back();
	}

	return *ret;
}

void affine_bound_calc::to_heap(affine_elem& e)
{
	e.m.reset();
	m_free_elems.push_back(&e);
}

void affine_bound_calc::clear_list(velem& lst)
{
	while (lst.first) {
		auto* d = lst.first;
		lst.first = d->next;
		to_heap(*d);
	}
	lst.clear();
}


////////////////////////////////////////////////////////////////////////////////

void affine_bound_calc::velem::clear()
{
	first = nullptr;
	last = nullptr;
	size7 = 0;
	r = 0;
}

bool affine_bound_calc::velem::empty() const
{
	return !first;
}

affine_bound_calc::affine_elem* affine_bound_calc::velem::get_next(affine_elem* p)
{
	return  p ? p->next : first;
}

affine_bound_calc::affine_elem* affine_bound_calc::velem::remove_next(affine_elem* p)
{
	if (!p) {
		return pop_front();
	}

	auto* e = p->next;
	p->next = e->next;

	if (e == last) {
		last = p;
	}

	--size7;

	return e;
}

affine_bound_calc::affine_elem* affine_bound_calc::velem::pop_front()
{
	auto* ret = first;

	assert(size7 > 0);

	if (first == last) {
		assert(!first->next);
		clear();
	} else {
		first = first->next;
		--size7;
	}

	return ret;
}

void affine_bound_calc::velem::push_front(affine_elem* e)
{
	e->next = first;

	if (empty()) {
		last = e;
	}
	first = e;

	++size7;
}

void affine_bound_calc::velem::push_back(affine_elem* e)
{
	e->next = nullptr;
	if (empty()) {
		assert(!first);
		first = e;
	} else {
		last->next = e;
	}
	last = e;

	++size7;
}

void affine_bound_calc::velem::exchange(affine_elem* e)
{
	assert(e->next && e != last);//this is the same property

	first = e->next;
	last->next = e;
	last = e;
	e->next = nullptr;
}
