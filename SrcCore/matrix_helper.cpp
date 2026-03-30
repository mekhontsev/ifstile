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
#include "matrix_helper.h"
#include "math_helpers.h"//mulpow


static thread_local matrix_helper s_matrix_helper;

matrix_helper& matrix_helper::get_matrix_helper()
{
	return s_matrix_helper;
}

matrix_helper::solvers& matrix_helper::get_solvers(size_t dim)
{
	return s_matrix_helper.get(dim);
}

using Real = matrix_helper::Real;

static Real get_adet_from_singulars(const Real* q, size_t dim)
{
	Real adet = q[0];
	if (adet == 0 || dim == 1)return adet;

	constexpr auto eps = ims_num_traits<Real>::almost_zero();

	let threshold = adet * eps;
	for (size_t i = 1; i < dim; ++i) {
		//TODO: understand
		if (q[i] < threshold)break;

		adet *= q[i];
	};

	return adet;
}


template<size_t dim>
static Real norm_adet_fixed(const Real* p, Real* adet)
{
	using Matrix = Eigen::Matrix<Real, dim, dim>;

	Eigen::Map<const Matrix> A(p);

	Eigen::JacobiSVD<Matrix> JacobiSVD;
	JacobiSVD.compute(A);
	let* q = JacobiSVD.singularValues().data();

	let ma = q[0];

	if (adet) {
		*adet = get_adet_from_singulars(q, dim);
	}
	
	return ma;
};


matrix_helper::solvers& matrix_helper::get(size_t dim)
{
	assert(dim > 0);
	let idx = dim - 1;

	if (idx >= m_solvers.size()) {
		m_solvers.resize(idx + 1);
	}

	auto& p = m_solvers[idx];

	if (!p) {
		p = std::make_unique<solvers>((int)dim);
	}
	return *p;
}

Real matrix_helper::norm_adet(const Real* p, size_t dim, Real* adet)
{
	Real ma;
#if 1 //cube search: 1.77->1.40
	let s = get_sim(p, dim);
	if (s > 0) {//including the case dim == 1
		ma = s;

		if (adet) {
			*adet = s;
			mulpow(*adet, s, (uint8_t)dim - 1);
		}
		return ma;
	}
#endif
	
	let& eps = ims_num_traits<Real>::almost_zero();

	if (dim == 2) {
		let b0 = p[0];
		let b1 = p[1];
		let b2 = p[2];
		let b3 = p[3];
		let det = b0 * b3 - b2 * b1;
		let f = b0 * b0 + b2 * b2 + b1 * b1 + b3 * b3;

		let f2 = f * f;
		let d2 = f2 - 4 * det * det;

		auto ad = std::abs(det);

		if (d2 <= f2 * eps) {
			ma = sqrt(f / 2);
		} else {
			Real d = sqrt(d2);
			ma = sqrt((f + d) / 2);
		
			if (ad < eps) {
				//TODO: understand
				ad = ma;
			}
		}

		if (adet) {
			*adet = ad;
		}
		
		return ma;
	}

#if 1 //cube search: 1.84->1.77
	if (dim == 3) {
		return norm_adet_fixed<3>(p, adet);
	}
#endif	

	auto& solver = get_solvers(dim).m_JacobiSVD;
	//TEST_ALLOC_HOOK(true);

	//general case
	Eigen::Map<const Matrix> A(p, dim, dim);
	solver.compute(A);
	let* q = solver.singularValues().data();
	
	ma = q[0];

	if (adet) {
		*adet = get_adet_from_singulars(q, dim);
	}
	
	return ma;
}

Real matrix_helper::calc_det_ex(const Real* p, size_t d)
{
	auto& solver = get_solvers(d).m_PartialPivLU;
	TEST_ALLOC_HOOK(true);

	Eigen::Map<const Matrix> A(p, d, d);
	return solver.compute(A).determinant();
}


bool matrix_helper::calc_fixed_point(Real* dst, const Real* p, size_t d)
{
	auto& solver = get_solvers(d).m_PartialPivLU;
	TEST_ALLOC_HOOK(true);

	using CMap = Eigen::Map<const Matrix>;
	solver.compute(DynMat<Real>::Identity(d, d) - CMap(p, d, d));

	if (solver.determinant() < ims_num_traits<Real>::almost_zero()) {
		return false;//the input matrix is very close to the identity
	}

	//TODO: Eigen 5 problem: unneeded allocation
	Eigen::Map<Matrix>(dst, d, 1).noalias() = solver.solve(CMap(p + d * d, d, 1));
	
	return true;
}


bool matrix_helper::is_not_contracting(const Real* p, size_t n)
{
	constexpr auto eps = ims_num_traits<Real>::almost_zero();
	constexpr auto max_s = 1 - eps;

	let sim_coeff = get_sim(p, n);
	if (sim_coeff > 0) {
		return sim_coeff * sim_coeff >= max_s;
	}

	Eigen::Map<const Matrix> A(p, n, n);

	auto& mh = get_matrix_helper();


	auto& solver = mh.get(n).m_RealSchur;


	TEST_ALLOC_HOOK(true);

	solver.compute(A, false);

	let& T = solver.matrixT();

	//run through all the blocks on the diagonal
	size_t idx = 0;//which part has already been processed
	while (idx < n) {

		let s00 = T(idx, idx);

		if (idx + 1 == n) {//last element 1x1
			return std::abs(s00) >= max_s;
		}

		let s10 = T(idx + 1, idx);
		let s01 = T(idx, idx + 1);
		
		if (std::abs(s10) < eps && std::abs(s01) < eps) {
			if (std::abs(s00) >= max_s) {
				return true;
			}
			idx += 1;
		} else {
			let s11 = T(idx + 1, idx + 1);
			let d = s00 * s11 - s10 * s01;//determinant
			if (std::abs(d) >= max_s) {
				return true;
			}
			idx += 2;
		}
	}
	return false;
}


