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
#include "graph_poly.h"
#include "block_info.h"
#include "poly_roots.h"
#include "eval_helpers.h"
#include "ims_val.h"
#include "eval_pool.h"

bool print_dimensions(std::ostream& sout, const block_info& bi)
{
	using Real = double;

	////////////////////////////////////////////////////////////////////////

	let& g = bi.get_fg();
	if (g.empty()) {
		return false;
	}

	let& di = bi.m_im.di;

	if (di.empty()) {
		return false;
	}

	////////////////////////////////////////////////////////////////////////

	//classify dimensions
	struct dver
	{
		size_t comp;
		size_t id;
	};
	std::vector<dver> dim_id(di.size());
	for (size_t i = 0; i < di.size(); ++i) {
		dim_id[i].comp = i;
	}

	//group the vertices in descending order of dimension
	std::sort(dim_id.begin(), dim_id.end(), [&di](let& e1, let& e2) {
		let& m1 = di[e1.comp];
		let& m2 = di[e2.comp];

		let eps = ims_num_traits<Real>::almost_zero();

		return m1.H > m2.H + eps;
	});

	let eps = ims_num_traits<Real>::almost_zero();


	size_t num_id = 0;
	dim_id[0].id = num_id;
	for (size_t i = 1; i < dim_id.size(); ++i) {
		const Real dh = di[dim_id[i - 1].comp].H - di[dim_id[i].comp].H;
		if (std::abs(dh) > eps) {
			++num_id;
		}
		dim_id[i].id = num_id;
	};
	++num_id;

	//return the order of components
	std::sort(dim_id.begin(), dim_id.end(), [](let& e1, let& e2) {
		return e1.comp < e2.comp;
	});

	//for each dimension, gives the component for which this
	//dimension is proper with finite measure
	std::vector<size_t> own_comp(num_id);
	for (size_t i = 0; i < g.m_comp.size(); ++i) {
		if (di[i].DR == dim_relations::own) {
			own_comp[dim_id[i].id] = i;
		}
	}
	
	////////////////////////////////////////////////////////////////////////////

	let num_maps = bi.m_em.size();

	//gives the index of the used map by the index of the standard map
	struct used_map
	{
		size_t map_idx;		//map index
		Real det_rootn;		//module of the determinant
		ims_rational p;
	};

	//map indices, here all maps have different determinant modulus
	//and the determinant is not equal to 1
	std::vector<used_map> um;

	//gives used_map by map index
	std::vector<size_t> remap(num_maps);

	std::vector<intptr_t> pows;
	BigPoly graph_poly;


	using MatrixRational = Eigen::Matrix<ims_rational_big, Eigen::Dynamic, Eigen::Dynamic>;
	using PolyType = ims_rational_big;
	std::vector<PolyType> base_poly;

	//to avoid printing extra base
	std::vector<Real> p_vals_uniq;

	auto get_p_val = [&p_vals_uniq, &eps](Real v, size_t& idx)
	{
		for (size_t i = 0; i < p_vals_uniq.size(); ++i) {
			if (std::abs(p_vals_uniq[i] - v) < eps) {
				idx = i;
				return false;//not inserted
			}
		}
		idx = p_vals_uniq.size();
		p_vals_uniq.emplace_back(v);
		return true;//inserted
	};



	for (size_t k = 0; k < own_comp.size(); ++k) {
		let ci = own_comp[k];

		let& dc = di[ci];
		assert(dc.DR == dim_relations::own);

		fmt::println(sout, "Component {}, dim ~= {}", ci, dc.H);

		//TODO: make it work for real IFS too...

		um.clear();

		let& c = g.m_comp[ci];

		let alg_id = bi.m_comp_info[ci].alg_id;

		if (alg_id == ims_max) {
			continue;
		}

		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
		
			let v = g.m_ver_sorted[j + c.idx_sorted];
			let ne = g.num_edges(v);
			for (size_t e = 0; e < ne; ++e) {
				let& qe = g.get_edge(v, e);

				if (g.m_ver2com[qe.second] != ci) {
					continue;//only for the component itself
				}

				let drn = bi.m_em[qe.m].det_rootn;
				assert(drn > eps);
				if (fabs(1 - drn) < eps) {
					remap[qe.m] = ims_max;
					continue;//useless
				}

				bool found = false;
				for (size_t i = 0; i < um.size(); ++i) {
					let& q = um[i];

					if (fabs(q.det_rootn - drn) < eps) {
						remap[qe.m] = i;
						found = true;
						break;
					}
				}

				if (found) {
					continue;//not unique
				}

				remap[qe.m] = um.size();

				auto& dst = um.emplace_back();
				dst.det_rootn = drn;
				dst.map_idx = qe.m;
			}
		}

		if (um.empty()) {
			continue;
		}


		//find rational approximations
		let div = log(um[0].det_rootn);
		um[0].p = 1;


		bool is_ok = true;
		for (size_t i = 1; i < um.size(); ++i) {
			auto& umi = um[i];
	
			let x = log(umi.det_rootn) / div;
			ims_integer p, q;
			is_ok = rational_approximation(x, p, q, 1e-10, (ims_integer)100);
			if (!is_ok) {
				break;
			}
			umi.p = { p,q };
		}
		if (!is_ok) {//could not find rational approximations
			continue;
		}

		

		//get rid of the denominators
		ims_integer pd = um[0].p.denominator();
		for (size_t i = 1; i < um.size(); ++i) {
			pd = boost::integer::lcm(pd, um[i].p.denominator());
		}

		for (size_t i = 0; i < um.size(); ++i) {
			um[i].p *= pd;
			assert(um[i].p.denominator() == 1);
		}

		//calculate the base matrix

		let n = bi.get_dim_alg(alg_id);
		
		MatrixRational R(n, n), Ri(n, n), RT(n, n);

	

		auto get_map = [&](MatrixRational& dst, size_t idx)
		{
			let* ma = bi.m_em[um[idx].map_idx].ma.get();

			pool_ptr ai(eval_helpers::to_affine3(ma, n));

			pool_ptr dv(eval_helpers::affine_inv(ai.get()));

			let* s = dv->p_i();
			let* se = s + n * n;
			auto* d = dst.data();
			while (s < se) {
				*d++ = { s->numerator(), s->denominator() };
				++s;
			}
		};

		get_map(R, 0);//the base matrix will be here
		auto cp = um[0].p.numerator();

		for (size_t i = 1; i < um.size(); ++i) {
			if (cp == 1)break;//result in R
			let& umi = um[i];
			
			//cp * res.x + umi.p * res.y = res.gcd
			auto res = boost::integer::extended_euclidean(cp, umi.p.numerator());
			
			if (res.x < 0) {
				R = R.inverse();
				res.x = -res.x;
			}
			RT.setIdentity();
			mulpow_mat(RT, R, (uint8_t)res.x, Ri);

			get_map(Ri, i);

			if (res.y < 0) {
				Ri = Ri.inverse();
				res.y = -res.y;
			}
			mulpow_mat(RT, Ri, (uint8_t)res.y, R);
			R = RT;

			cp = res.gcd;
		}

		assert(cp == 1);

		auto R_denom = denominator(R(0));
		for (size_t i = 1; i < n*n; ++i) {
			R_denom = boost::integer::lcm(R_denom, denominator(R(i)));
		}
		R *= R_denom;//now R is an integer

		base_poly.resize(n + 1);

		assert(R_denom > 0);
		char_poly(R, n, Ri, RT, base_poly.data());
		base_poly.back() = 1;

		size_t idx_p;


		let dproj = bi.get_dim_proj(alg_id);

		R_denom = ipow(R_denom, (uint8_t)dproj);

		fmt::println(sout, "dim = {}*log(x)/log(p)", dproj);

		if (n == dproj) {
			using Int = ims_get<PolyType>::int_type;
			let a0 = (Int)abs(numerator(base_poly.front()));
			Real p_val = (Real)a0 / (Real)R_denom;

			let is_new = get_p_val(p_val, idx_p);
			fmt::print(sout, "p = p{}", idx_p);
			if (is_new) {
				fmt::print(sout, " = {}", a0);
				if (R_denom != 1) {
					fmt::print(sout, "/{}", R_denom);
				}
			}
		} else {

			pool_ptr RR(eval_pool::ep.get_affine_real(bi.get_dim_alg(alg_id)));
			eval_helpers::affine_real_set_to(RR.get(), 0);
			RR->MatR() = R.cast<Real>();

			pool_ptr mB(eval_pool::ep.get_affine_real(dproj));
			bi.get_affine(mB->p_r(), RR->p_r(), alg_id);
			
			let& vals = mB->MatR().eigenvalues();

			Real p_val = 1;
			for (size_t i = 0; i < (size_t)vals.size(); ++i) {
				p_val *= std::abs(vals[i]);
			}

			p_val /= (Real)R_denom;

			let is_new = get_p_val(p_val, idx_p);
			fmt::print(sout, "p = p{}", idx_p);
			if (is_new) {
				fmt::print(sout, " = |product of used roots|");
				if (R_denom != 1) {
					fmt::print(sout, "/{}", R_denom);
				}
				fmt::print(sout,"\n");
				fmt::println(sout, "p{} ~= {}", idx_p, p_val);
				fmt::print(sout, "used roots of ");
				poly_func::print(base_poly.data(), base_poly.size(),
					false, sout, "z");
				fmt::print(sout, " :");

				for (size_t i = 0; i < (size_t)vals.size(); ++i) {
					fmt::print(sout, "\n");

					let& q = vals[i];

					auto qr = q.real();
					auto qi = q.imag();

					if (std::abs(qr) < eps)qr = 0;
					if (std::abs(qi) < eps)qi = 0;

					fmt::print(sout, "\t");
					if (qr > 0) {
						//to compensate '-' for the minus of the negative values
						fmt::print(sout, " ");
					};
					if (qr != 0) {
						fmt::print(sout, "{}", qr);
					};

					if (qi != 0) {
						if (qi > 0) {
							fmt::print(sout, "+");
						}
						fmt::print(sout, "{}*i", qi);
					};
				}
			}
		}
		fmt::print(sout, "\n");

		pows.resize(num_maps);

		for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
			let v = g.m_ver_sorted[j + c.idx_sorted];
			let ne = g.num_edges(v);
			for (size_t e = 0; e < ne; ++e) {
				let& qe = g.get_edge(v, e);

				if (g.m_ver2com[qe.second] != ci) {
					continue;//only for the component itself
				}

				let midx = remap[qe.m];
				if (midx == ims_max) {
					pows[qe.m] = 0;
				} else {
					pows[qe.m] = um[midx].p.numerator();
				}
			}
		}

		ims_worker::get()->work_reset();
		compute_graph_poly(graph_poly.data(), g, ci, pows);
		if (ims_need_stop())return false;

		if (graph_poly.is_zero())continue;

		let x = poly_roots::max_positive_root<Real>(graph_poly);

		poly_func::print(
			graph_poly.data().data(),
			graph_poly.size(),
			false,
			sout);

		fmt::println(sout, "=0");
		fmt::println(sout, "x~={}", x);
	}


	return true;
}
