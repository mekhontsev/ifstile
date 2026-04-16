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
#include "ims_bitmap.h"
#include "ims_info.h"
#include "block_info.h"
#include "standard_vars.h"
#include "builder.h"
#include "builder2d.h"
#include "builder3d.h"
#include "builder_ext.h"
#include "gbuffer3d.h"
#include "render_params.h"
#include "integer_ims.h"
#include "neighbors_data.h"
#include "diam_solver.h"
#include "dist_solver.h"

struct report_params;

struct ifs_renderer
{
	using real_number = double;

	static void fit1d2d(
		standard_vars& sv,
		block_info& bi,
		camera_ex& cc,
		size_t root,
		size_t tw,
		size_t th,
		float iter_thk,
		bool is2d);

	//returns the number of blocks found in the input, or 0 on error
	size_t init(const std::string& aifs);

	// Resolves a block identifier to its internal 0-based index without selecting it.
	// Accepts block string ID (@id) or display name ($n).
	// Pass an empty string to get the index of the first non-hidden block.
	// Must be called after init() has succeeded.
	// Returns block_id_max if no matching block is found or init() was not called.
	block_id_t get_block_idx(std::string_view block_id) const;

	// Selects a block by its 0-based index (as returned by get_block_idx()).
	// Initializes block metrics; must be called before any root_* or render() calls.
	// Returns number of variables in the block or 0 on error or empty block.
	size_t set_block(block_id_t idx);

	// Overrides the active root attractor set within the already-selected block.
	// Must be called after set_block(). Has no effect across set_block() calls.
	// Returns -1 on error, or DIM = Euqleadean dimension for projcted attractor.
	int set_root(std::string_view root_id);

	// Computes all geometric diameters of the current root attractor set.
	// Must be called after set_block(). Returns false on error.
	//
	// diams           — output array, resized to 1 + N * 2 * DIM elements:
	//                     diams[0]              = N (number of diameter pairs)
	//                     diams[1 + i*2*DIM + j]       = endpoint a_i[j]  (j = 0..DIM-1)
	//                     diams[1 + i*2*DIM + DIM + j] = endpoint b_i[j]  (j = 0..DIM-1)
	// max_queue_size  — search budget: larger values find more diameters but are slower.
	// max_result_size — maximum number of diameter pairs to return.
	bool calc_diams(std::vector<double>& diams,
		size_t max_queue_size = 100'000'000, size_t max_result_size = 1000);

	// Computes all points most distant from a given point to the current root attractor set.
	// Must be called after set_block(). Returns false on error.
	//
	// pt              — input point array of size ps_dim
	// ps_dim          — dimension of the input point
	// dists           — output array, resized to 1 + N * DIM elements:
	//                     dists[0]             = N (number of farthest points)
	//                     dists[1 + i*DIM + j] = point a_i[j]  (j = 0..DIM-1)
	// max_queue_size  — search budget: larger values find more distances but are slower.
	// max_result_size — maximum number of distance pairs to return.
	bool calc_dists(const double* pt, size_t ps_dim, std::vector<double>& dists,
		size_t max_queue_size = 100'000'000, size_t max_result_size = 1000);


	// Returns the variable name of the root attractor set at the given 0-based index
	// within the currently selected block.
	// Returns an empty string_view if root_idx is out of range or no block is selected.
	std::string_view get_root_name(size_t root_idx) const;

	// Returns DIM+1 values: [radius, center[0], ..., center[DIM-1]].
	// Not the minimal enclosing ball; radius is at most 3/2 of the minimal enclosing ball radius.
	// For point attractors, radius is 0. Returns nullptr on error.
	const double* root_enclosing_ball() const;

	// Hausdorff dimension of the root attractor set.
	// Returns -1 if the set is empty (e.g. point contacts), or NaN on failure.
	double root_hdim();

	// d-dimensional measure of the root attractor set, where d = root_hdim().
	// For dim > 0: the d-dimensional Hausdorff measure (including infinity).
	// For points(dim = 0): 1 for a single point, 2 for finitely many points, infinity for infinitely many.
	double root_measure();

	// Center of mass of the root attractor set. Array size is DIM.
	// not meaningful for point attractors.
	const double* root_mass_center() const;

	// Eigenvalues of the inertia tensor (principal moments), in ascending order. Array size is DIM.
	// not meaningful for point attractors.
	const double* root_mass_moments() const;

	// Principal-axes matrix of the inertia tensor, stored column-major (DIM*DIM elements).
	// Column k is the eigenvector corresponding to root_mass_moments()[k].
	// Not meaningful for point attractors (dim = 0). Returns nullptr on error.
	const double* root_mass_matrix() const;

	bool render(ims_bitmap& dst, float quality, float thickness);

	bool information(const char* what);

	size_t max_complexity = 1000;
	size_t max_bits = 63;
	float find_prec2 = 0;//0.3 is a reasonable value

	bool calc_neighbor_graph(inter_result& ires, const integer_ims::settings& settings);
	bool custom_ifs(const report_params& rp, bool boundary_mode);
	double boundary_dim();


	bool set_camera(const double* camera_params, size_t num_params);



	//for store temporary real arrays
	//std::vector<double> m_ret_real_array;

	render_params m_rp;

	builder2d m_builder2d;
	builder3d m_builder3d;
	builder_ext m_builder_ext;

	draw_info2d m_buf2d_di;
	draw_info_ext m_buf_ext_di;
	gbuffer3d m_buf3d_di;

	state_stack m_ss;
	ims_cmap<real_number> m_cm;
	std::unique_ptr<ims_info> m_nfo;

	block_info m_bi;
	standard_vars m_sv;
	oper_block* m_bb = nullptr;

	integer_ims m_cs;
	neighbors_data m_nb;

	ims_graph m_inters;
	ifs_metrics<real_number> m_boundary_measure;

	diam_solver m_diam_s;
	dist_solver m_dist_s;

	bool m_fit = true;

	size_t get_froot() const;
};
