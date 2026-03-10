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

struct block_class;
struct search_info;
struct operator_ptr;
struct ims_graph_base;

struct variator_params;
struct oper_block;
struct ims_info;
struct distrib_info;
struct ifs_list;
struct block_graph;
struct eval_context;
struct ovr_data;

void check_block(const oper_block*);

struct oper_block_flags
{
	oper_block_flags() 
	{
		clear();
	};

	void clear();

	//saves attributes
	void clear_but_attr();

	void clear_attr();
	
	bool
		//preprocessing completed
		ready : 1,
		//marked by user
		checked : 1,
		//hidden by user
		hidden : 1,
		//block has $dim field
		has_dim : 1,
		//block time will be saved to file
		has_timestamp : 1,
		//used for various temporary needs
		marked : 1,
		//if !own_ctx: has free (not overridden) variations
		free_var : 1,
		//if !own_ctx: has no other operators besides variations
		only_var : 1,
		//if !own_ctx: consists only of cameras, etc.
		only_view : 1,
		//temporary
		priv : 1,
		//created in js
		from_js : 1;
};


struct oper_source 
{
	std::vector<std::string> lines;
	size_t line_name = ims_max;//line with name
	size_t line_attr = ims_max;//line with attributes

	//gives a description for the variable, taken from the comment directly above
	std::vector<std::string> ref2comments;

	//comes from JS
	std::vector<std::pair<size_t, std::string>> unk2description;

	std::string md;//markdown describing the entire block
};


//base class, copyable
struct oper_block
{
	const oper_block* m_parent = nullptr;

	oper_block_flags m_flags;

	//name (description)
	std::string m_name;

	//all operations
	std::vector<sval> m_ops;

	block_id_t m_block_id = block_id_max;

	block_id_t m_parent_id = block_id_max;

	//the line where the block definition began in the source code
	//numbering starts from 1
	size_t m_line8 = 0;

	//$dim, inherited
	size_t m_dim2 = 0;

	//$subspace, inherited
	operator_ptr m_subspace;

	//number of variables with a name, the rest are created in JS
	size_t m_named_vars = ims_max;//all by default

	//converter index
	block_id_t m_conv_id = block_id_max;

	//time when it was found, ms since epoch (0 - no information)
	uint64_t m_timestamp = 0;

	//offset of the first root
	var_header::type m_first_var = var_header::nil;

	//reference to the $init function, in the js_aifs_block::m_init_funcs array
	size_t m_js_init = ims_max;

	std::shared_ptr<oper_block> m_js_parent;

	//used as a sign of block correctness after initialization
	std::shared_ptr<block_graph> m_graph;
	
	std::shared_ptr<eval_context> m_ctx;
	//non-copyable part..

	//the graph it belongs to, cannot be empty
	ims_unique_ptr<block_class> m_class;

	//information filled in during the search process
	ims_unique_ptr<search_info> m_calc_data;

	//block source code
	std::unique_ptr<oper_source> m_src2;

	//lock at the time of modification or reading m_ops
	static std::mutex s_access_lock;

	////////////////////////////////////////////////////////////////////////////

	const oper_block* get_parent() const;

	void set_parent(const oper_block* b);

	void prepare_js_parent();

	bool has_id() const { return m_block_id != block_id_max; };
	bool is_converter() const { return m_conv_id != block_id_max; };
	std::string_view str_id4() const;

	size_t simple_hash() const;

	const ifs_list& get_list() const;

	size_t get_js_init_identifier() const;

	//the vertex to be constructed in the final graph
	size_t get_froot() const;

	block_class* get_class() const;
	block_graph* get_graph() const;


	eval_context* ctx() const;
	eval_context* own_ctx() const;



	size_t get_var_from_unk(size_t unk) const;

	//number of variables in the hierarchy
	size_t num_vars() const;

	//insert several undefined values
	//returns the start index
	size_t add(size_t num);

	void clear_ops();

	operator_ptr get_ptr(size_t idx) const;

	operator_ptr get_ptr_to_elem(const ims_operator& vec, size_t idx) const;

	bool get_val(const ims_operator& h,
		bool& is_rational, double& fv, int64_t& n, int64_t& d) const;

	bool get_int64(
		int64_t& numerator,
		int64_t& denominator,
		size_t idx,
		const ESUBTYPE st) const;

	bool get_double(const ims_operator& h, double& fv) const;


	struct int_arr_ref
	{
		const int64_t* ptr;
		size_t sz;
	};

	std::string_view get_graph_id() const;

	bool get_distrib(distrib_info& di, const operator_ptr& p) const;
	//returns true if something was created randomly
	bool apply_templates(
		const control_values& arr,
		const variator_params& vp,
		int_arr_ref* rel_vec);

	//insert operator recursively
	void insert_op_ex(
		size_t dst_idx, 
		ast_context src, 
		const eval_context& ctx,
		control_values* ci = nullptr,//fill in for apply_templates
		bool do_subst = true);//enter substitutions recursively

	void generate_random_vector(size_t dst_idx,
		const operator_ptr& src, const distrib_info& di, const int_arr_ref* proto = nullptr);
	void generate_random_vector(size_t dst_idx,
		size_t dim, const distrib_info& di, const int_arr_ref* proto = nullptr);

	void generate_random_permutation(size_t dst_idx, const operator_ptr& src);
	void generate_random_number(size_t dst_idx, const distrib_info& dim);

	void remove_search();
	//get a comment on a variable
	const std::string* get_comment(size_t ref_id) const;
	//outputs in Markdown format
	std::string get_block_decription() const;

	void clear();

	size_t get_active_ref() const;
	void set_active_ref(size_t r);

	//has a dimension that may not match the parent dimension
	bool has_own_dim() const;

	size_t get_dim() const;

	void set_own_dim();

	void fix_js_parent();
	bool has_js_parent() const;
	////////////////////////////////////////////////////////////////////////////
	//operator=
	void simple_copy(oper_block& dst) const;

	//we can build without extending the definition
	bool completely_defined() const;
	
	//can it used as a prototype for search, and can it be added to domain
	bool can_be_proto() const;
	//can it used as a prototype for search, but may NOT be suitable for domain
	bool can_be_proto_ex() const;
	//can it be replaced during search
	bool can_be_replaced() const;

	//try to convert the newly inserted vector to type vector_imm
	void adjust_vector(size_t pos);

	const oper_block* elevate_empty() const;
	const oper_block* elevate_priv() const;
	
	//set the element with idx to the given type with num arguments
	//returns the index of the beginning of the arguments
	size_t add_args(size_t idx, ETYPE t, size_t num);

	size_t set_vector_ex(size_t idx, ETYPE t, ESUBTYPE st, size_t narg);
	size_t set_vector(size_t idx, ETYPE t, size_t narg);
	size_t set_vector(size_t idx, size_t narg);
	size_t set_vector_template(size_t idx, ETYPE t);

	size_t set_index_imm(size_t offset, int arg_idx);

	void set_int_poly(std::span<const int64_t> p, int64_t denom, size_t ds, size_t arg);

	//set type with one vector argument (companion, diagonal,...)
	void set_one_vector_arg(std::span<const int64_t> p, size_t ds, ETYPE t);

	//get the default operator index for the build
	//priority goes to those who are last in the block because usually
	//they depend on those who were above them, and that's natural
	size_t find_default_ref() const;

	size_t set_neg(size_t idx);

	//returns the argument index
	size_t set_power(size_t idx, intptr_t e);
	void set_exchange(size_t idx);
	void set_mobius(size_t idx);
	size_t set_power_ref(size_t idx);
	size_t set_mod_ref(size_t idx);

	void set_distribution(size_t idx, ETYPE t, ESUBTYPE s, double v1, double v2);
	void set_distribution_def(size_t idx);

	void set(size_t idx, ETYPE t, size_t primary, size_t secondary);

	//no operations, no source code, no parents.
	//for example, after clear()
	bool empty4() const;
	
	void set_integer(size_t idx, int64_t v);
	void set_integer(ims_operator& h, int64_t v);
	void set_rational(size_t idx, int32_t numer, int32_t denom);
	void set_rational(ims_operator& h, int32_t numer, int32_t denom);
	
	void set_double(size_t idx, double v);
	void set_double(ims_operator& h, double v);

	//add after prev position, updating it
	var_header::type add_var(uint32_t& prev, size_t ref, bool is_subs);
	
	//add to the beginning
	size_t add_builtin(builtin_ids bid);

	void remove_builtin(builtin_ids bid);

	bool get_builtin(operator_ptr& dst, builtin_ids id) const;
	void get_builtins(builtin_arr& dst_arr, bool rec) const;

	bool is_invalid() const 
	{
		return m_flags.ready && !m_graph;
	}

	bool convert_type_inplace(ims_operator& h, bool to_int);

	block_class& create_own_class();
	block_class& set_new_class(const ims_info* nfo);
	block_class& create_copy(const block_class* p);

	bool can_exists() const;//there is a graph and the dimension is defined
	
	//inherit from the src block and define open variations
	//returns true if random generation was used
	bool inherit_from(
		const oper_block& src,
		const variator_params& vp,
		control_values2& temp,
		bool copy_view);//copy cameras, etc.


	//copies to the current block
	//the operators are taken from override_info
	//the remaining attributes are from src
	void copy_ovr(
		const oper_block& src,
		const variator_params& vp,
		control_values& temp,
		std::span<const override_info> ovr);

	void inherit_view(const oper_block& src);

	////////////////////////////////////////////////////////////////////////////
	struct it_value
	{
		var_header::type ref6;
		var_header::type pos5;

		size_t gr() const { return var_header::unpack_ref(ref6); };//get_ref
		bool is_subs() const { return var_header::unpack_subs(ref6); };

		bool is_builtin() const { return ref_is_builtin(ref6); };

		builtin_ids get_builtin() const 
		{
			assert(is_builtin());
			return ref2builtin(ref6);
		}

		var_header& get_vh(oper_block& b) const;
		ims_operator& get_left(oper_block& b) const;
	};

	class iterator :
		public boost::iterator_facade<iterator, it_value,
		boost::forward_traversal_tag, it_value >
	{
		using POS = var_header::type;

	public:
		iterator(const oper_block* t, POS pos) :m_b(t), m_pos(pos) {}

	private:

		friend class boost::iterator_core_access;

		void increment() { m_pos = m_b->m_ops[m_pos].vh.next; }
		bool equal(iterator const& other) const { return m_pos == other.m_pos; }

		it_value dereference() const {
			return it_value{ m_b->m_ops[m_pos].vh.ref8, m_pos + var_header::offset_right };
		}

		const oper_block* m_b;
		POS m_pos;
	};

	iterator begin() const { return iterator(this, m_first_var); }
	iterator end() const { return iterator(this, var_header::nil); }

};
