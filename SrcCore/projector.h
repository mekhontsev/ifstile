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
#include "dyn_mat_vec.h"

struct projector
{
	using Real = double;
	using Matrix = DynMat<Real>;

	//L*R=E, R*L=P - projector decomposition
	Matrix L, R;

	//get an affine map from a projective space
	void get_affine(Real* dst, const Real* src) const;

	//project only the matrix from the phase to the target
	void get_matrix(Matrix& dst, const Matrix& src) const;

	//check for matrix compatibility with the projector
	//sim - allow only similarities
	bool check(const Real* src, bool sim = false) const;

	//calculate the orthogonal projection based on the basis
	//L = [(Rt*R)^-1]Rt, R = Lt*[(L*Lt)^-1] - but this is only one solution
	void calc_L_ortho();


	void setIdentity(size_t dim);

	bool empty() const;


	void clear();

	//dimension of the set space (dim_proj<=dim_phas)
	size_t dim_proj() const;

	//dimension of rational space
	size_t dim_algebraic() const;
};

