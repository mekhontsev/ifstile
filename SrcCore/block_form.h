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

struct block_form
{
	using Integer = int64_t;
	using Real = double;

	//check if the block is a companion matrix
	static bool is_companion_block(
		const DynMat<Integer>& M,
		const size_t start,
		const size_t size,
		Integer& d);


	//check if a block is a companion matrix
	static bool is_symmetric_block(
		const DynMat<Integer>& M,
		const size_t start,
		const size_t size);

	//get the projector and basis vectors of the subspace
	//assume the first matrix is block-diagonal
	//for each block:
	//if the block is a companion matrix
	//decompose into Jordan form using the confluent Vandermonde matrix
	//if the block is not a companion matrix
	//use the entire block with the standard basis
	//sort the subspaces:
	//by block, by eigenvalue modulo, by real part
	static bool get_proj(
		const DynMat<Integer>& M,
		std::span<const Real> cells,
		DynMat<Real>& L,
		DynMat<Real>& R);


	//get a matrix of the form
	//1  0  0  0
	//0 -1  0  0
	//0  0  1  0
	//0  0  0 -1

	static bool get_simple_reflection(DynMat<Integer>& dst, const size_t n);

	//check matrix for compatibility
	//TODO: remove, deprecated
	static bool check_additional_group(
		const DynMat<Integer>& src,
		std::span<const Integer> poly,
		std::span<const Real> cells);

};
