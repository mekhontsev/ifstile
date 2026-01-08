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


struct ims_graph;
struct edge_map;
struct edge_ball;

struct affine_builder
{
	using Real = double;

	using EVec = Eigen::Map<DynVec<Real>>;
	using EMat = Eigen::Map<DynMat<Real>>;

	//subspace - several points
	struct subspace 
	{
		size_t num;//number of points forming the subspace
		size_t idx;//index of the starting point
	};


	//each vertex corresponds to several subspaces
	struct vertex_data 
	{
		subspace own;//proper subspace
		size_t next_check;//which act on next?
		size_t lim_check;//will no longer act on this
		//attached, all contain their own
		std::vector<subspace> m_sbs;
	};


	std::vector<vertex_data> m_data;

	//TODO: Get rid of the extended space
	//column vectors for all points forming the subspaces
	//in the extended affine space (last component = 1)
	//the initial memory area is used for temporary needs
	std::vector<Real> m_points;

	Real* get_temp() { return m_points.data(); };


	//find all subspaces
	//returns the number of control points for the maximum subspace
	//or 0 if the limit is exceeded
	size_t compute(
		const ims_graph& gm,
		size_t dim,
		std::span<const edge_map> ri,
		std::span<const edge_ball> vb,
		const Real eps,		//relative error
		size_t max_subspaces //maximum temporary number of subspaces
	);

private:

	//the subspace itself must already be in get_temp()
	bool append_subspace(
		size_t rows,
		size_t cols,
		size_t dst_ver);//to which vertex do we add the subspace?
};