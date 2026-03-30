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
#include "search_info.h"
#include "full_search.h"
#include "variator.h"
#include "def_number_types.h"
#include "ims_chrono.h"

struct ims_graph_data;
struct oper_block;
struct ifs_list;

enum class search_domain
{
	All = 0,		//for all in the list
	Checked = 1,	//only for those who have a check mark at the time of launch
	Current=2,
	None=3,
	NumDomains = 3,
};

//create a separate calculation structure for each graph



struct bi_cl
{
	full_search m_fs;

	//which was selected at the time the search was launched
	std::vector<oper_block*> m_domain_checked;

	//all blocks at launch time
	std::vector<oper_block*> m_domain_all;

	//hot list - those who need to be checked first
	std::vector<const oper_block*> m_hot_list;

	//the list of those who satisfy the OSC, doesn't match those already found,
	//but doesn't pass the filter. We'll use it as a prototype.
	std::vector<std::unique_ptr<oper_block>> m_virt_list;

	const oper_block* m_base_block = nullptr;

	//how many more times to mutate the current prototype
	size_t m_attempts_remain = 0;

	//how many attempts were made last time
	size_t m_attempts_remain_init = 0;

	//how many were found without overlap and overflow from the start of the search
	size_t m_num_separated = 0;
	size_t m_num_found = 0;
	size_t m_num_overflowed = 0;
	size_t m_num_overlapped = 0;
	size_t m_num_complicated = 0;
	//how many were discarded because of duplicates
	size_t m_num_duplicated = 0;

	size_t m_num_virtual = 0;//how many virtual ones are there now?
	size_t m_virtual_checked = 0;//how many virtual ones were checked

	//when was the last time the hot list became non-empty?
	ims_chrono m_hot_time;
	bool m_hot_used = false;

	size_t m_index = 0;

	const oper_block* m_last_selected = nullptr;

	bool m_complete = true;

	////////////////////////////////////////////////////////////////////////////

	void add_hot(const oper_block* b);

	void clear_hot();

	void update_hot_time();

	//set an additional number of attempts, not counting the current one
	void set_attempts(size_t num);


	const oper_block* extract_base_from_hot(ims_random& irn);
	//get a random element
	const oper_block* extract_base_from_list(ims_random& irn, search_domain sd);
	void add_virtual(const oper_block* b);

	void force_proto(const oper_block* b);
};


struct columns;

enum class e_list_status
{
	just_loaded = 0, //the file has just been loaded
	proc_loaded = 1, //the loading has been responded to
	just_search = 2, //the search has just started
	proc_search = 3, //the search has been responded to
};

struct finder
{
	using real_number = DefNumTypes::Real;
	using int_number = DefNumTypes::Integer;

	static finder& get();


	void set_default();

	variator_params m_var_par;

	bool m_use_full_search = false;

	bool m_check_new_found = true;

	bool m_store_time_to_file = false;

	//set the hidden flag for newly found ones if they don't meet the filter conditions
	bool m_hide_filtered = false;

	//when extracting hot, skip it if the maximum number of isomers is reached
	bool m_skip_maximized = false;

	search_domain m_search_domain = search_domain::All;

	//how many isomers of one graph are allowed?
	//isomers can be from different graphs
	//an isomer can be automatically replaced during the search if it is not the last one in the graph
	size_t m_max_isomers = 0;

	//how many additional checks should be done if something is found?
	size_t m_search_attempts = 0;

	//proportion of the current number of normal attempts
	float m_virtual_quota = 0;

	//multiplier: how many times larger GCX than the prototype should be allowed during the search
	float m_proto_complexity_mul = 0;

	////////////////////////////////////////////////////////////////////////////
	size_t m_num_active_graphs = 0;
	bool m_calc_nbh = false;
	bool m_calc_pdim2 = false;
	bool m_calc_nbi = false;
	bool m_mode_full_search = false;
	//first we run through the list
	bool m_search_init_mode = false;
	//if defined, it will be the next prototype using m_search_attempts
	const oper_block* m_next_proto = nullptr;

	//when the search was started
	ims_chrono m_start_time;
	////////////////////////////////////////////////////////////////////////////

	//number of checked since the last output to the screen
	uint32_t m_last_attempts = 0;

	e_list_status m_list_status = e_list_status::just_loaded;

	////////////////////////////////////////////////////////////////////////////
	metric_key::Map m_metric_hash;
	structure_info::Map m_structure_hash;

	//only for overflow::real
	float m_find_prec = 0;

	//only for overflow::big or int
	size_t m_max_bits = std::numeric_limits<int_number>::digits;

	//how many variants were tested
	size_t m_num_attempts = 0;

	//how many elements of the general list were initialized
	size_t m_init_index = 0;
	
	using map = ankerl::unordered_dense::map<block_id_t, size_t>;
	map m_bi_map;

	//also used in GUI
	std::vector<bi_cl> m_bi_map_vec;

////////////////////////////////////////////////////////////////////////////////

	
	uint32_t get_rate();

	void init();
	
	using PTR = std::unique_ptr<oper_block>;


	enum check_result
	{
		init_started,		//finder::m_start_time contains time
		search_started,		//finder::m_start_time contains time
		interrupted,
		best_replaced,
		some_replaced,
		new_found,		//returns true if it is visible
		virtual_found,	//suitable as virtual
		other,//did not meet the criteria or initialization
	};

	static void find_set(
		finder& fnd2,
		ifs_list& full_list, 
		const columns& cols,
		std::mutex& hot_mut,//for modifying lists
		std::function<void(check_result r, oper_block* b)> cb);

	void clear_search_domain();

	size_t get_search_index(const oper_block* b) const;
	//must be called from the GUI thread
	void init_search_domain(ifs_list& lst);

	void check_extra_isomers();
	size_t get_num_isomers(
		const metric_key::metric_val::arr& arr, 
		block_id_t base_block_id);
	//remove those who are marked from the hash
	void adjust_metric_hash();

	//remove those who are marked from the hash
	void adjust_structure_hash();


	static size_t get_extra_complexity(const columns& cols);


	void on_create_ifs(size_t num_edges);

};

