// This file is part of IFStile project
// Copyright (C)2026 Dmitry Mekhontsev <mekhontsev@gmail.com>

// This library is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library.  If not, see <https://www.gnu.org/licenses/>.

#ifdef __EMSCRIPTEN__

#include "pch.h"
#include "ifslib_core.h"
#include "conbuf.h"
#include "report_params.h"


#include <emscripten.h>

bool ims_need_stop() { return false; }

struct state
{
	ifslib_core m_core;
	ims_bitmap m_bitmap;
    ims_conbuf m_conbuf;
    std::string m_console_text;

    state()
    {
        m_conbuf.redirect(std::cout);
        m_conbuf.redirect(std::cerr);
    }
};

static state g_state;

void ext_console_clear()
{
    g_state.m_console_text.clear();
    g_state.m_conbuf.clear();
}

extern "C" {


// Retrieves analytical information about the currently selected block (for all roots).
// Results are output to the console as text.
//
// what — null-terminated UTF-8 string specifying the type of information to retrieve.
//        Must be one of the following values:
//          "Components"  — map between variables and graph components
//          "Dimension"   — Hausdorff dimension using symbolic calculation (if possible) for every graph component (can be slow)
//          "Evaluation"  — IFS map evaluation data
//          "NormalMaps"  — normal maps for the attractor (only for positive-dimension blocks)
//          "Projection"  — affine projection data (projected maps and graph in the graphviz format)
//          "Subspaces"   — affine subspace for every attractor, defined by points (can be slow)
//          "AST"         — abstract syntax tree of the block definition
//
// Must be called after set_block() has succeeded.
// Returns 1 on success, 0 if the requested information type is unrecognised or
// no block is currently selected. Call get_last_output() to retrieve
// the results or error message.
EMSCRIPTEN_KEEPALIVE
int32_t information(const char* what)
{
    ext_console_clear();
    return g_state.m_core.information(what) ? 1 : 0;
}

// Generates custom AIFS using calc_neighbor_graph() results for the currently selected block.
// Results are output to the console as text.
//
// bitmask — controls which analysis components to generate:
//           0             — boundary for every attractor (boundary mode)
//           bit 0 (0x01)  — intersections between neighbors
//           bit 1 (0x02)  — connections between neighbors
//           bit 2 (0x04)  — neighborhoods
//           bit 3 (0x08)  — neighborhoods graph
//           bit 4 (0x10)  — relators for the group of IFS maps that encode the neighbor graph
//           bit 5 (0x20)  — alternative boundary
//
// lim     — maximum intersection order to enumerate. Must be >= 2.
//           2 = pairwise (A∩B); sufficient for boundary dimension.
//           3 = also triple intersections (tile corners).
//           Computation time is exponential. Use 2 unless triple
//           (or higher) meeting points are explicitly needed.
//
// Must be called after calc_neighbor_graph() has succeeded.
// Returns 1 on success, 0 on failure. Call get_last_output() to retrieve
// the analysis results or error message.

// Prefixes used to generate unique variable names for each category of
// the custom AIFS in the output. The actual names are formed
// as  <prefix><1-based index>  (e.g. "k1", "k2", ...).
//
//   eva_prefix  ("q")   – extra pre-evaluated references copied from
//                         the eval_context (ec.m_refs5 beyond the
//                         original user refs).
//   map_prefix  ("k")   – IFS contraction maps taken from the index map
//                         (bg.m_am.m_ixm.m_maps).
//   lab_prefix  ("e")   – identity-map labels for each contraction map;
//                         emitted when neighbor maps or intersections are
//                         requested.
//   ver_prefix  ("v")   – implicit (non-user) attractor sets derived
//                         from graph vertices that have no direct user
//                         reference.
//   nbm_prefix  ("m")   – neighbor conjugacy maps  r⁻¹ · ? · f  for
//                         each pairwise neighborhood relation.
//   rel_prefix  ("id_") – group relators built from products of
//                         contraction maps (report_params::relators).
//   nbi_prefix  ("i")   – pairwise boundary/intersection sets (order 2).
//   nbu_prefix  ("u")   – pairwise connection unions (order 2,
//                         report_params::connections).
//   fi_prefix   ("j")   – higher-order intersection sets (order 3, 4 …,
//                         report_params::intersections).
//   fu_prefix   ("w")   – higher-order connection unions (order 3, 4 …,
//                         report_params::connections).
EMSCRIPTEN_KEEPALIVE
int32_t custom_ifs(int32_t bitmask, int32_t lim)
{
    ext_console_clear();

    report_params rp;
    const auto bmode = bitmask == 0;
    rp.max_inter = (size_t)lim;

	rp.intersections = (bitmask & 1);
	rp.connections = (bitmask & 2);
	rp.neighbourhoods = (bitmask & 4);
	rp.neighbourhoods_graph = (bitmask & 8);
	rp.relators = (bitmask & 16);
	rp.nboundary = (bitmask & 32);

    if (!rp.empty() && lim < 2) {
        std::cerr << "invalid arguments: lim must be >= 2 when bitmask is not 0" << std::endl;
		return false;
    }

    if (!bmode && rp.empty()) {
        std::cerr << "invalid arguments" << std::endl;
        return false;
    }

    return g_state.m_core.custom_ifs(rp, bmode) ? 1 : 0;
}

// Computes the neighbor intersection graph for the currently selected block.
// Must be called after set_block() has succeeded, and before custom_ifs().
//
// ires     — output: pointer to an inter_result struct (24 bytes, align 4):
//              offset  0  uint32  m_gcx         — how many intersections were checked
//              offset  4  uint32  m_depth        — depth reached
//              offset  8  uint32  m_bits         — how many bits were used
//              offset 12  uint32  m_neigh        — number of neighbours found (valid if m_completed == 1)
//              offset 16  uint32  m_over_depth   — minimum depth where an exact overlap was found
//                                                  (0 = no overlap found, i.e. OSC condition is satisfied)
//              offset 20  uint8   m_completed    — 1 if all intersections were fully constructed;
//                                                  0 if some were left unknown due to complexity limits
//                                                  or user interruption
//              offset 21  uint8   m_overflowed   — 1 if a rational overflow occurred
//              offset 22  uint8   m_mode         — arithmetic mode: 0=rational, 1=big_rational, 2=real
//              offset 23  uint8   (padding)
//
// settings — input: pointer to an integer_ims::settings struct (20 bytes, align 4):
//              offset  0  uint32  max_inters         — maximum number of elements in the search tree;
//                                                      performance depends linearly on this parameter,
//                                                      result may be incomplete if too small.
//                                                      Default: 4000
//              offset  4  uint32  max_depth          — maximum tree depth; 0xFFFFFFFF means no limit.
//                                                      Default: 0xFFFFFFFF (no limit)
//              offset  8  uint32  max_bits           — max rational precision in bits;
//                                                      >63 = big-rational, slow but more complete results.
//                                                      Default: 63
//              offset 12  float32 prec               — floating-point search tolerance;
//                                                      0 for exact arithmetic, ~0.3 for floating-point
//                                                      or infinite-neighbor-graph cases.
//                                                      Default: 0
//              offset 16  uint8   mode_ori           — 1 = orientation-finding mode (ignores translates).
//                                                      Default: 0
//              offset 17  uint8   stop_on_overlap    — 1 = stop processing the vertex if an overlap is found.
//                                                      Default: 1
//              offset 18  uint8   stop_on_incomplete — 1 = stop graph processing as soon as at least one
//                                                      vertex cannot be fully processed.
//                                                      Default: 1
//              offset 19  uint8   (padding)
//
// Returns 1 on success, 0 on failure. Call get_last_output() to retrieve
// the error message on failure.
EMSCRIPTEN_KEEPALIVE
int32_t calc_neighbor_graph(
    inter_result* ires,
    const integer_ims::settings* settings)
{
    ext_console_clear();
    return g_state.m_core.calc_neighbor_graph(*ires, *settings) ? 1 : 0;
}


// Computes Hausdorff dimension of the boundary of the currently selected block.
// The boundary dimension is calculated based on the neighbor intersection graph
// previously computed by calc_neighbor_graph().
// Returns -1 if the boundary is empty.
// Must be called after calc_neighbor_graph() has succeeded.
// Returns the computed boundary dimension as a floating-point value.
// Returns NaN on failure. Call get_last_output() to retrieve the error message.
EMSCRIPTEN_KEEPALIVE
double calc_boundary_dim()
{
	ext_console_clear();
	return g_state.m_core.boundary_dim();
}

// Set the rendering section for the current block.
// The call only affects subsequent render() invocations.
// It replaces the automatic section derived from the attractor or $section within the block.
//
// params       — pointer to an array of doubles describing the section.
//                Layout: [origin, e1, e2, ...]
//                origin is a point on the section subspace, and e1/e2/... are
//                the basis vectors of the section subspace.
//                The array must contain (1 + section_dim) * space_dim values.
// space_dim    — dimension of the ambient attractor space.
// section_dim  — dimension of the rendering section; must be 2 or 3 and not greater than space_dim.
//
// Returns 1 on success, 0 if no block is selected or the parameters are invalid.
// Call get_last_output() to retrieve the error message on failure.
EMSCRIPTEN_KEEPALIVE
int32_t set_section(const double* params, int32_t space_dim, int32_t section_dim)
{
    ext_console_clear();
    return g_state.m_core.set_section(params, space_dim, section_dim) ? 1 : 0;
}

// Returns console output accumulated since the last ifslib call.
// Call after any failed ifslib function to retrieve the error message.
// Returned memory is valid until the next ifslib call, and should not be freed by the caller.
EMSCRIPTEN_KEEPALIVE
const char* get_last_output()
{
    g_state.m_conbuf.sync_string(g_state.m_console_text);
    return g_state.m_console_text.c_str();
}


// Sets the camera/viewport for subsequent render() calls.
// Must be called after set_block() has succeeded.
//
// camera_params — pointer to an array of doubles describing the camera.
//                 Ignored when num_params is 0.
//                 Two explicit layouts are supported, selected by num_params:
//
//                 2D camera (num_params == 4):
//                   [0]  cx  — viewport center X
//                   [1]  cy  — viewport center Y
//                   [2]  r   — radius of the circle inscribed in the screen (viewport scale)
//                   [3]  a   — rotation angle (degrees)
//
//                 3D camera (num_params == 10):
//                   [0..2]  loc  — camera location (x, y, z)
//                   [3..5]  ref  — look-at reference point (x, y, z)
//                   [6..8]  ver  — up vector (x, y, z)
//                   [9]     fov  — vertical field of view (degrees)
//
// num_params    — number of elements in camera_params; must be 0, 4, or 10.
//                 0 = reset to automatic camera (fit to attractor on every render() call).
//
// Returns 1 on success, 0 if no block is selected or num_params is not 0, 4, or 10.
// Call get_last_output() to retrieve the error message on failure.
EMSCRIPTEN_KEEPALIVE
int32_t set_camera(const double* camera_params, int32_t num_params)
{
    ext_console_clear();
    return g_state.m_core.set_camera(camera_params, num_params) ? 1 : 0;
}


// Updates the renderer state kept inside the wasm instance.
//
// param — integer selector identifying which render setting to update.
//         Use the numeric value of ifslib_core::Param:
//           0 = Mode      — selects the colorization/rendering mode used by render().
//                         The provided value is interpreted as a colorize_params::EPAR value:
//                           0 = e_edge          — each graph edge gets its own palette color
//                           1 = e_vertex        — each graph vertex gets its own palette color;
//                                                 for ordinary GIFS with a single vertex this produces
//                                                 a monochrome coloring, but boundaries and density are
//                                                 still visible; for GIFS with more than one vertex it
//                                                 visualizes the substitution structure (which parts come
//                                                 from which vertices)
//                           2 = e_graph         — palette color is assigned by global position in the graph
//                           3 = e_field_lines   — field-line coloring with power control
//                           4 = e_equipotential — equipotential coloring with power and magnitude control
//           1 = Depth     — sets the logarithmic tiling depth threshold used by tiling-like coloring.
//                         Starting from this depth, tiles are colored as separate portions and their
//                         boundaries are emphasized. If the palette contains semi-transparent colors,
//                         deeper levels are then colored fractally inside those portions. If all palette
//                         colors are opaque, the portions remain flat-colored. Independent of depth,
//                         low-density parts of the set are darkened.
//                         The value must be a non-negative number smaller than 1000.
//           2 = Quality   — sets the render quality / sampling level.
//                         The value must be in the inclusive range [1, 16].
//           3 = Thickness — sets the edge thickness in pixels.
//                         The value must be in the range [1, 1024).
//
// value — new numeric value for the selected parameter.
//         Its meaning depends on param:
//           - for Mode: the number is converted to the corresponding colorize_params::EPAR value;
//           - for Depth: the number is stored as the logarithmic tiling depth;
//           - for Quality: the number is converted to a floating-point render quality;
//           - for Thickness: the number is converted to a floating-point edge thickness.
//
// Returns 1 on success, 0 if no block is selected, param is invalid, or value
// is outside the accepted range. Call get_last_output() to retrieve the error
// message on failure.
EMSCRIPTEN_KEEPALIVE
int32_t set_parameter(int32_t param, double value)
{
    ext_console_clear();
    return g_state.m_core.set_parameter(static_cast<ifslib_core::Param>(param), value) ? 1 : 0;
}

// Updates the internal palette used by renderer color maps.
//
// colors     — pointer to a flat RGBA array in WASM memory.
//              Layout: [r0,g0,b0,a0, r1,g1,b1,a1, ...].
//              Values are expected in [0,1] range.
//              If alpha is used (a < 1), deeper levels mix colors fractally inside the portions
//              selected by the current Depth threshold. With fully opaque colors, those portions
//              stay flat-colored. Density-based darkening is applied independently of alpha and depth.
// num_colors — number of palette entries (not the number of doubles).
//              If there are fewer palette entries than assigned graph colors,
//              the color index is taken modulo num_colors.
//              A palette can also be provided directly in AIFS via
//              $palette=[[r,g,b,a], ...]. That built-in palette is loaded by
//              set_block(); calling set_palette() overrides it until the next
//              set_block() call. Calling set_palette(nullptr, 0) resets to the
//              renderer default palette, not to the built-in AIFS $palette.
//
// Pass colors=nullptr or num_colors=0 to reset the palette to defaults.
// Returns 1 on success, 0 on failure.
EMSCRIPTEN_KEEPALIVE
int32_t set_palette(const double* colors, int32_t num_colors)
{
    ext_console_clear();
    return g_state.m_core.set_palette(colors, static_cast<size_t>(num_colors)) ? 1 : 0;
}

// Parses an AIFS fractal definition and initializes the library state.
// Replaces any previously loaded data; must be called before set_block().
//
// Accepts any valid AIFS source: a single anonymous block (@), multiple
// named blocks (@id:parentId), optional JavaScript interop (export ... @@),
// and all AIFS language features ($dim, $subspace, $companion, etc.).
//
// aifs_text — null-terminated UTF-8 string containing the AIFS source.
//             Pass nullptr to clear the state without loading new data.
//
// Returns the number of blocks found in the input, or 0 on error.
// Call get_last_output() to retrieve the error message on failure.
// On success, call set_block() to select a block before rendering.
EMSCRIPTEN_KEEPALIVE
int32_t init(const char* aifs_text)
{
    ext_console_clear();
    return static_cast<int32_t>(g_state.m_core.init(aifs_text));
}

// Resolves a block identifier to its internal 0-based index without selecting it.
// Accepts the block string ID (@id).
// Pass nullptr or an empty string to get the index of the first non-hidden block.
//
// Must be called after init() has succeeded.
// Returns the 0-based block index on success, or -1 if no matching block is found
// or init() was not called.
EMSCRIPTEN_KEEPALIVE
int32_t get_block_idx(const char* block_id)
{
    let ret = g_state.m_core.get_block_idx(block_id ? block_id : std::string_view{});
    return ret == block_id_max ? -1 : static_cast<int32_t>(ret);
}

// Selects a block by its 0-based index (as returned by get_block_idx()).
// The default root is set to the first visible and buildable variable in
// the selected block, or to $root if one is present. Call set_root() after
// this function to override the default root selection.
//
// block_idx — 0-based block index. Pass -1 to select the first non-hidden block.
//
// Must be called after init() has succeeded.
// Returns number of variables in the block or 0 on error or empty block.
// Call get_last_output() to retrieve the error message on failure.
// May be called multiple times to switch between blocks without
// reinitializing the library.
EMSCRIPTEN_KEEPALIVE
int32_t set_block(int32_t block_idx)
{
    ext_console_clear();

    if (block_idx < 0) {
        block_idx = g_state.m_core.get_block_idx({});
        if (block_idx < 0) {
            std::cerr << "Invalid block index." << std::endl;
            return 0;
        }
    }
    return static_cast<int32_t>(g_state.m_core.set_block(static_cast<block_id_t>(block_idx)));
}

// Overrides the active root attractor set within the currently selected block.
//
// root_idx — 0-based index of the variable to set as root (as returned by get_var_idx()).
//            Valid range is 0 to set_block() return value minus 1.
//
// Must be called after set_block() has succeeded.
// Returns the Euclidean dimension of the projected attractor (DIM >= 0) on success,
// or -1 if the index is out of range or no block is selected.
// Call get_last_output() to retrieve the error message on failure.
// May be called multiple times to switch between root variables without
// reinitializing the library or reselecting the block.
EMSCRIPTEN_KEEPALIVE
int32_t set_root(int32_t root_idx)
{
	ext_console_clear();
	return g_state.m_core.set_root(root_idx);
}

// Resolves a variable name to its 0-based index within the currently selected block.
// Returns ALL variables (map definitions and attractor sets alike),
// not just attractor variables.
//
// var_name — null-terminated UTF-8 string with the variable name to look up.
//
// Must be called after set_block() has succeeded.
// Returns the 0-based variable index on success, or -1 if the variable is not found
// or no block is currently selected.
// Call get_last_output() to retrieve the error message on failure.
EMSCRIPTEN_KEEPALIVE
int32_t get_var_idx(const char* var_name)
{
	ext_console_clear();
	return g_state.m_core.get_var_idx(var_name ? var_name : std::string_view{});
}

// Returns the name of the variable at the given 0-based index within the currently
// selected block. Returns ALL variables (map definitions and attractor sets alike),
// not just attractor variables.
//
// var_idx — 0-based index of the variable to look up.
//
// Must be called after set_block() has succeeded.
// Returns a null-terminated string valid until the next ifslib call.
// Returns nullptr if var_idx is out of range or no block is selected.
// The returned memory should not be freed by the caller.
EMSCRIPTEN_KEEPALIVE
const char* get_var_name(int32_t var_idx)
{
	auto sv = g_state.m_core.get_root_name(var_idx);
    g_state.m_core.m_ret_string = std::string{sv};
	return sv.empty() ? nullptr : g_state.m_core.m_ret_string.c_str();
}

// Returns the substitution graph of the currently selected block as a flat int32 array.
// Must be called after set_block() has succeeded.
//
// Array layout: [NE, NV, NM, e0.src, e0.dst, e0.map, e1.src, e1.dst, e1.map, ...]
//   NE  = number of edges (result[0])
//   NV  = number of vertices (result[1])
//   NM  = number of maps / contraction maps (result[2])
//   Each subsequent triple: src vertex index, dst vertex index, map index (all 0-based).
// Total array length: 3 + NE * 3 elements.
//
// Returns nullptr if no block is selected or the substitution graph is unavailable.
// The returned pointer is valid until the next ifslib call and must not be freed by the caller.
EMSCRIPTEN_KEEPALIVE
int32_t* get_graph()
{
	ext_console_clear();
	return g_state.m_core.get_graph();
}

// Resolves a variable index to its corresponding vertex index in the substitution graph.
// var_idx — 0-based variable index within the currently selected block,
//            as returned by get_var_idx().
// Must be called after set_block() has succeeded.
//
// Use this together with get_graph() to map attractor variables to graph vertices:
//   const varIdx = wasm.get_var_idx(namePtr);  // e.g. 'S'
//   const vertIdx = wasm.get_vertex(varIdx);    // vertex in the substitution graph
//
// Returns the 0-based graph vertex index on success.
// Returns -1 if no block is selected, the substitution graph is unavailable,
// or the variable at var_idx has no corresponding graph vertex.
EMSCRIPTEN_KEEPALIVE
int32_t get_vertex(int32_t var_idx)
{
	ext_console_clear();
	return g_state.m_core.get_vertex(var_idx);
}

// Returns the neighbor graph of the currently selected block as a flat int32 array.
// Must be called after calc_neighbor_graph() has succeeded.
// Returns nullptr if no block is selected or the neighbor graph is empty.
//
// Array layout: [NE, NV, e0.ver_from, e0.ver_to, e0.e_revers, e0.e_forward, ...]
//   NE = number of edges (result[0])
//   NV = number of neighbor-graph vertices (result[1]; excludes original substitution vertices)
//   Each subsequent quadruple: ver_from, ver_to, e_revers, e_forward
//   e_revers, e_forward - edge indices in the substitution graph - from get_graph()
//        −1 = no edge (identity map)
//   Total array length: 2 + NE * 4 elements.
//
// Vertex encoding: ver >= 0 — neighbor-graph vertex; ver < 0 — original substitution vertex (−ver − 1).
// Neighbor relation: tile(ver_to) = e_revers.map^-1 * tile(ver_from) * e_forward.map
// The returned pointer is valid until the next ifslib call and must not be freed by the caller.
EMSCRIPTEN_KEEPALIVE
int32_t* get_neighbor_graph()
{
	ext_console_clear();
	return g_state.m_core.get_neighbor_graph();
}

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
EMSCRIPTEN_KEEPALIVE
double* get_map_rational(int32_t map_idx)
{
	ext_console_clear();
	return g_state.m_core.get_map_rational(map_idx);
}

// Returns the approximate enclosing ball of the currently selected root attractor.
// The returned array has DIM+1 elements: [radius, center[0], ..., center[DIM-1]].
// This is not the minimal enclosing ball; the radius is at most 3/2 of the minimal.
// For point attractors (dim = 0), the radius is 0.
// DIM is the Euclidean dimension of the rendering subspace (1, 2, or 3).
//
// Must be called after set_block() has succeeded.
// Returns a pointer to an internal double array valid until the next ifslib call.
// Returns nullptr if no block/root is selected or the attractor set is empty.
// The returned memory should not be freed by the caller.
EMSCRIPTEN_KEEPALIVE
const double* root_enclosing_ball()
{
	ext_console_clear();
	return g_state.m_core.root_enclosing_ball();
}

// Returns the Hausdorff dimension of the currently selected root attractor set.
// Must be called after set_block() has succeeded.
// Returns -1.0 if the attractor is empty (e.g. point contacts only),
// or NaN on failure. Call get_last_output() to retrieve the error message.
EMSCRIPTEN_KEEPALIVE
double root_hdim()
{
	ext_console_clear();
	return g_state.m_core.root_hdim();
}

// Returns the d-dimensional Hausdorff measure of the root attractor set,
// where d = root_hdim().
// For dim > 0: the normalized d-dimensional measure (may be infinity).
// For dim = 0: 1 for a single point, 2 for finitely many points,
//              infinity for infinitely many.
// Must be called after set_block() has succeeded.
EMSCRIPTEN_KEEPALIVE
double root_measure()
{
	ext_console_clear();
	return g_state.m_core.root_measure();
}

// Returns the center of mass of the root attractor set.
// The returned array has DIM elements: [x, y, ...] in rendering subspace coordinates,
// where DIM is the Euclidean dimension of the rendering subspace (as returned by set_root()).
// Not meaningful for point attractors (dim = 0).
// Must be called after set_block() has succeeded.
// Returns a pointer to an internal double array valid until the next ifslib call.
// Returns nullptr on error. The returned memory should not be freed by the caller.
EMSCRIPTEN_KEEPALIVE
const double* root_mass_center()
{
	ext_console_clear();
	return g_state.m_core.root_mass_center();
}

// Returns the eigenvalues of the inertia tensor (principal moments) of the root attractor,
// in ascending order. The returned array has DIM elements,
// where DIM is the Euclidean dimension of the rendering subspace (as returned by set_root()).
// Not meaningful for point attractors (dim = 0).
// Must be called after set_block() has succeeded.
// Returns a pointer to an internal double array valid until the next ifslib call.
// Returns nullptr on error. The returned memory should not be freed by the caller.
EMSCRIPTEN_KEEPALIVE
const double* root_mass_moments()
{
	ext_console_clear();
	return g_state.m_core.root_mass_moments();
}

// Returns the principal-axes matrix of the inertia tensor, stored column-major (DIM*DIM elements),
// where DIM is the Euclidean dimension of the rendering subspace (as returned by set_root()).
// Column k is the eigenvector corresponding to root_mass_moments()[k].
// Not meaningful for point attractors (dim = 0).
// Must be called after set_block() has succeeded.
// Returns a pointer to an internal double array valid until the next ifslib call.
// Returns nullptr on error. The returned memory should not be freed by the caller.
EMSCRIPTEN_KEEPALIVE
const double* root_mass_matrix()
{
	ext_console_clear();
	return g_state.m_core.root_mass_matrix();
}

// Computes all geometric diameters of the current root attractor set.
// Returns a pointer to a double array: [N, a1[0]..a1[DIM-1], b1[0]..b1[DIM-1], a2[0]..., ...]
// where N is the number of diameter pairs and DIM is the rendering dimension.
// The array has 1 + N * 2 * DIM elements. Valid until the next ifslib call.
// Returns nullptr on error.
//
// max_queue_size  — search budget (larger = slower but more complete).
// max_result_size — maximum number of diameter pairs to return.
EMSCRIPTEN_KEEPALIVE
const double* calc_diams(int32_t max_queue_size, int32_t max_result_size)
{
	ext_console_clear();
    auto& arr = g_state.m_core.m_ret_real_array;
	if (!g_state.m_core.calc_diams(arr, max_queue_size, max_result_size))
		return nullptr;
	return arr.data();
}

// Computes all points on the current root attractor most distant from the given point.
// pt        — pointer to a double array of size dim (rendering dimension).
// dim       — number of elements in pt; must match the attractor's rendering dimension.
// Returns a pointer to a double array: [N, a1[0]..a1[DIM-1], a2[0]..., ...]
// where N is the number of farthest points and DIM is the rendering dimension.
// The array has 1 + N * DIM elements. Valid until the next ifslib call.
// Returns nullptr on error.
//
// max_queue_size  — search budget (larger = slower but more complete).
// max_result_size — maximum number of farthest points to return.
EMSCRIPTEN_KEEPALIVE
const double* calc_dists(const double* pt, int32_t dim, int32_t max_queue_size, int32_t max_result_size)
{
	auto& arr = g_state.m_core.m_ret_real_array;
	ext_console_clear();
	if (!g_state.m_core.calc_dists(pt, dim, arr, max_queue_size, max_result_size))
		return nullptr;
	return arr.data();
}

// Renders into an internal RGBA bitmap of size width x height.
// Returns pointer to raw RGBA pixel data (width * height * 4 bytes),
// or NULL on failure. Valid until the next render call, and should not be freed by the caller.
EMSCRIPTEN_KEEPALIVE
const uint8_t* render(int32_t width, int32_t height)
{
    ext_console_clear();
    g_state.m_bitmap.recreate(static_cast<size_t>(width), static_cast<size_t>(height));
    if (g_state.m_bitmap.empty()) {
        std::cerr << "Failed to create bitmap of size " << width << "x" << height << std::endl;
        return nullptr;
    }

    if (!g_state.m_core.render(g_state.m_bitmap)) {
        std::cerr << "Rendering failed" << std::endl;
        return nullptr;
    }

    return reinterpret_cast<const uint8_t*>(g_state.m_bitmap.data());
}

} // extern "C"

#endif //__EMSCRIPTEN__
