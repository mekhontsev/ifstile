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
#include "block_form.h"
#include "poly_funcs.h"
#include "poly_roots.h"
#include "matrix_helper.h"
#include "projector.h"
#include "matrix_funcs.h"

//get the block sizes of a block-diagonal matrix
template<typename Matrix, typename Number>
void get_block_sizes(const Matrix& M, std::vector<size_t>& sizes, Number prec)
{
	let n = (size_t)M.rows();
	assert((size_t)M.cols() == n);

	sizes.clear();
	size_t k = 0;//done before
	while (k < n) {
		for (size_t s = 1; s <= n - k; ++s) {//possible block sizes
			//check if it's a block of size s.
			for (size_t r = k; r < k + s; ++r) {
				for (size_t c = k + s; c < n; ++c) {
					if (ims_abs(M(r, c)) > prec || ims_abs(M(c, r)) > prec) {
						goto lab;
					}
				}
			}
			sizes.push_back(s);
			k += s;
			break;
		lab:;
		}
	}
};


bool block_form::is_companion_block(const DynMat<Integer>& M, const size_t start, const size_t size, Integer& d)
{
	d = size <= 1 ? (Integer)1 : M(1 + start, start);
	for (size_t r = 0; r < size; ++r) {
		for (size_t c = 0; c + 1 < size; ++c) {//the last column is not important
			let v = (c + 1 == r) ? d : 0;
			if (M(r + start, c + start) != v)return false;
		}
	}
	return true;
}

bool block_form::is_symmetric_block(const DynMat<Integer>& M, const size_t start, const size_t size)
{
	for (size_t r = 0; r < size; ++r) {
		for (size_t c = 0; c < r; ++c) {//the last column is not important
			if (M(r + start, c + start) != M(c + start, r + start)) {
				return false;
			}
		}
	}
	return true;
}

bool block_form::get_proj(
	const DynMat<Integer>& M,
	std::span<const Real> cells,
	DynMat<Real>& L, 
	DynMat<Real>& R)
{
	using MatrixReal = DynMat<Real>;
	using Complex = std::complex<Real>;

	let dim = M.rows();

	if (cells.empty()) {//take the entire matrix as a whole
		L.resize(dim, dim);	L.setIdentity();
		R.resize(dim, dim);	R.setIdentity();
		return true;
	}

	//block sizes in block-diagonal
	std::vector<size_t> block_sizes;
	get_block_sizes(M, block_sizes, (Integer)0);

	struct block
	{
		size_t start;
		size_t size;
		poly_roots::root_list<Real>  relem;
	};

	std::vector<block> blocks(block_sizes.size());
	for (size_t i = 0; i < block_sizes.size(); ++i) {
		auto& q = blocks[i];
		q.size = block_sizes[i];
		if (i == 0) {
			q.start = 0;
		} else {
			let& qp = blocks[i - 1];
			q.start = qp.start + qp.size;
		}
	}


	//J=V*C*Vi, J*V=V*C
	MatrixReal V(dim, dim);
	V.setZero();



	struct cell_pos
	{
		size_t start;
		size_t size;
	};
	std::vector<cell_pos> cell2pos;//by cell number gives the position	



	ims_polynomial<Integer> poly;
	poly_roots::root_finder<Integer, Real> rf;



	MatrixReal T;
	std::vector<size_t> tarr;


	
	std::vector<size_t> subblocks;//sizes


	for (let& b : blocks) {
		let n = b.size;
		let bs = b.start;
		Integer denom;
		if (!is_companion_block(M, b.start, n, denom))
			//if (true)
		{
			T.resize(n, n);
			for (size_t r = 0; r < n; ++r) {
				for (size_t c = 0; c < n; ++c) {
					T(r, c) = static_cast<Real>(M(r + b.start, c + b.start));
				}
			}
			if (is_symmetric_block(M, b.start, n)) {
				//symmetric matrix
				auto& adj_solver = matrix_helper::get_solvers(n).m_SelfAdjointEigenSolver;
				adj_solver.compute(T, Eigen::DecompositionOptions::ComputeEigenvectors);
				tarr.resize(n);
				let& vals = adj_solver.eigenvalues();
				for (size_t i = 0; i < n; ++i) {
					tarr[i] = i;
				}
				//in descending of module
				std::sort(tarr.begin(), tarr.end(), [&vals](let& i1, let& i2) {
					let& v1 = vals[i1];
					let& v2 = vals[i2];
					let av1 = ims_abs(v1);
					let av2 = ims_abs(v2);
					let& eps = ims_num_traits<Real>::almost_zero();
					if (av1 - eps > av2)return true;//av1 is definitely larger than av2
					if (av1 + eps < av2)return false;//av1 is definitely less than av2
					return v1 > v2;//positive first
					});

				let& vecs = adj_solver.eigenvectors();
				let& eps = ims_num_traits<Real>::almost_zero();
				for (size_t i = 0; i < n; ++i) {
					let& vec = vecs.col(tarr[i]);
					let& val = vals(tarr[i]);

					//make the sign of the first non-zero component of the eigenvector
					//equal to the sign of the eigenvalue
					bool neg = (val + eps < 0);
					for (size_t c = 0; c < n; ++c) {
						let& q = vec(c);
						if (q > eps)break;//first positive
						if (q + eps < 0) { neg = !neg; break; };
					}
					for (size_t c = 0; c < n; ++c) {
						let& q = vec(c);
						V(bs + i, bs + c) = neg ? -q : q;
					}
					cell2pos.push_back({ bs + i,1 });
				}



				continue;
			} else {

				auto& eig_solver = matrix_helper::get_solvers(n).m_EigenSolver;
				
				eig_solver.compute(T, true);

				let& pv = eig_solver.pseudoEigenvalueMatrix();
				MatrixReal H = eig_solver.pseudoEigenvectors();


				get_block_sizes(pv, subblocks, ims_num_traits<Real>::almost_zero());
				let m = subblocks.size();

				auto& eig_vals = rf.m_relem;
				eig_vals.resize(m);

				let& eps = ims_num_traits<Real>::almost_zero();

				for (size_t i = 0, c = 0; i < m; ++i) {

					let sz = subblocks[i];

					//normalize the eigenvectors
					if (sz == 1) {

						let& val = pv(c, c);

						eig_vals[i].set(val, 0);

						//make the sign of the first non-zero component of the eigenvector 
						//equal to the sign of the eigenvalue
						bool neg = (val + eps < 0);
						for (size_t r = 0; r < n; ++r) {
							let& q = H(r, c);
							if (q > eps)break;//first positive
							if (q + eps < 0) { neg = !neg; break; };
						}

						Real nr = 0;//norm
						for (size_t r = 0; r < n; ++r) {
							let& q = H(r, c);
							nr += q * q;
						}
						nr = Real(1) / sqrt(nr);
						if (neg)nr = -nr;

						for (size_t r = 0; r < n; ++r) {
							H(r, c) *= nr;
						}
					} else {

						//TODO: better understand the sign of the imaginary part
						eig_vals[i].set(pv(c, c), ims_abs(pv(c, c + 1)));

						Real nr = 0;//norm
						for (size_t r = 0; r < n; ++r) {
							let& vx = H(r, c);
							let& vy = H(r, c + 1);
							nr += vx * vx + vy * vy;
						}
						nr = Real(1) / sqrt(nr);

						for (size_t r = 0; r < n; ++r) {
							H(r, c) *= nr;
							H(r, c + 1) *= nr;
						}
					}

					eig_vals[i].sq = c;
					eig_vals[i].pw = 1;

					c += sz;
				}


				poly_roots::root_elem<Real>::sort(eig_vals);
				if (eig_vals.back().ar < eps) {
					return false;
				}

				size_t u = bs;//target line


				//std::cout << "T=" << std::endl << T << std::endl;
				//std::cout << "pv=" << std::endl << pv << std::endl;
				//std::cout << "pe=" << std::endl << pe << std::endl;
				//std::cout << "pei=" << std::endl << (pe.inverse()) << std::endl;
				//std::cout << "xres=" << std::endl << (pe.inverse()*T*pe) << std::endl;

				MatrixReal X = H.inverse();

				for (let& q : rf.m_relem) {
					let& z = q.r;

					let vu = u;

					let r = q.sq;//column position in the matrix

					if (z.imag() == 0) {
						for (size_t c = 0; c < n; ++c) {
							V(u, bs + c) = X(r, c);
						}

						u += 1;
					} else {
						for (size_t c = 0; c < n; ++c) {
							V(u, bs + c) = X(r, c);
							V(u + 1, bs + c) = X(r + 1, c);
						}

						u += 2;
					}

					cell2pos.push_back({ vu,u - vu });

				}

				//std::cout << "V=" << std::endl << V << std::endl;

			}

		} else {

			//companion matrix

			//form a polynomial
			poly.data().resize(n + 1);
			for (size_t i = 0; i < n; ++i) {
				poly[i] = -M(b.start + i, b.start + n - 1);
			}
			poly[n] = denom;
			if (poly[0] == 0) {
				return false;
			}

			//factorization
			rf.find(poly);

			////////////////////////////////////////////////////////////////////////

			size_t u = bs;//target line
			//form a block from a confluent Vandermonde matrix
			for (auto& q : rf.m_relem) {
				auto& z = q.r;

				let vu = u;

				Real mul = 1;
				
#if 0
				{
					//find the sum 1+|z|+|z|^2+....|z|^(n-1)
					Real nrm = 1;

					let zn = std::norm(z);
					for (size_t c = 1; c < n; ++c) {
						nrm = nrm * zn + 1;
					}
					mul = 1.0 / sqrt(nrm);
					if (z.imag() == 0 && z.real() < 0) {
						mul *= -1;
					}
				}
#endif			

				if (z.imag() == 0) {

					//if (z.real() < 0)mul = 0.25426901617246173066425024106786493966;

					Real m = mul;

					

					V(u, bs) = m;
					for (size_t c = 1; c < n; ++c) {
						m *= z.real();
						V(u, bs + c) = m;
					}
					u += 1;

					for (size_t r = 1; r < q.pw; ++r) {
						V(u, u) = 0;
						for (size_t c = 1; c < n; ++c) {
							V(u, bs + c) = c * V(u - 1, bs + c - 1);
						}
						u += 1;
					}

				} else {

					Complex m(mul, 0);

					V(u, bs) = mul;
					V(u + 1, bs) = 0;

					for (size_t c = 1; c < n; ++c) {
						m *= z;
						V(u, bs + c) = m.real();
						V(u + 1, bs + c) = m.imag();
					}
					u += 2;

					for (size_t r = 1; r < q.pw; ++r) {

						V(u, bs) = 0;
						V(u + 1, bs) = 0;

						for (size_t c = 1; c < n; ++c) {
							V(u, bs + c) = V(u - 2, bs + c - 1) * c;
							V(u + 1, bs + c) = V(u - 1, bs + c - 1) * c;
						}
						u += 2;
					}
				}

				cell2pos.push_back({ vu,u - vu });

			}

		}
	}
#if 0
	//icosahedral group recognition
	if (dim == 6) {
		static const std::array<char, 6 * 6> MC =
		{
			0, 1, 0, 0, 0, 0,
			1, 1, 0, 0, 0, 0,
			0, 0, 0, 1, 0, 0,
			0, 0, 1, 1, 0, 0,
			0, 0, 0, 0, 0, 1,
			0, 0, 0, 0, 1, 1,
		};
		bool ico = true;
		for (size_t i = 0; i < dim * dim; ++i) {
			if (M.m_data[i] != MC[i]) {
				ico = false;
				break;
			}
		}
		if (ico) {
			let t = (Real(1) + sqrt(Real(5))) / Real(2);
			let t2 = t * t;
			let ti = 1 / t;

			V <<
				t, t2, -ti, -1, t, t2,
				-1, ti, t2, -t, -1, ti,
				0, 0, t, t2, 1, t,
				0, 0, -1, ti, t, -1,
				1, t, -1, -t, t2, t2* t,
				t, -1, -t, 1, ti, -ti * ti;

		}
	}
#endif

#if 0
	{

		/*
		let t = sqrt(5);

		V <<
			t, 1, 1, -1, 1, 1,
			1, t, 1, -1, -1, -1,
			1, 1, t, 1, 1, -1,
			-1, -1, 1, t, 1, -1,
			1, -1, 1, 1, t, 1,
			1, -1, -1, -1, 1, t;

			*/



		let t = (sqrt(5) + 1) / 2;

		V <<
			t, t, 1, -1, 0, 0,
			0, 0, t, t, 1, -1,
			1, -1, 0, 0, t, t,
			-1, -1, t, -t, 0, 0,
			0, 0, -1, -1, t, -t,
			t, -t, 0, 0, -1, -1;

		Vi = V.inverse();

		MatrixReal AR(6, 6);
		for (size_t r = 0; r < 6; ++r) {
			for (size_t c = 0; c < 6; ++c) {
				AR(r, c) = M(r, c);
			}
		}
		MatrixReal tst = V * AR * Vi;

		cell2pos.resize(1);
		cell2pos[0] = { 0,6 };


	}

#endif


#if 0
	if (dim == 6) {

		let t = sqrt(3);

		V <<
			1, 0, 0, t, 0, 0,
			0, 1, 0, 0, t, 0,
			0, 0, 1, 0, 0, t,
			-1, 0, 0, t, 0, 0,
			0, -1, 0, 0, t, 0,
			0, 0, -1, 0, 0, t;


		Vi = V.inverse();


		cell2pos.resize(6);
		for (size_t i = 0; i < 6; ++i) {
			cell2pos[i] = { i,1 };
		}
	}

#endif


	size_t dp = 0;//dimension of the projection space

	for (let& s : cells) {
		size_t idx = (size_t)floor(s);
		idx %= cell2pos.size();
		dp += cell2pos[idx].size;
	}

	L.resize(dp, dim);
	R.resize(dim, dp);

	MatrixReal Vi = V.inverse();

	size_t dst = 0;
	for (let& s : cells) {
		size_t idx = (size_t)floor(s);
		idx %= cell2pos.size();
		let src = cell2pos[idx].start;
		let sz = cell2pos[idx].size;
		L.block(dst, 0, sz, dim) = V.block(src, 0, sz, dim);
		R.block(0, dst, dim, sz) = Vi.block(0, src, dim, sz);

		dst += sz;
	};




	//std::cout << "res2=" << P * T*B << std::endl;

	return true;
}

bool block_form::get_simple_reflection(DynMat<Integer>& dst, const size_t n)
{
	if (n % 2 != 0)return false;

	dst.resize(n, n);
	dst.setZero();
	for (size_t i = 0; i < n; i += 2) {
		dst(i, i) = 1;
		dst(i + 1, i + 1) = -1;
	}
	return true;
}

bool block_form::check_additional_group(
	const DynMat<Integer>& src, 
	std::span<const Integer> poly,
	std::span<const Real> cells)
{
	if (poly.size() < 2) {
		return false;
	}
	let n = poly.size() - 1;


	//base matrix
	DynMat<Integer> M;
	get_companion(M, poly);

	projector proj;

	if (!get_proj(M, cells, proj.L, proj.R)) {
		return false;
	}

	DynMat<Real> X(n, n);
	for (size_t i = 0; i < n; ++i) {
		for (size_t j = 0; j < n; ++j) {
			X(i, j) = static_cast<Real>(src(i, j));
		}
	}

	return proj.check(X.data(), true);
}
