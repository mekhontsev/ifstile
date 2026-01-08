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


struct matrix_helper: public boost::noncopyable
{
	using Real = double;
	using Matrix = DynMat<Real>;

	struct solvers : public boost::noncopyable
	{
		solvers(int dim) : 
			m_RealSchur(dim),
			m_PartialPivLU(dim),
			m_JacobiSVD(dim, dim),
			m_SelfAdjointEigenSolver(dim),
			m_EigenSolver(dim)
		{};
	
		Eigen::RealSchur<Matrix> m_RealSchur;
		Eigen::PartialPivLU<Matrix> m_PartialPivLU;
		Eigen::JacobiSVD<Matrix> m_JacobiSVD;
		Eigen::SelfAdjointEigenSolver<Matrix> m_SelfAdjointEigenSolver;
		Eigen::EigenSolver<Matrix> m_EigenSolver;
	};

	//calculates the maximum singular value and the modulus of the determinant
	static Real norm_adet(const Real* p, size_t dim, Real* adet = nullptr);
	static Real calc_det_ex(const Real* p, size_t dim);
	static bool calc_fixed_point(Real* dst, const Real* p, size_t n);
	static bool is_not_contracting(const Real* p, size_t n);

	//for special needs
	Eigen::FullPivLU<Matrix> m_FullPivLU_generic;

	static matrix_helper& get_matrix_helper();
	static solvers& get_solvers(size_t dim);

	solvers& get(size_t dim);
private:

	std::vector<std::unique_ptr<solvers>> m_solvers;

};
