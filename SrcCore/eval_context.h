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
#include "variable.h"

struct oper_block;
struct ifs_list;
struct ims_edge;

struct ims_val;

//#define use_eval_arg_cahche
//#define use_call_resolver

//1) Eliminate recursion
//2) Memoize function calls
//3) Hashing with operator_ptr to avoid creating duplicate ims_val (lazy)

//relatively lightweight structure
//used when constructing a graph and computing its edge maps
//topological calculations cannot return an error
struct eval_stack
{
	std::vector<size_t> stack;
	std::vector<ims_edge> m_edges;

	void push(size_t ref);
	void pop();
};

struct eval_context
{
#ifdef use_eval_arg_cahche
	eval_context() :
		m_call_hasher(*this), 
		m_call_cache(0, m_call_hasher, m_call_hasher) {};
#endif

	////////////////////////////////////////////////////////////////////////////

	//maps for named operators (geometric only)
	//rebuilt references for evaluation
	std::vector<variable> m_refs5;

	eval_stack* m_stack = nullptr;

	const ifs_list* m_lst = nullptr;//get_func_for_call

	//how many times did we encounter the corresponding situations during the calculation?
	//track this even in geometric mode to understand that a new graph is needed
	size_t m_topo_unions = 0;//unions with number of elements>=2
	size_t m_topo_cycles = 0;

	//also use it for our own graph as a sign of the presence of variations
	size_t m_geom_templates = 0;
	
	////////////////////////////////////////////////////////////////////////////

	variable& get_ref4(size_t idx);
	const variable& get_ref4(size_t idx) const;

	//strict mode, affects only geometric calculations
	bool strict_mode() const { return m_stack != nullptr; };

	static size_t get_val_idx(bool is_geom)
	{
		return is_geom ? 0 : 1;
	}

	bool is_geom(size_t ref) const;

	bool is_ready(size_t idx, bool is_geom) const;

	//called only when creating a graph
	void set_own_block(const oper_block& block, eval_stack* stack);


	//overrides graph variables
	//topologically calculates geometric variables
	//to obtain their decomposition into atoms
	//returns false if a graph recalculation is required
	//i.e.the geometric variable has become topological or the topological variable has overrided
	bool set_block(const oper_block& block);

	//the result does not need to be freed
	const ims_val* eval_ref(size_t idx, bool is_geom = true);

	//numeric conversions
	//indexing
	//traversing references
	//calling functions
	//returns nullptr on error
	const ims_val* eval7(ast_context p, bool is_geom);

	
	//are there variations - only for own_ctx
	bool has_vars() const;

#ifdef use_call_resolver
	ankerl::unordered_dense::map<
		ast_context,
		size_t,
		ast_context::raw_hasher,
		ast_context::raw_hasher> m_call_resolver;
#endif

private:


	//all blocks whose fields we accessed to avoid duplicates
	//as well as blocks that were called without parameters
	//value - call offset
	ankerl::unordered_dense::map<block_id_t, size_t> m_fields_call;

	
	//returns ESUBTYPE::integer, real, or other
	ESUBTYPE eval_pow_exponent(ast_context p, intptr_t& e, double& v);

	
	static const ims_val* get_ast_val(const ast_context& p);
	

	const oper_block* get_func_for_call(
		const ims_val* v, 
		bool use_cache,
		size_t dim, 
		size_t& new_call_offset);


#ifdef use_eval_arg_cahche
	struct call_context
	{
		size_t start_arg;
		size_t num_args;
		block_id_t m_block_id;
	};

	struct call_hasher
	{
		call_hasher(eval_context& ec) : m_ec(ec) {};
		//compare two elements
		bool operator()(call_context c1, call_context c2) const;

		//get the element's hash
		size_t operator()(call_context c) const;

		eval_context& m_ec;
	};


	call_hasher m_call_hasher;

	ankerl::unordered_dense::set<
		call_context, call_hasher, call_hasher> m_call_cache;

#endif



#define def_ec_method2(func) const ims_val* func(ast_context c, bool is_geom)
	def_ec_method2(eval_pow);
	def_ec_method2(eval_reference);
	def_ec_method2(eval_call);
	def_ec_method2(eval_thickness);
	def_ec_method2(eval_exchange);
	def_ec_method2(eval_inversion);
	def_ec_method2(eval_color_style);
	def_ec_method2(eval_number_imm);
	def_ec_method2(eval_number);
	def_ec_method2(eval_condition);
	def_ec_method2(eval_call_built_in);
	def_ec_method2(eval_vector_imm);
	def_ec_method2(eval_diagonal);
	def_ec_method2(eval_charpoly);
	def_ec_method2(eval_csg);
	def_ec_method2(eval_companion);
	def_ec_method2(eval_neg);
	def_ec_method2(eval_index);
	def_ec_method2(eval_uni);
	def_ec_method2(eval_vector);
	def_ec_method2(eval_mul);
	def_ec_method2(eval_sum);
	def_ec_method2(eval_empty);
	def_ec_method2(eval_id);
#undef def_ec_method2
};

