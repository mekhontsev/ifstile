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
#include "projector.h"
#include "math_helpers.h"

void projector::get_affine(Real* dst, const Real* src) const
{
	using CMap = Eigen::Map<const DynMat<Real>>;
	using CVec = Eigen::Map<const DynVec<Real>>;

	using Map = Eigen::Map<DynMat<Real>>;
	using Vec = Eigen::Map<DynVec<Real>>;

	let da = dim_algebraic();
	let dp = dim_proj();

	//TODO: get rid of dynamic memory allocation

	Map(dst, dp, dp).noalias() =
		L * CMap(src, da, da) * R;

	Vec(dst + dp * dp, dp).noalias() = 
		L * CVec(src + da * da, da);
}

void projector::get_matrix(Matrix& dst, const Matrix& src) const
{
	dst = L * src * R;
}

bool projector::check(const Real* src, bool sim /*= false*/) const
{
	//TODO: get rid of dynamic memory allocation

	let da = dim_algebraic();
	using Map = Eigen::Map<const DynMat<Real>>;

	Map A(src, da, da);

	Matrix Z = L * A * (Matrix::Identity(da, da) - R * L);
	let& eps = ims_num_traits<Real>::almost_zero();
	for (int r = 0; r < Z.rows(); ++r) {
		for (int c = 0; c < Z.cols(); ++c) {
			if (std::abs(Z(r, c)) > eps) {
				return false;
			}
		}
	}

	if (sim) {
		get_matrix(Z, A);
		if (get_sim(Z.data(), Z.rows()) == 0) {
			return false;
		}
	}


	return true;
}

void projector::calc_L_ortho()
{
	const Matrix Rt = R.transpose();
	L = (Rt * R).inverse() * Rt;
}

void projector::setIdentity(size_t dim)
{
	L.resize(dim, dim);
	L.setIdentity();

	R.resize(dim, dim);
	R.setIdentity();
}

bool projector::empty() const
{
	return dim_proj() == 0;
}

void projector::clear()
{
	L.resize(0, 0);
	R.resize(0, 0);
}

size_t projector::dim_proj() const
{
	return R.cols();
}

size_t projector::dim_algebraic() const
{
	return R.rows();
}

