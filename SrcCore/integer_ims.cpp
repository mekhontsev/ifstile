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
#include "integer_ims.h"
#include "neighbors_data.h"
#include "block_info.h"
#include "edge_ball.h"
#include "edge_map.h"
#include "ims_val.h"
#include "eval_pool.h"
#include "matrix_helper.h"
#include "eval_helpers.h"
#include "math_helpers.h"


////////////////////////////////////////////////////////////////////////////
//Amman A3, 2-minute search
//1.47 - old, integer
//1.25 - new, integer
//1.25 - new, rational
//4.08 - new, pure rational, unchecked
//0.50 - new, real
//0.99 - old, real

template<typename Number>
void affine_set_translate_to_zero(Number* ptr, size_t dim)
{
	ptr += dim * dim;
	std::fill(ptr, ptr + dim, 0);
};

template<typename D, typename S>
void convert_to_big(D* d, S* s, size_t num)
{
	let* de = d + num;
	while (d < de) {
		*d++ = { s->numerator(), s->denominator() };
		++s;
	}
}


template<typename Real>
inter_type compare_size(Real adet)
{
	let eps = ims_num_traits<Real>::almost_zero();
	let dif = 1 - adet;
	if (std::abs(dif) < eps) {
		return inter_type::both;
	}
	return dif > 0 ? inter_type::left : inter_type::right;
}

template<typename Number>
bool is_id_rational(const inter_elem& a)
{
	if (a.s0 != a.s1)return false;

	let dim = a.m->extent(0);
	let* p = a.m->gp<Number>();

	for (size_t c = 0; c < dim + 1; ++c) {
		for (size_t r = 0; r < dim; ++r) {
			let& v = *p++;
			if (r == c && v != 1)return false;
			if (r != c && v != 0)return false;
		}
	}

	return true;
}

static bool mul_maps_real(
	inter_elem& a,
	const ims_val* m0,
	const ims_val* m1,
	bool ori)
{
	let dim = m0->extent(0);
	a.m.reset(eval_pool::ep.get_affine_real(dim));

	eval_helpers::affine_mul_real(a.m.get(), m0, m1);

	if (ori) {
		affine_set_translate_to_zero(a.m->p_r(), dim);
	}

	a.bits = 0;

	return true;
}


static bool is_id_real(const inter_elem& a, double prec)
{
	if (a.s0 != a.s1)return false;
	//if (rtype2 == inter_type::left || rtype2 == inter_type::right)return false;

	let dim = a.get_dim();
	let* p = a.m->p_r();

	for (size_t c = 0; c < dim + 1; ++c) {
		for (size_t r = 0; r < dim; ++r) {
			let& v = *p++;
			if (r == c && std::abs(v - 1) >= prec)return false;
			if (r != c && std::abs(v) >= prec)return false;
		}
	}

	return true;
};


template<typename Number>
bool mul_maps_rational(
	inter_elem& a,
	const ims_val* m0,
	const ims_val* m1,
	bool ori)
{
	let dim = m0->extent(0);

	a.m.reset(eval_pool::ep.get_affine<Number>(dim));

	auto* dst = a.m->gp<Number>();

	mul_affine_rational(
		dst,
		m0->gp<Number>(),
		m1->gp<Number>(),
		dim);

	if (ori) {
		affine_set_translate_to_zero(dst, dim);
	}

	a.bits = number_of_bits(rational_vec_magnitude(dst, a.get_sz()));

	return true;
};



bool mul_maps_rational_checked(
	inter_elem& a,
	const ims_val* m0,
	const ims_val* m1,
	bool ori)
{
	let dim = m0->extent(0);

	a.m.reset(eval_pool::ep.get_affine<ims_rational>(dim));

	auto* dst = a.m->gp<ims_rational>();

	let is_ok = mul_affine_rational_checked(
		dst,
		m0->gp<ims_rational>(),
		m1->gp<ims_rational>(),
		dim);

	if (!is_ok) {
		return false;
	}

	if (ori) {
		affine_set_translate_to_zero(dst, dim);
	}

	a.bits = number_of_bits(rational_vec_magnitude(dst, a.get_sz()));

	return true;
};

static bool mul_maps(
	inter_elem& a,
	const ims_val* m0,
	const ims_val* m1,
	size_t sum_bits,
	bool ori)
{
	switch (m0->gs())
	{
	case ims_val::EST::rational:
	{
		//for dot product:  dim*(ML*MR)^dim < M
		//for sum vectors:	2*(ML*MR) < M
		let dim = m0->extent(0);
		constexpr auto M = rational_get_max<ims_val_b::Rational>();
		let bw = (size_t)std::bit_width(M / std::max(dim, size_t(2)));
		if (dim * sum_bits > bw) {
			//let's try a slower path
			return mul_maps_rational_checked(a, m0, m1, ori);
		}
	
		return mul_maps_rational<ims_val_b::Rational>(a, m0, m1, ori);
	}
	case ims_val::EST::big_rational:
	{
		return mul_maps_rational<ims_val_b::BigRational>(a, m0, m1, ori);
	}
	case ims_val::EST::real:
	{
		return mul_maps_real(a, m0, m1, ori);
	}
	default:
		assert(false);
		return false;
	}
}


static bool is_id2(const inter_elem& a, double prec)
{
	switch (a.m->gs())
	{
	case ims_val::EST::rational:
	{
		return is_id_rational<ims_val_b::Rational>(a);
	}
	case ims_val::EST::big_rational:
	{
		return is_id_rational<ims_val_b::BigRational>(a);
	}
	case ims_val::EST::real:
	{
		return is_id_real(a, prec);
	}
	default:
		assert(false);
		return false;
	}
};


////////////////////////////////////////////////////////////////////////////////

bool integer_ims::map::set(const ims_val* m, size_t dim)
{
	p[0].m.reset();
	p[1].m.reset();

	pool_ptr ma(eval_helpers::to_affine3(m, dim));
	if (!ma)return false;

	pool_ptr mi(eval_helpers::affine_inv(ma.get()));
	if (!mi)return false;

	p[0].m = ma;
	p[1].m = mi;

	return true;
}

void integer_ims::map::e::set_bits()
{
	let sz = m->extent(0) * m->extent(1);
	let v = rational_vec_magnitude(m->p_i(), sz);
	bits = std::bit_width(v);
}


////////////////////////////////////////////////////////////////////////////////
bool integer_ims::create_for_ver(
	size_t s,
	inter_result& ir,
	neighbors_data& nb,
	const block_info& bi,
	const settings& st) 
{
	let& dig = bi.get_fg();

	let start_idx = nb.m_data.size();

	let sne = dig.num_edges(s);

	let csz = nb.m_childs.size();
	nb.m_childs.resize(csz + sne * sne);
	nb.m_root_inters[s] = { csz , sne };

	////////////////////////////////////////////////////////////////////////
	//initial intersections

	for (size_t i = 0; i < sne; ++i) {
		let& qi = dig.get_edge(s, i);

		let& mi = m_maps[qi.m];//expanding - left

		for (size_t j = 0; j < sne; ++j) {
			let& qj = dig.get_edge(s, j);


			let sij = nb.get_root_inter(s, i, j);
			nb.m_childs[sij] = ims_max;


			//skip self-intersection
			if (j == i) {
				continue;
			}

			let& mj = m_maps[qj.m];//contraction - right

			let hidx = nb.m_data.size();
			auto& he = nb.m_data.emplace_back();
	
			he.set_vers(qi.second, qj.second);

			if (!mul_maps(
				he,
				mi.p[1].m.get(),
				mj.p[0].m.get(),
				mi.p[1].bits + mj.p[0].bits,
				st.mode_ori))
			{
				ir.m_overflowed = true;
				return false;
			}

			ir.m_bits = std::max(ir.m_bits, he.bits);

			if (ir.m_bits > st.max_bits) {
				ir.m_overflowed = true;
				return false;
			}

			integer_ims::init_elem_real(he);
			let hres = nb.m_hash.emplace(hidx);
			nb.m_childs[sij] = *hres.first;

			if (hres.second) {
				he.next_edge = 0;
				he.depth = 1;
			} else {
				nb.m_data.pop_back();
			}
		};
	};

	////////////////////////////////////////////////////////////////////////
	
	size_t div_idx = start_idx;//will be divided

	while (div_idx < nb.m_data.size()) {
		if (ims_need_stop()) {
			return false;
		}

		nb.m_data.reserve(nb.m_data.size() + 1);//for stable references
		auto& e = nb.m_data[div_idx];

		let ediv = e.next_edge;

		//if the element is encountered for the first time, then we check the intersection
		if (e.res == inter_type::unknown) {

			if (is_id2(e, nb.m_prec)) {
				e.res = inter_type::overlapped;

				if (ir.m_over_depth > 0) {
					ir.m_over_depth = std::min(ir.m_over_depth, e.depth);
				} else {
					ir.m_over_depth = e.depth;
				}

				if (st.stop_on_overlap) {
					return false;
				}
			} else {

				e.res = intersect(bi, e, st.mode_ori);
			}

			if (e.res == inter_type::empty) {
				++div_idx;
				continue;
			};
		}


		let left = e.inter_type_left();
		let vdiv = left ? e.s0 : e.s1;
		let	ne = dig.num_edges(vdiv);

		if (ediv == ne) {
			++div_idx;
			continue;
		}

		if (ediv == 0) {//allocate space for references
			e.idx = nb.m_childs.size();
			nb.m_childs.resize(e.idx + ne);
		};

		///////////////////////////////////////////////////////
		let& qd = dig.get_edge(vdiv, ediv);

		bool res_mul;

		let hidx = nb.m_data.size();
		auto& he = nb.m_data.emplace_back();
	
		let& m = m_maps[qd.m];

		if (left) {
			he.set_vers(qd.second, e.s1);
			res_mul = mul_maps(
				he,
				m.p[1].m.get(),
				e.m.get(),
				m.p[1].bits + e.bits,
				st.mode_ori);
		} else {
			he.set_vers(e.s0, qd.second);
			res_mul = mul_maps(
				he,
				e.m.get(),
				m.p[0].m.get(),
				e.bits + m.p[0].bits,
				st.mode_ori);
		}
		if (!res_mul) {
			ir.m_overflowed = true;
			return false;
		}

		ir.m_bits = std::max(ir.m_bits, he.bits);

		if (he.bits > st.max_bits) {
			ir.m_overflowed = true;
			return false;
		}

		e.next_edge += 1;

		//next for processing
		integer_ims::init_elem_real(he);
		let hres = nb.m_hash.emplace(hidx);
		nb.m_childs[e.idx + ediv] = *hres.first;

		if (hres.second) {
			//new element, add to the processing list

			let ndp = e.depth + 1;

			ir.m_gcx = std::max(ir.m_gcx, nb.m_hash.size());
			ir.m_depth = std::max<size_t>(ir.m_depth, ndp);

			if (ir.m_gcx > st.max_inters) {
				return false;
			}

			if (ir.m_depth > st.max_depth) {
				return false;
			}

			he.next_edge = 0;
			he.depth = ndp;
		} else {
			nb.m_data.pop_back();
		};

	}//div_idx<nb.m_data.size()

	assert(nb.m_data.size() == nb.m_hash.size());


	////////////////////////////////////////////////////////////////////////
	//depth-first traversal to finally check intersections
	for (size_t i = start_idx; i < nb.m_data.size(); ++i) {
		auto& ev = nb.m_data[i];
		ev.next_edge = 0;//not processed yet
	}

	for (size_t i = start_idx; i < nb.m_data.size(); ++i) {
		let& q = nb.m_data[i];

		if (q.res == inter_type::empty || q.next_edge > 0) {
			continue;
		}

		//new candidate for checking
		assert(nb.m_idxs.empty());
		nb.m_idxs.push_back(i);
		while (!nb.m_idxs.empty()) {

			auto& ev = nb.m_data[nb.m_idxs.back()];

			let cur_depth = nb.m_idxs.size();

			let	ne = dig.num_edges(ev.div_ver());

			//search for the next non-empty child
			auto& ediv = ev.next_edge;
			while (ediv < ne) {

				let nid = nb.m_childs[ev.idx + ediv];
				++ediv;

				if (nid != ims_max &&
					nid >= start_idx &&
					nb.m_data[nid].res != inter_type::empty &&
					nb.m_data[nid].next_edge == 0)
				{

					nb.m_idxs.push_back(nid);
					break;
				}
			}

			if (cur_depth < nb.m_idxs.size()) {
				continue;//found a child, let's go deeper
			}
			assert(ediv == ne);

			//check all child elements, we can find the intersection
			//we'll also remove references to empty ones
			assert(ev.res != inter_type::empty);
			bool emp = true;
			for (size_t j = 0; j < ne; ++j) {
				auto& ri = nb.m_childs[ev.idx + j];
				if (ri != ims_max) {
					auto& ref = nb.m_data[ri];
					if (ref.res == inter_type::empty) {
						ri = ims_max;
					} else {
						emp = false;
					}
				}

			}
			if (emp) {//divided into empty
				ev.res = inter_type::empty;
			}

			nb.m_idxs.pop_back();

		}
	}

	return true;
}


inter_result integer_ims::calc_inter(
	neighbors_data& nb,
	const block_info& bi,
	const settings& st)
{
	assert(bi.exists());

	let& dig = bi.get_fg();
	let num_ver = dig.num_ver();
	let num_maps = bi.m_em.size();

	if (m_eval_id != bi.m_id8) {
		m_eval_id = bi.m_id8;
		ims_resize(m_maps, num_maps);//reset the maps
		m_taff.reset();
		m_tvec.reset();
	}


	nb.clear();
	inter_result ir;

	if (st.prec > 0) {
		ir.m_mode = intersect_mode::real;
		nb.m_prec = st.prec;
	} else {
		ir.m_mode = intersect_mode::rational;//for beginning...
		nb.m_prec = 1;
	}

	for (;;) {//1 or 2 times (rational+big rational)
		
		ims_resize(nb.m_root_inters, num_ver);//full reset for roots

		for (size_t comp_idx = 0; comp_idx < dig.m_comp.size(); ++comp_idx) {
			let& c = dig.m_comp[comp_idx];
			bool wrong_comp = false;
			//check that other components we depend on have been processed successfully
			for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
				let v = dig.m_ver_sorted[j + c.idx_sorted];
				let ne = dig.num_edges(v);
				for (size_t e = 0; e < ne; ++e) {
					let vt = dig.get_edge(v, e).second;
					if (dig.m_ver2com[vt] != comp_idx && nb.ver_invalid(vt)){
						wrong_comp = true;
						goto lab_next;
					}
				}
			}
		lab_next:;

			if (wrong_comp) {
				continue;//there's no point in checking
			}

			let start_data = nb.m_data.size();
			let start_childs = nb.m_data.size();

			auto revert_comp = [&]{//reject the component
				nb.revert(start_data, start_childs);
				for (size_t k = 0; k < c.num_ver; ++k) {
					let vk = dig.m_ver_sorted[k + c.idx_sorted];
					nb.m_root_inters[vk].set_invalid();
				}
			};

			ir.m_overflowed = false;

			let& ci = bi.m_comp_info[comp_idx];

			if (ir.m_mode == intersect_mode::real) {

				if (ci.dim_p == ims_max) {
					continue;
				}

				for (size_t i = 0; i < num_maps; ++i) {
					let& mi = bi.m_map_info[i];
					if (mi.status != block_info::map_info::used) {
						continue;
					}

					auto& t = m_maps[i];
					if (t.p[0].m && 
						t.p[0].m->gs() == ims_val_b::EST::real &&
						t.p[0].m->extent(0) == ci.dim_p)
					{
						continue;//already filled in correctly
					}

					if (!t.set(bi.m_em[i].mg.get(), ci.dim_p)) {
						continue;
					}
				}
				if (!m_tvec || m_tvec->get_size() != ci.dim_p) {
					m_tvec = eval_pool::ep.get_vector_real(ci.dim_p);
				}

				m_cur_alg_id = ims_max;

			} else if (ir.m_mode == intersect_mode::rational) {
				//calculate maps

				if (ci.alg_id == ims_max) {
					continue;
				}

				let dim_a = bi.get_dim_alg(ci.alg_id);

				for (size_t i = 0; i < num_maps; ++i) {
					let& mi = bi.m_map_info[i];
					if (mi.status != block_info::map_info::used) {
						continue;
					}

					auto& t = m_maps[i];
					if (t.p[0].m &&
						t.p[0].m->gs() == ims_val_b::EST::rational &&
						t.alg_id == ci.alg_id)
					{
						continue;//already filled in correctly
					}

					if (!t.set(bi.m_em[i].ma.get(), dim_a)) {
						continue;
					};
					t.p[0].set_bits();
					t.p[1].set_bits();
					t.alg_id = ci.alg_id;
				}

				if (!m_taff || m_taff->extent(0) != ci.dim_p) {
					m_taff = eval_pool::ep.get_affine_real(ci.dim_p);
					m_tvec = eval_pool::ep.get_vector_real(ci.dim_p);
				}

				m_cur_alg_id = ci.alg_id;
			} else {
				assert(ir.m_mode == intersect_mode::big_rational);

				//calculate maps
				if (ci.alg_id == ims_max) {
					continue;
				}

				let dim_a = bi.get_dim_alg(ci.alg_id);

				for (size_t i = 0; i < num_maps; ++i) {
					let& mi = bi.m_map_info[i];
					if (mi.status != block_info::map_info::used) {
						continue;
					}

					auto& t = m_maps[i];
					if (t.p[0].m &&
						t.p[0].m->gs() == ims_val_b::EST::big_rational &&
						t.alg_id == ci.alg_id)
					{
						continue;//already filled in correctly
					}


					if (!t.set(bi.m_em[i].ma.get(), dim_a)) {
						continue;
					};

					t.p[0].set_bits();
					t.p[1].set_bits();
					t.alg_id = ci.alg_id;

					let sz = dim_a * (dim_a + 1);
				
					pool_ptr m0(eval_pool::ep.get_affine_big_rational(dim_a));
					convert_to_big(m0->p_b(), t.p[0].m->p_i(), sz);
					t.p[0].m = m0;

					pool_ptr m1(eval_pool::ep.get_affine_big_rational(dim_a));
					convert_to_big(m1->p_b(), t.p[1].m->p_i(), sz);
					t.p[1].m = m1;
				}

				if (!m_taff || m_taff->extent(0) != ci.dim_p) {
					m_taff = eval_pool::ep.get_affine_real(ci.dim_p);
					m_tvec = eval_pool::ep.get_vector_real(ci.dim_p);
				}

				m_cur_alg_id = ci.alg_id;
			}

			////////////////////////////////////////////////////////////////////

			for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
				let v = dig.m_ver_sorted[j + c.idx_sorted];

				if (create_for_ver(v, ir, nb, bi, st)) {
					continue;
				}

				if (ir.m_overflowed &&
					ir.m_mode == intersect_mode::rational &&
					st.max_bits > std::numeric_limits<ims_val_b::Integer>::digits)
				{
					//try to recalculate everything with greater accuracy
					goto try_rational_big;
				}

				if (st.stop_on_incomplete) {
					return ir;
				}

				revert_comp();

				break;//move to the next component
			}
		}//by components

		nb.collapse_empty();
		ir.m_completed = true;
		return ir;

	try_rational_big:;//recreate everything and try again
		ir = inter_result();
		ir.m_mode = intersect_mode::big_rational;
		nb.clear();
	}
}



void integer_ims::init_elem_real(inter_elem& e)
{
	if (e.m->is(ims_val_b::EST::real)) {
		ims_val_b::Real adet;
		eval_helpers::norm_adet(e.m.get(), &adet);
		e.res2 = compare_size(adet);
	}
}

inter_type integer_ims::intersect(
	const block_info& bi, 
	const inter_elem& e, 
	bool ori)
{
	let* m = e.m.get();

	switch (m->gs())
	{
	case ims_val::EST::rational:
	{
		pool_ptr mr(eval_helpers::to_real3(m));
		bi.get_affine(m_taff->p_r(), mr->p_r(), m_cur_alg_id);
		break;
	}
	case ims_val::EST::big_rational:
	{
		pool_ptr mr(eval_helpers::to_real3(m));
		bi.get_affine(m_taff->p_r(), mr->p_r(), m_cur_alg_id);
		break;
	}
	case ims_val::EST::real:
	{
		m_taff = m; m->add_ref();
		break;
	}
	default:
		assert(false);
		return inter_type::unknown;
	}

	using Real = ims_val_b::Real;
	let eps = ims_num_traits<Real>::almost_zero();

	if (ori) {
		
		let det = matrix_helper::calc_det_ex(m_taff->p_r(), m_taff->extent(0));

		let dif = 1 - std::abs(det);

		if (std::abs(dif) < eps) {
			return det > 0 ? inter_type::both : inter_type::both_neg;
		}
		return dif > 0 ? inter_type::left : inter_type::right;
	} 


	let& b0 = bi.m_vb[e.s0];
	let& b1 = bi.m_vb[e.s1];

	Real adet;
	let sma = eval_helpers::norm_adet(m_taff.get(), &adet);

	if (b0.defined2() && b1.defined2()) {
		//TEST_ALLOC_HOOK(true);
		auto d = m_tvec->VecR();
		d.noalias() = m_taff->MatR() * b1.center();
		d = d + m_taff->TrR() - b0.center();
		let sumr = sma * b1.radius() + b0.radius();
		if (d.squaredNorm() > sumr * sumr + eps) {
			return inter_type::empty;
		}
	}

	//use the determinant
	return compare_size(adet);
}
