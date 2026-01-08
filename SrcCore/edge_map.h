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
#include "pool_ptr.h"

struct edge_map
{
	using Real = double;

	//null_ptr for erroneous
	pool_ptr m;
	//algebraic part, rational affine, Mobius, scalar
	pool_ptr ma;
	//geometric part - projected affine, Mobius, scalar
	pool_ptr mg;

	//multiplicative for scalars and affines of mixed dimension
	Real det_rootn = 0;		//abs(det)^(1/dim)
	
	bool used = false;	//is it used in the end?

	bool is_sim = false;	//similarity
	bool neg_det = false;	//negative determinant
};
