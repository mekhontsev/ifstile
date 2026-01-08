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

//dimensions relationships
enum class dim_relations
{
	own, //the proper dimension is greater than the dependent dimension
	dep, //the dependent dimension is greater than the proper dimension
	equ, //the proper dimension is equal to the dependent dimension (infinite measure)
};


template<typename Real>
struct ifs_metrics 
{
	
	struct dimension_info
	{
		Real H;	//dimension
		dim_relations DR;
	};

	struct metrics
	{
		DynVec<Real> C;//mass center

		//Euler tensor
		DynVec<Real> I;//principal moments of inertia
		DynMat<Real> Q;//directions of the principal axes
		
		Real R2;//distance from the center of mass to the farthest point
		uint32_t NR;//number of radii
	};

	std::vector<Real> measure;//measure of each set
	std::vector<dimension_info> di;//relative dimension of each component
	std::vector<metrics> me;//moments of all sets
	std::vector<Real> mes_mul;//edge measures; for each vertex, the sum of the outgoing edges is 1
};
