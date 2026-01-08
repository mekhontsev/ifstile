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
#include "ifs_metrics.h"
#include "pool_ptr.h"
#include "ims_view.h"


struct ims_graph;
struct edge_map;


//numerical algorithm for finding the proper dimension of each component
//returns the maximum dimensions
struct affine_dim_calc 
{
	using Real = double;

	struct err_checker
	{
		Real min_err;
		size_t min_err_iters;

		void init();

		//returns true if ready
		bool check(Real v);

	};


	//calculate the dependent dimension of the component
	//returns -1 if the component does not depend on others
	static Real calc_ext_dim(
		const ims_graph& gm,
		const size_t comp_idx,//for which component of the graph
		std::span<const Real> idim);//proper dimensions of all components

	//find the component's own dimension and vertex measures
	Real compute_dim(
		//proper measure for each vertex of the entire graph,
		std::vector<Real>& mes,
		const ims_graph& gm,
		const size_t comp_idx,//for which component of the graph
		ims_view<Real> sim //similarity ratio
	);

	

	//find metric properties of all sets
	//the ret array does not decrease in size; the current number of 
	//elements in it is equal to the number of vertices in the graph
	void compute_all_dims(
		ifs_metrics<Real>& im,
		const ims_graph& gm,
		ims_view<Real> sim);


	//finds moments, including the real (not proper) measure of each set
	bool compute_moments(
		ifs_metrics<Real>& im,
		std::span<const edge_map> ri,
		const ims_graph& gm,
		std::span<const size_t> vdim);

	private:

		struct entry
		{
			size_t ver;	//which vertex does it lead to?
			Real k;		//compression ratio/logarithm of compression ratio
			Real p;		//measure multiplier 
		};


		////////////////////////////////////////////////////////////////////////////
		struct ver_info
		{
			std::vector<entry> edges;

			Real mes;//current approximation of the measure
			Real mes_new;//new approximation of measure
		

			size_t ver_in_comp;//which vertex in the graph corresponds to
		};

		//all vertices of the component
		std::vector<ver_info> m_vers;


		std::vector<entry> tmp_edges;//for temporary needs
		std::vector<entry> cnt_edges;//for temporary needs


		//moduli of matrix determinants raised to the power of the Hausdorff dimension
		//divided by the dimension of the space
		//how many times the measure is transformed under the map
		std::vector<Real> m_det_pow;


		//extended matrices
		std::vector<pool_ptr> m_mat;
		std::vector<pool_ptr> m_exm;
		pool_ptr m_sum;
};



