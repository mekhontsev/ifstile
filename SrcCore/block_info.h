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
#include "operator_ptr.h"
#include "ifs_metrics.h"
#include "geometry.h" //style
#include "pool_ptr.h"
#include "proj_data.h"
#include "dfs.h"
#include "ims_graph.h"
#include "edge_ball.h"
#include "edge_map.h"

//maps act in Hilbert space
//the dimension of a map is the number of the coordinate from which it acts as the identity
//edge dimension = maximum over atom dimensions
//vertex dimension is determined iteratively - the maximum over outgoing edges and vertices
//dimensionless maps (e.g. scalars) - do not affect the dimension calculation
//edge_mul does not perform collapse_compos when going deeper into a lower dimension

struct affine_dim_calc;
struct variator_params;
struct affine_calc;
struct report_params;
struct eval_context;
struct eval_info;

struct ims_val;
struct ims_identifiers;
struct ast_maps;

enum class ifs_object_type : size_t
{
	normal,			//normal mode
	boundary,		//boundary
	custom,			//neighbours
	num_modes,
};

struct block_info
{
	using Integer = int64_t;
	using Real = double;


	bool compute_metrics(affine_dim_calc& dc);

	//no errors occurred as a result of initialization
	bool exists() const;
	void set_to_recalc_graph();
	void gen_next_id();

	//calculates graph, real maps, sizes of sets
	//return false if no valid vertex could be found
	bool init4(
		const oper_block& b,
		eval_context& ec,
		ast_maps& am,
		graph_init_data& gid,
		affine_calc& ac);

	const ims_graph& get_fg() const { return *m_cur_fg; };

	using alg_id_t = size_t;
	size_t get_dim_alg(alg_id_t id) const;
	size_t get_dim_proj(alg_id_t id) const;

	//get an affine map from an algebraic space
	void get_affine(Real* dst, const Real* src, alg_id_t alg_id) const;


	//returns the single dimension of the projection, if it exists, or 0
	size_t common_dim_proj() const;

	void clear_proj_data();
	const proj_data& get_proj_data() const;

public:

	ifs_metrics<Real> m_im;

	std::vector<edge_map> m_em;

	struct comp_info
	{
		alg_id_t alg_id;
		size_t dim_p;//projection dimension
	};

	std::vector<comp_info> m_comp_info;

	//sequence number from program start
	//updated every time an attempt is made to calculate the graph (even if unsuccessful)
	//reset and checked in the main thread
	//set in the worker thread
	//if 0, then a new calculation is currently in progress
	size_t m_id8 = 0;


	struct map_info
	{
		size_t proj_id = ims_max;	//projector id (for affine)
		size_t dim_p = 0;			//projection dimension

		enum e_status {

			any = 0,	//it's not clear yet
			empty,		//empty
			invalid,	//no projection
			used,		//correct and used somewhere (for maps)
		};

		e_status status = any;

		bool is_ok(bool allow_invalid = false) const
		{
			if (status == empty)return false;
			return status != invalid || allow_invalid;
		};
	};
	std::vector<map_info> m_map_info;

	//upper bound for the sizes of graph vertices
	//sizes are defined only for components that depend on components
	//that have references to themselves
	std::vector<edge_ball> m_vb;

	//ims_max if the vertex is empty or invalid
	std::vector<size_t> m_ver_dim;


	std::vector<style<Real>> m_style;




private:

	dfs_po m_dfs;//TODO: move to graph_init_data ?




	struct atom_data
	{
		pool_ptr m_v;

		//either null or affine
		pool_ptr m_v_proj;

		//ims_max means the trivial projector (for a scalar - always)
		size_t m_proj_id = ims_max;

		ast_context* m_ast = nullptr;
	
		void eval(eval_context& ec, ast_context ast);

		void project(eval_context& ec, ast_context proj, proj_data& pd, bool checked);

		//projection dimension, 0 - any
		size_t get_dim() const;

		bool invalid() const;
	};


	////////////////////////////////////////////////////////////////////////////



	std::vector<atom_data> m_atom_data;

	//the algebraic component ID is equal to the dimension for the trivial projector
	//or s_first_non_trivial_alg_id + projector ID
	//0 means not processed yet, ims_max is an error

	static constexpr alg_id_t s_first_non_trivail_alg_id = 1024;



	proj_data m_proj_data;


private: 

	
	const atom_data& get_atom(size_t idx, const ast_maps& am) const;

	void any_to_used(std::span<const ims_edge> s);

	bool init_fg(const ims_graph& g, bool allow_invalid = false);

	static void set_map(edge_map& emi, const ims_val* m, size_t dim_p);

	const ims_val* create_proj_map(size_t idx, const ast_maps& am) const;

	const ims_val* create_alg_map(
		size_t& proj_id,//returns the projector ID
		size_t idx,
		const ast_maps& am) const;


private:

	//the final graph taking into account the removed vertices and edges
	ims_graph m_fg;
	const ims_graph* m_cur_fg = nullptr;
};
