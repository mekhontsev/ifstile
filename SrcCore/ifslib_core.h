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
#include "palette.h"

struct report_params;

struct ifslib_core
{
	ifslib_core();

	//returns the number of blocks found in the input, or 0 on error
	int32_t init(const std::string& aifs);

	// Resolves a block identifier to its internal 0-based index without selecting it.
	// Accepts block string ID (@id) or display name ($n).
	// Pass an empty string to get the index of the first non-hidden block.
	// Must be called after init() has succeeded.
	// Returns block_id_max if no matching block is found or init() was not called.
	block_id_t get_block_idx(std::string_view block_id) const;

	// Selects a block by its 0-based index (as returned by get_block_idx()).
	// Initializes block metrics; must be called before any root_* or render() calls.
	// Returns number of variables in the block or 0 on error or empty block.
	int32_t set_block(int32_t block_idx);

	// Overrides the active root attractor set within the already-selected block.
	// root_idx — 0-based index of the variable within the block (as returned by get_var_idx()).
	//            Valid range is 0 to set_block() return value minus 1.
	// Must be called after set_block() has succeeded. The override is reset on the next set_block() call.
	// Returns the Euclidean dimension of the projected attractor (DIM >= 0) on success, or -1 on failure.
	int32_t set_root(int32_t root_idx);

	// Resolves a variable name to its 0-based index within the currently selected block.
	// Must be called after set_block() has succeeded.
	// Returns the variable index on success, or -1 if no block is selected or the variable is not found.
	int32_t get_var_idx(std::string_view var_name) const;

	// Computes all geometric diameters of the current root attractor set.
	// Must be called after set_block(). Returns false on error.
	//
	// diams           — output array, resized to 1 + N * 2 * DIM elements:
	//                     diams[0]              = N (number of diameter pairs)
	//                     diams[1 + i*2*DIM + j]       = endpoint a_i[j]  (j = 0..DIM-1)
	//                     diams[1 + i*2*DIM + DIM + j] = endpoint b_i[j]  (j = 0..DIM-1)
	// max_queue_size  — search budget: larger values find more diameters but are slower.
	// max_result_size — maximum number of diameter pairs to return.
	int32_t calc_diams(std::vector<double>& diams,
		int32_t max_queue_size = 100'000'000, int32_t max_result_size = 1000);

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
	int32_t	 calc_dists(const double* pt, int32_t ps_dim, std::vector<double>& dists,
		int32_t max_queue_size = 100'000'000, int32_t max_result_size = 1000);


	// Returns the variable name of the root attractor set at the given 0-based index
	// within the currently selected block.
	// Returns an empty string_view if root_idx is out of range or no block is selected.
	std::string_view get_root_name(int32_t root_idx) const;


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

	// Renders the current root attractor into dst.
	// Must be called after set_block(). Returns 1 on success, 0 on error.
	int32_t render(ims_bitmap& dst);

	// Computes analytical information for the selected block and writes it to console.
	// what — type of information: "Components", "Dimension", "Evaluation",
	//         "NormalMaps", "Projection", "Subspaces", "AST".
	// Must be called after set_block(). Returns true on success, false on error.
	int32_t information(const char* what);

	// Computes the neighbor intersection graph for the currently selected block.
	// ires     — output: receives computation statistics and result flags.
	// settings — input: controls precision, budget, and arithmetic mode.
	// Must be called after set_block() and before custom_ifs() or boundary_dim().
	// Returns true on success, false on error.
	int32_t calc_neighbor_graph(inter_result& ires, const integer_ims::settings& settings);

	// Generates a custom AIFS program from the previously computed neighbor graph.
	// rp            — parameters controlling which analysis components to generate.
	// boundary_mode — if true, generates boundary AIFS for every attractor (rp is ignored).
	// Must be called after calc_neighbor_graph(). Output goes to console.
	// Returns true on success, false on error.
	int32_t custom_ifs(const report_params& rp, bool boundary_mode);

	// Returns the Hausdorff dimension of the boundary of the current block's attractor,
	// computed from the neighbor graph built by calc_neighbor_graph().
	// Returns 0 if tiles only touch at isolated points, -1 if completely disjoint,
	// or NaN on failure.
	double boundary_dim();

	// Used for rendering only
	// space_dim = dimension of the attractor's ambient space
	// section_dim = 2 or 3
	// Returns true on success, false if no block is selected or parameters are invalid.
	// params layout: [origin, e1, e2,...]
	// origin - point on the section subspace, e1, e2,  - basis of the section subspace
	// length of section_params should be (1+section_dim)*space_dim
	int32_t set_section(const double* params, int32_t space_dim, int32_t section_dim);

	// Sets the camera/viewport for subsequent render() calls.
	// Must be called after set_block(). num_params must be 0, 4, or 10:
	//   0  — reset to automatic (fit to attractor on every render() call).
	//   4  — 2D camera: [cx, cy, r, angle_deg].
	//   10 — 3D camera: [loc.x, loc.y, loc.z, ref.x, ref.y, ref.z, up.x, up.y, up.z, fov_deg].
	// Returns true on success, false on error.
	int32_t set_camera(const double* camera_params, int32_t num_params);

	// Returns the substitution graph of the currently selected block as a flat int32 array.
	// Must be called after set_block(). Returns nullptr if no block is selected or the graph is unavailable.
	//
	// Array layout: [NE, NV, NM, e0.src, e0.dst, e0.map, e1.src, e1.dst, e1.map, ...]
	//   NE  = number of edges
	//   NV  = number of vertices
	//   NM  = number of maps (contraction maps)
	//   Each edge triple: src vertex index, dst vertex index, map index (0-based).
	// Total array size: 3 + NE * 3 elements.
	// The returned pointer is valid until the next ifslib call and must not be freed by the caller.
	int32_t* get_graph();

	// Resolves a variable index to its corresponding vertex index in the substitution graph.
	// var_idx — 0-based variable index within the currently selected block (as returned by get_var_idx()).
	// Must be called after set_block(). Returns the 0-based graph vertex index on success,
	// or -1 if no block is selected, the substitution graph is unavailable,
	// or the variable has no corresponding graph vertex.
	int32_t get_vertex(int32_t var_idx);


	// Returns the neighbor graph of the currently selected block as a flat int32 array.
	// Must be called after calc_neighbor_graph() has succeeded.
	// Returns nullptr if no block is selected or the neighbor graph is empty.
	//
	// Array layout: [NE, NV, e0.ver_from, e0.ver_to, e0.e_revers, e0.e_forward, ...]
	//   NE = number of edges (result[0])
	//   NV = number of neighbor-graph vertices (result[1]; excludes original substitution vertices)
	//   Each subsequent quadruple describes one neighbor edge:
	//     ver_from, ver_to    — vertex indices (see encoding below)
	//     e_revers, e_forward — edge indices in the substitution graph (from get_graph())
	//         −1 = no edge (identity map)
	//   Total array length: 2 + NE * 4 elements.
	//
	// Neighbor relation: tile(ver_to) = e_revers.map^-1 · tile(ver_from) · e_forward.map
	//
	// Vertex index encoding:
	//   ver >= 0 — neighbor-graph vertex with index ver
	//   ver <  0 — original substitution graph vertex with index (−ver − 1)
	//
	// The returned pointer is valid until the next ifslib call and must not be freed by the caller.
	int32_t* get_neighbor_graph();

	// Returns the rational affine map at index map_idx of the substitution graph
	// of the currently selected block as a flat double array.
	// Must be called after set_block() has succeeded.
	//
	// map_idx — 0-based index into the block's contraction map list, in the same
	//           order as the .map field of the substitution graph (get_graph()).
	//           Valid range: 0 .. NM-1, where NM is result[2] of get_graph().
	//
	// Array layout: [dim, A[0][0], A[1][0], ..., A[dim-1][dim-1], b[0], ..., b[dim-1]]
	//   dim   = dimension of the rational space (result[0])
	//   A     = dim*dim matrix in column-major order (next dim*dim entries)
	//   b     = translation vector (last dim entries)
	//   The map represents x → A·x + b in the original rational space.
	//   Each matrix and translation entry is encoded as TWO consecutive doubles:
	//   the numerator followed by the denominator (both exact int64 values cast
	//   to double; the cast is verified and the function returns nullptr if any
	//   value does not fit losslessly into a double).
	// Total array length: 1 + dim*(dim + 1) * 2 doubles.
	//
	// Returns nullptr if no block is selected, map_idx is out of range, the map
	// has no rational representation, or any numerator/denominator overflows
	// double precision.
	// The returned pointer is valid until the next ifslib call and must not be
	// freed by the caller.
	double* get_map_rational(int32_t map_idx);

	enum class Param
	{
		Mode,		//color assignment mode: vertex = per graph vertex, edges = per edge, graph = by global position in the graph; for ordinary one-vertex GIFS vertex mode is monochrome but still shows boundaries and density
		Depth,		//log2 depth threshold where tiling portions become separately colored and boundaries are emphasized; deeper fractal recoloring happens only with semi-transparent palette colors, density darkening is always applied
		Quality,	//coloring quality, 2 by default (computation time grows exponentially with this parameter)
		Thickness,	//thickness of edges in pixels, 1 by default
	};

	static constexpr double default_depth=0.1;
	static constexpr colorize_params::EPAR default_mode = colorize_params::EPAR::e_edge;

	colorize_params::EPAR m_mode = default_mode;

	//log2 of the depth of the tiling for coloring; 0 means no tiling,
	//Must be non-negative and less than 1000.
	double m_depth = default_depth;

	bool m_fit = true;

	// Indicates that render parameters have been changed and the next set_block() call should reset them to default values.
	bool m_need_reset_parameters = false;

	void reset_depth() { m_depth = default_depth; }
	void reset_mode() { m_mode = default_mode; }
	void reset_palette() {m_rp.m_palette.reset(); }

	// Updates a renderer parameter for subsequent render() calls.
	// Must be called after set_block(). Returns true on success, false if
	// no block is selected, param is unrecognised, or value is out of range.
	int32_t set_parameter(Param param, double value);

	// colors are passed as RGBA quadruples [r,g,b,a]; alpha < 1 enables deeper fractal color mixing.
	// With fully opaque colors, portions selected by Depth remain flat-colored.
	// Density-based darkening is applied independently of alpha and depth.
	// If the palette is shorter than the number of assigned graph colors, indices are taken modulo palette size.
	// num_colors counts RGBA entries, not the number of doubles in the input array.
	int32_t set_palette(const double* colors, int32_t num_colors);

	//for store temporary results of calculations, to avoid reallocations on each call
	std::vector<double> m_ret_real_array;
	std::vector<int32_t> m_ret_int_array;
	std::string m_ret_string;

	render_params m_rp;

	builder2d m_builder2d;
	builder3d m_builder3d;
	builder_ext m_builder_ext;

	draw_info2d m_buf2d_di;
	draw_info_ext m_buf_ext_di;
	gbuffer3d m_buf3d_di;

	state_stack m_ss;
	ims_cmap<double> m_cm;
	std::unique_ptr<ims_info> m_nfo;

	block_info m_bi;
	standard_vars m_sv;
	oper_block* m_bb = nullptr;

	integer_ims m_cs;
	neighbors_data m_nb;

	ims_graph m_inters;
	ifs_metrics<double> m_boundary_measure;

	diam_solver m_diam_s;
	dist_solver m_dist_s;

	ims_graph_base m_neighbour_graph;
	std::vector<neighbors_data::neighbor_edge_label> m_neighbour_maps;

private:
	int32_t graph_root() const;
	bool block_valid() const;
};
