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
#include "affine_dim_calc.h"
#include "ims_graph.h"
#include "ims_num_traits.h"
#include "edge_map.h"
#include "eval_helpers.h"
#include "matrix_helper.h"
#include "ims_val.h"
#include "eval_pool.h"


affine_dim_calc::Real affine_dim_calc::calc_ext_dim(
	const ims_graph& gm, 
	const size_t comp_idx,
	std::span<const Real> idim)
{
	let& c = gm.m_comp[comp_idx];

	Real d = -1;

	for (size_t i = 0; i < c.num_ver; ++i) {//over all vertices of the component
		let v = gm.m_ver_sorted[i + c.idx_sorted];

		let ne = gm.num_edges(v);

		for (size_t j = 0; j < ne; ++j) {//all edges of the vertex
			let vt = gm.get_edge(v, j).second;

			//only for other components
			if (gm.m_ver2com[vt] != comp_idx) {
				d = std::max(d, idim[vt]);
			}
		}
	}

	return d;
}


affine_dim_calc::Real 
affine_dim_calc::compute_dim(
	std::vector<Real>& mes,
	const ims_graph& gm,
	const size_t comp_idx,
	ims_view<Real> sim)
{
	let& c = gm.m_comp[comp_idx];

	mes.resize(gm.num_ver());
	for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
		let v = gm.m_ver_sorted[j + c.idx_sorted];
		mes[v] = Real(1.0) / c.num_ver;
	}


	if (!c.has_self) {
		return -1;//empty set
	}

	if (c.own_countable) {
		return 0;
	}
#if 0	
	for (let d : det) {
		if (std::abs(d) < ims_num_traits<Real>::almost_zero()) {
			return -1;
		}
	}
#endif


	m_vers.resize(c.num_ver);
	size_t max_num_edges = 0;
	for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
		let v = gm.m_ver_sorted[j + c.idx_sorted];


		let ne = gm.num_edges(v);
		assert(gm.m_ver_in_comp[v] == j);

		auto& h = m_vers[j];
		h.ver_in_comp = j;
		h.edges.clear();
		for (size_t l = 0; l < ne; ++l) {//all edges of the vertex
			let& e = gm.get_edge(v, l);

			if (gm.m_ver2com[e.second] != comp_idx) {
				continue;//only for the current component
			};

			//ignore null determinants
			let dm = std::abs(sim[e.m]);
			if (dm < ims_num_traits<Real>::almost_zero()) {
				continue;
			}

			let vt = gm.m_ver_in_comp[e.second];

			entry en;
			en.ver = vt;
			en.k = dm;
			en.p = 0;

			h.edges.emplace_back(en);
		};

		if (h.edges.empty()) {
			return 0;
		}

		max_num_edges =
			std::max(max_num_edges, h.edges.size());
	};

	if (max_num_edges < 2) {
		return 0;
	}


	//remove references to vertices with no more than one edge
#if 0
	for (auto& q : m_vers) {
		if (q.edges.size() <= 1) {
			//we'll delete this edge anyway
		//	continue;
		}

		//leave, assign a new index to the vertex
		for (auto& e : q.edges) {
			//extend the chain
			let* vt = &m_vers[e.ver];
			while (vt->edges.size() == 1) {
				let& de = vt->edges.front();
				e.k *= de.k;
				e.ver = de.ver;
				vt = &m_vers[e.ver];
			}
		}
	}

	//renumbering the required ones
	for (auto& q : m_vers) {
		for (auto& e : q.edges) {
			e.ver = m_vers[e.ver].new_idx;
		}
	}

	//removing unnecessary
	ims_erase(m_vers, [](let& v) {return v.edges.size() <= 1; });

	if (m_vers.empty()) {
		return 0;
	}
#endif

	let max_k = 1 - ims_num_traits<Real>::almost_zero();
	//remove non-compressing edges
	for (auto& q : m_vers) {

		tmp_edges.clear();//expanding edges
		cnt_edges.clear();//compression edges

		for (let& e : q.edges) {
			if (e.k > max_k) {
				tmp_edges.push_back(e);
			} else {
				cnt_edges.push_back(e);
			}
		}

		//divide edges to be compressions
		while (!tmp_edges.empty()) {



			entry t = tmp_edges.back();//copy
			tmp_edges.pop_back();

			for (let& re : m_vers[t.ver].edges) {//TODO can be copied directly here
				auto e = re;//copy
				e.k *= t.k;

				if (e.k > max_k) {
					tmp_edges.push_back(e);
				} else {
					cnt_edges.push_back(e);
				}
			}

			//too many edges
			if (tmp_edges.size() + cnt_edges.size() > 1000 * q.edges.size()) {
				return -1;
			}
		}

		q.edges = cnt_edges;
	}

	////////////////////////////////////////////////////////////////////////

	//transition to logarithms and initial approximations of set measures
	for (auto& q : m_vers) {
		q.mes = Real(1) / m_vers.size();
		for (auto& e : q.edges) {
			e.k = -log(e.k);
		}
	};

	////////////////////////////////////////////////////////////////////////


#if 0

	Real x = 0.5;//initial approximation of the component dimension

	auto& rng = ims_random::getR().rng;
	std::uniform_real_distribution<Real> distr(Real(1) / 32768, 1);

	///////////////////////////////////////////////////
	const int n = m_vers.size();
	if (n > 2) {

		Eigen::VectorXcd evector;


		Eigen::SparseMatrix<double> M;



		auto get_y = [&](Real arg) {

			const Real lx = log(arg);

			for (auto& q : m_vers) {
				for (auto& e : q.edges) {
					e.p = exp(e.k * lx);
				}
			}


			M.resize(n, n);
			M.setZero();

			for (int i = 0; i < n; i++) {
				auto& q = m_vers[i];
				for (size_t j = 0; j < q.edges.size(); ++j) {
					auto& e = q.edges[j];
					M.coeffRef(i, e.ver) += e.p;
				}
			}
			// Construct matrix operation object using the wrapper class SparseGenMatProd
			Spectra::SparseGenMatProd<double> op(M);


			// Construct eigen solver object, requesting the largest three eigenvalues
			Spectra::GenEigsSolver<Spectra::SparseGenMatProd<double>> eigs(op, 1, 3);

			// Initialize and compute
			eigs.init();
			int nconv = eigs.compute(Spectra::SortRule::LargestReal);

			// Retrieve results

			if (eigs.info() != Spectra::CompInfo::Successful) {
				return std::numeric_limits<Real>::max();
			}

			evector = eigs.eigenvectors(1);
			auto v = eigs.eigenvalues()[0];
			v -= 1;

			return std::norm(v);
			};

		x = 0.5;
		Real xx = 0.6;//the penultimate value of x(n-2)

		Real y = get_y(x);
		Real yy = get_y(xx);

		Real err_dim = 1;

		for (;;) {
			if (y == yy)break;
			Real nx = x - y * (x - xx) / (y - yy);

			if (nx <= 0) {
				x = distr(rng);
				xx = distr(rng);

				y = get_y(x);
				yy = get_y(xx);

				err_dim = 1;
				continue;
			}


			xx = x;
			yy = y;
			x = nx;
			y = get_y(x);



			const Real e = std::abs(y);
			bool ready = e >= err_dim && e < ims_num_traits<Real>::almost_zero();
			err_dim = e;

			if (ready) {

				double sum = 0;
				for (int i = 0; i < n; i++) {
					sum += evector(i).real();
				}
				evector /= sum;
				for (int i = 0; i < n; i++) {
					if (evector(i).real() < 0) {
						evector(i) = 0;
					}
				}

				for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
					let v = gm.m_ver_sorted[j + c.idx_sorted];
					mes[v] = evector(j).real();
				}

				return (x >= 1) ? 0 : -log(x);
			}
		}

	}

#endif

#ifdef DEVELOPER_VERSION
	static size_t total_num_iter = 0;
#endif

	auto get_y = [this](Real x)->Real {

		assert(x > 0 && x < 1);
		
		const Real lx = log(x);

		for (auto& q : m_vers) {
			for (auto& e : q.edges) {
				e.p = exp(e.k * lx);
			}
		}

		Real err_mes = 1;

		while(!ims_need_stop()) {

#ifdef DEVELOPER_VERSION
			++total_num_iter;
#endif
		
			Real sum = 0;
			for (auto& q : m_vers) {
				q.mes_new = 0;
				for (auto& e : q.edges) {
					q.mes_new += e.p * m_vers[e.ver].mes;
				}
				sum += q.mes_new;
			}

			//make the sum of all component measures equal to 1
			for (auto& q : m_vers) {
				q.mes_new /= sum;
			}

			Real err = 0;
			for (auto& q : m_vers) {
				//use the sum, the maximum may not decrease monotonically, which sometimes gives a large error
				err += std::abs(q.mes - q.mes_new);

				//q.mes = q.mes_new;
				q.mes = (q.mes_new + q.mes) / 2;
			}
			if (err >= err_mes && err < ims_num_traits<Real>::almost_zero()) {
				return sum - 1;
			}
			err_mes = err;
		}

		return 0;
	};

	
	boost::math::tools::eps_tolerance<Real> tol;
	constexpr uintmax_t MAX_ITER = 100;
	auto num_iter = MAX_ITER;
	constexpr Real
		a = 0, b = 1,
		fa = -1, fb = 1;
#ifdef DEVELOPER_VERSION
	total_num_iter = 0;
#endif

	auto r = boost::math::tools::toms748_solve(
		get_y, 
		a, b, 
		fa, fb, 
		tol, 
		num_iter);
	if (num_iter >= MAX_ITER)return 0;
	const Real x = (r.first + r.second) / 2;

	
	for (size_t j = 0; j < c.num_ver; ++j) {//over all vertices of the component
		let v = gm.m_ver_sorted[j + c.idx_sorted];
		mes[v] = m_vers[j].mes_new;
	}

	return (x >= 1) ? 0 : -log(x);
}

void affine_dim_calc::compute_all_dims(
	ifs_metrics<Real>& im, 
	const ims_graph& gm, 
	ims_view<Real> sim)
{
	auto& ret_dim = im.di;
	ret_dim.resize(gm.m_comp.size());

	im.mes_mul.resize(gm.m_edges.size());
	std::fill(im.mes_mul.begin(), im.mes_mul.end(), 0);

	auto& ret = im.me;
	let nv = gm.num_ver();
	//heavy structure, try not to destroy it
	if (ret.size() < nv) {
		ret.resize(nv);
	}

	//for all components
	for (size_t comp_idx = 0; comp_idx < gm.m_comp.size(); ++comp_idx) {
		auto& comp = gm.m_comp[comp_idx];

		dim_relations dr;

		//own dimension
		Real hown = compute_dim(im.measure, gm, comp_idx, sim);
		if (ims_need_stop()) return;


		Real hdep = -1;

		//find the dependent dimension
		for (size_t k = 0; k < comp.num_ver; ++k) {//over all vertices of the component
			let v = gm.m_ver_sorted[k + comp.idx_sorted];

			let ne = gm.num_edges(v);

			for (size_t j = 0; j < ne; ++j) {//all edges of the vertex
				let vt = gm.get_edge(v, j).second;

				let vtc = gm.m_ver2com[vt];

				//only for other components
				if (vtc == comp_idx)continue;

				hdep = std::max(hdep, ret_dim[vtc].H);
			}
		}

		let eps = ims_num_traits<Real>::almost_zero();


		if (std::abs(hdep - hown) < eps) {
			dr = dim_relations::equ;
		} else if (hdep < hown) {
			dr = dim_relations::own;
		} else {
			dr = dim_relations::dep;
		}

		//real dimension
		let hdim = std::max(hown, hdep);
		auto& ds = ret_dim[comp_idx];
		ds.H = hdim;
		ds.DR = dr;
	}

}


static bool adjust(pool_ptr& p, size_t dim) 
{
	if (p && p->rows() == dim + 1) return false;
	p = eval_pool::ep.get_matrix_real(dim + 1, dim + 1);
	return true;
}

bool affine_dim_calc::compute_moments(
	ifs_metrics<Real>& im,
	std::span<const edge_map> ri,
	const ims_graph& gm,
	std::span<const size_t> vdim)
{
	bool is_ok = true;

	auto& ret_dim = im.di;


	let nm = ri.size();

	let nv = gm.num_ver();


	m_det_pow.resize(nm);

	////////////////////////////////////////////////////////////////////////
	
	ims_resize(m_mat, nm);
	
	
	m_exm.resize(nv);
	

	////////////////////////////////////////////////////////////////////////
	

	//for all components
	for (size_t comp_idx = 0; comp_idx < gm.m_comp.size(); ++comp_idx) {
		auto& comp = gm.m_comp[comp_idx];

		//Euclidean dimension of the component
		let dim = vdim[gm.m_ver_sorted[comp.idx_sorted]];
		if (dim==0)continue;

		adjust(m_sum, dim);
		auto sum = m_sum->MR();
		
		let& ds = ret_dim[comp_idx];


		for (size_t k = 0; k < comp.num_ver; ++k) {
			let v = gm.m_ver_sorted[k + comp.idx_sorted];

			//initial approximations
			auto& q = m_exm[v];
			adjust(q, dim);
			auto qM = q->MR();
			qM.setZero();
			if (ds.DR != dim_relations::dep) {
				assert(k == gm.m_ver_in_comp[v]);
				qM(dim, dim) = im.measure[v];
			}
		}
		////////////////////////////////////////////////////////////////////

		//move on to the measures' factors
		for (size_t j = 0; j < nm; ++j) {
			m_det_pow[j] = std::pow(ri[j].det_rootn, ds.H);
		}

		//start the approximation cycle
		err_checker errc;
		errc.init();

		let eps = ims_num_traits<Real>::almost_zero();

		bool err_mom = false;

		for (;;) {

			if (ims_need_stop()) {
				return false;
			}

			Real err = 0;

			for (size_t j = 0; j < comp.num_ver; ++j) {
				let v = gm.m_ver_sorted[j + comp.idx_sorted];
				assert(j == gm.m_ver_in_comp[v]);

				let ne = gm.num_edges(v);
				
				sum.setZero();

				for (size_t k = 0; k < ne; ++k) {//all edges of the vertex
					let& e = gm.get_edge(v, k);

					if (ri[e.m].det_rootn < eps)continue;

					let vtc = gm.m_ver2com[e.second];

					if (vdim[v] != vdim[e.second]) {
						continue;
					}


					if (vtc != comp_idx) {
						//the components on which they depend must have a larger dimension
						if (ds.DR == dim_relations::equ || ret_dim[vtc].H + eps < ds.H) {
							continue;
						}
					}

					auto& m = m_mat[e.m];
					if (adjust(m, dim)) {
						m = eval_helpers::to_ext_real(ri[e.m].mg.get(), dim);
						assert(m);
					}

					auto mat = m->MR();

					sum +=
						mat *
						m_exm[e.second]->MR() *
						mat.transpose() *
						m_det_pow[e.m];
				};

				//adjust at every step so that it doesn't diverge
				if (ds.DR != dim_relations::dep) {
					sum *= im.measure[v] / sum(dim, dim);
				}


				err += (sum - m_exm[v]->MR()).norm();

				if (!std::isfinite(err)) {
					err_mom = true;
					break;
				}

				m_exm[v]->MR() = sum;
			}

			if (err_mom) {
				break;
			}

			//the total number of iterations is usually around 100
			if (errc.check(err)) {
				break;
			}
		}

		if (err_mom) {
			is_ok = false;
		}

		for (size_t j = 0; j < comp.num_ver; ++j) {
			let v = gm.m_ver_sorted[j + comp.idx_sorted];

			auto& q = im.me[v];
			q.C.resize(dim);
			q.I.resize(dim);
			q.Q.resize(dim, dim);

			if (err_mom) {
				im.measure[v] = 0;
				q.C.setZero();
				q.I.setZero();
				q.Q.setIdentity();
				m_exm[v]->MR().setZero();
			} else {
				let m = m_exm[v]->MR();
				let W = m(dim, dim);
				im.measure[v] = W;

				for (size_t r = 0; r < dim; ++r) {
					q.C(r) = m(r, dim) / W;
				}

				//temporarily use Q
				for (size_t r = 0; r < dim; ++r) {
					for (size_t c = 0; c < dim; ++c) {
						q.Q(r, c) = m(r, c) -  m(r, dim) * m(c, dim) / W;
					}
				}

				auto& saes = matrix_helper::get_solvers(dim).m_SelfAdjointEigenSolver;
				saes.compute(q.Q);
				q.I = saes.eigenvalues();

				for (size_t r = 0; r < dim; ++r) {
					if (q.I(r) < 0)q.I(r) = 0;
				}
				
				q.Q = saes.eigenvectors();
			}

			q.R2 = -1;
			q.NR = 0;

			////////////////////////////////////////////////////////////////////

			//set the edge weights
			let ne = gm.num_edges(v);
			Real sum_val = 0;


			for (size_t k = 0; k < ne; ++k) {
				let eidx = gm.get_edge_idx(v, k);
				let& e = gm.m_edges[eidx];

				let val = im.measure[e.second] / im.measure[v] * m_det_pow[e.m];

				let vtc = gm.m_ver2com[e.second];

				if (vtc == comp_idx ||
					(ds.DR != dim_relations::equ && ds.H < ret_dim[vtc].H + eps))
				{
					im.mes_mul[eidx] = val;
					sum_val += val;
				} else {
					im.mes_mul[eidx] = 0;
				}
			}

			int num_zero = 0;
			for (size_t k = 0; k < ne; ++k) {
				let eidx = gm.get_edge_idx(v, k);
				if (im.mes_mul[eidx] == 0) {
					++num_zero;
				}
			}

			constexpr int avg_img_res = 100;
			let smx = num_zero > 0 ? (sum_val / num_zero / avg_img_res) : 1;
			for (size_t k = 0; k < ne; ++k) {
				let eidx = gm.get_edge_idx(v, k);
				if (im.mes_mul[eidx] == 0) {
					im.mes_mul[eidx] = smx;
				} else {
					im.mes_mul[eidx] /= sum_val;
				}
			}
		}
	}//by components

	return is_ok;
}

void affine_dim_calc::err_checker::init()
{
	min_err = std::numeric_limits<Real>::max();
	min_err_iters = 0;
}

bool affine_dim_calc::err_checker::check(Real v)
{
	if (v > ims_num_traits<Real>::almost_zero()) {
		return false;
	}

	if (v < min_err) {

		if (v == 0) {
			return true;
		}

		min_err = v;
		min_err_iters = 0;

		return false;//found something better, reset iterations
	}

	if (++min_err_iters <= 5) {
		return false;
	}

	return true;//ready
}
