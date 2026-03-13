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

#include "pch.h"
#include "finder.h"
#include "oper_block.h"
#include "integer_ims.h"
#include "ims_full_graph.h"
#include "columns.h"
#include "dist_solver.h"
#include "affine_subspace.h"
#include "clock_print.h"
#include "block_info.h"
#include "affine_calc.h"
#include "eval_info.h"
#include "ifs_list.h"
#include "block_graph.h"
#include "ims_val.h"
#include "edge_map.h"
#include "eval_context.h"
#include "neighbors_data.h"
#include "variable.h"

oper_block* get_cur_block();

void bi_cl::set_attempts(size_t num)
{
	m_attempts_remain = num;
	m_attempts_remain_init = num;
}

void bi_cl::update_hot_time()
{
	if(m_hot_list.empty()){
		m_hot_time = ims_chrono::now();
		m_hot_used = true;
	}
};

void bi_cl::add_hot(const oper_block* b)
{
	update_hot_time();
	m_hot_list.emplace_back(b);
}

void bi_cl::clear_hot() 
{
	if(m_hot_list.empty()){
		return;
	}
	m_hot_list.clear();
	update_hot_time();
}

const oper_block* bi_cl::extract_base_from_hot(ims_random& irn)
{
	//extract a random element
	let sz = m_hot_list.size();
	let idx = irn.rng() % sz;
	if(idx + 1 < sz){
		std::swap(m_hot_list[idx], m_hot_list.back());
	}

	auto* ret = m_hot_list.back();
	m_hot_list.pop_back();

	update_hot_time();

	m_num_virtual = 0;

	m_last_selected = ret;

	return ret;
}

const oper_block* bi_cl::extract_base_from_list(ims_random& irn, search_domain sd)
{
	oper_block* ret = nullptr;

	if(sd == search_domain::Checked){
		let sz = m_domain_checked.size();
		if(sz > 0){
			ret = m_domain_checked[irn.rng() % sz];
		};
	} else if(sd == search_domain::All){
		let sz = m_domain_all.size();
		if(sz > 0){
			ret = m_domain_all[irn.rng() % sz];
		}
	}
	if(!ret){
		//the user specified an empty variation area
		m_complete = true;
		return nullptr;
	}

	m_num_virtual = 0;

	m_last_selected = ret;

	return ret;
}

void bi_cl::add_virtual(const oper_block* b)
{
	//don't insert identical ones, reuse the id field for the hash
	let hash = b->simple_hash();
	
	for(size_t i = 0; i < m_num_virtual; ++i){
		if(m_virt_list[i]->m_block_id == (block_id_t)hash){
			return;
		}
	}

	//num_virtual - how many are actually contained in m_virt_list
	if(m_num_virtual == m_virt_list.size()){
		m_virt_list.emplace_back(new oper_block);
	};

	assert(m_num_virtual < m_virt_list.size());
	auto& nb = m_virt_list[m_num_virtual];
	b->simple_copy(*nb);
	nb->m_block_id = (block_id_t)hash;


	if(m_num_virtual == 0){
		m_virtual_checked = 0;
	}
	++m_num_virtual;
}



void bi_cl::force_proto(const oper_block* b)
{
	m_last_selected = b;

	
	m_num_virtual = 0;
	m_virtual_checked = 0;
	m_num_virtual = 0;
	m_complete = false;
}

template<typename Real>
struct uniq_array
{
	std::vector<Real> m_arr;//numbers to store

	static bool eq(const Real& a, const Real& b)
	{
		return std::abs(a - b) <
			ims_num_traits<Real>::almost_zero();
	}

	size_t find(const Real d) const
	{
		let sz = m_arr.size();
		for(size_t i = 0; i < sz; ++i){
			if(eq(m_arr[i], d))return i;
		}

		return sz;//not found
	}

	void init()
	{
		std::sort(m_arr.begin(), m_arr.end());
		m_arr.erase(std::unique(m_arr.begin(), m_arr.end(), eq), m_arr.end());
	};


};

static int better_iso_ex(const oper_block& lhs, const search_info& lhs_cd, const oper_block& rhs)
{
	let res = lhs_cd.better_than(*rhs.m_calc_data);
	if(res != 0)return res;
	let li = lhs.m_block_id;
	let ri = rhs.m_block_id;
	if(li < ri)return 1;
	if(li > ri)return -1;
	return 0;
}

static bool better_iso(const oper_block* lhs, const oper_block* rhs)
{
	return better_iso_ex(*lhs, *lhs->m_calc_data, *rhs) > 0;
}


void finder::adjust_metric_hash()
{
	for(auto it = m_metric_hash.begin(); it != m_metric_hash.end();){
		auto& e = it->second.elemetns;

		assert(!e.empty());

		//need to update the best one
		let need_sort = e.front()->m_flags.marked;

		ims_erase(e, [](let* ob) { return ob->m_flags.marked; });

		if(e.empty()){//no links left
			it = m_metric_hash.erase(it);
		} else{
			if(need_sort){
				std::sort(e.begin(), e.end(), better_iso);
			}
			it = std::next(it);
		}
	}
}

void finder::adjust_structure_hash()
{
	for(auto it = m_structure_hash.begin(); it != m_structure_hash.end();){
		if(it->second.num_ref == 0){//no links left
			it = m_structure_hash.erase(it);
		} else{
			it = std::next(it);
		}
	}
}


size_t finder::get_extra_complexity(const columns& cols)
{
	return std::max(cols.get_complexity(), (size_t)10'000'000);
	//return get_complexity();
}


void finder::on_create_ifs(size_t num_edges)
{
	init();

	m_var_par.m_search_rad = 0.3;
	m_var_par.m_kernel_defect = 0;
	m_var_par.m_max_disabled = num_edges - 2;
	m_search_attempts = 0;
	m_virtual_quota = 0;
	m_search_domain = search_domain::All;
	m_max_isomers = 0;
}

finder& finder::get()
{
	ims_func_static finder s_finder;
	return s_finder;	
}

void finder::set_default()
{
	m_use_full_search = false;
	m_check_new_found = false;
	m_hide_filtered = false;
	m_store_time_to_file = false;
	m_skip_maximized = false;

	m_search_domain = search_domain::All;
	m_max_isomers = 0;
	m_search_attempts = 0;
	m_virtual_quota = 0;
	m_proto_complexity_mul = 0;
	m_find_prec = 0;
	m_max_bits = std::numeric_limits<int_number>::digits;
	m_var_par.set_default();
}


uint32_t finder::get_rate()
{
	uint32_t ret = m_last_attempts;
	m_last_attempts = 0;
	return ret;
}

void finder::init()
{
	m_bi_map_vec.clear();
	m_structure_hash.clear();
	m_metric_hash.clear();
	m_list_status = e_list_status::just_loaded;
}

static void reset_name(oper_block* b)
{
	let& br = b->m_calc_data;
	if(!br || !br->m_structure){
		return;
	}

	using Real = search_info::Real;

	let dim = br->get_dim(0);
	int idim = int(std::floor(dim + Real(0.5)));
	bool exact = std::abs(dim - idim) < ims_num_traits<Real>::almost_zero();
	if(exact){
		b->m_name = std::to_string(idim);
	} else{
		b->m_name = std::to_string(dim);
	}

	b->m_name += "_";

	auto str = std::to_string(int(std::floor(br->m_aspect_ratio * 1000 + Real(0.5))));
	if(str.length() < 3)str.insert(0, 3 - str.length(), '0');
	b->m_name += str;
};


static block_id_t get_base_block_id(const oper_block* b)
{
	let& i2d = b->get_list().m_id2data;
	for (; b; b = b->get_parent()) {
		if (b->m_flags.only_view || b->has_js_parent())continue;
		let id = b->m_block_id;
		if (id != block_id_max && i2d[id].m_str_id != ims_max) {
			assert(!b->m_flags.priv);
			return id;
		}
		//the block does not have a string id, let's check parent
	}

	return block_id_max;
}

static size_t get_search_index(const finder::map& bi_map, const oper_block* b)
{
	let base_block_id = get_base_block_id(b);
	auto it = bi_map.find(base_block_id);
	if (it == bi_map.end())return ims_max;
	return it->second;
}

size_t finder::get_search_index(const oper_block* b) const
{
	return ::get_search_index(m_bi_map, b);
};

void finder::clear_search_domain()
{
	m_num_attempts = 0;
	m_init_index = 0;
}

void finder::init_search_domain(ifs_list& lst)
{
	clear_search_domain();

	m_bi_map_vec.clear();
	m_bi_map.clear();

	size_t num = 0;
	for (let id : lst.m_blocks) {
		//only for blocks that have a non-empty string ID
		if (lst.get_str(id).empty())continue;
		auto* sr = lst.get_block(id);

		//initialize the block
		check_block(sr);

		if (sr->m_flags.only_view || sr->m_js_parent)continue;

		if (!sr->can_be_proto() && !sr->can_be_proto_ex()) {
			continue;
		}
		
		m_bi_map[id] = num++;
	}

	m_bi_map_vec.resize(num);
	for (let& v : m_bi_map) {
		auto& m = m_bi_map_vec[v.second];
		m.m_complete = false;
		m.m_base_block = lst.get_block(v.first);
		assert(m.m_base_block);
	}
	
	for (let id : lst.m_blocks) {
		auto* sr = lst.get_block(id);
		if (sr->m_flags.only_view)continue;

		let idx = get_search_index(sr);
		if (idx == ims_max)continue;

		auto& m = m_bi_map_vec[idx];

		//insert the base element
		m.m_domain_all.emplace_back(sr);
		if (sr->m_flags.checked) {
			m.m_domain_checked.emplace_back(sr);
		}

	}
}


int compare_matrix(const ims_val* v1, const ims_val* v2)
{
	//TODO: should be reworked
	let v1n = v1->is(ims_val::ETP::number);
	let v2n = v2->is(ims_val::ETP::number);

	if (v1n) {
		if (!v2n)return -1;
		let d = v1->get_real() - v2->get_real();
		if (d < 0)return -1;
		if (d > 0)return 1;
		return 0;
	}
	if (v2n) {
		return 1;
	};
	
	let& a1 = v1->MatR();
	let& a2 = v2->MatR();

	using Real = ims_val::Real;
	let nc = (size_t)a1.cols();
	let nr = (size_t)a1.rows();

	for(size_t c = 0; c < nc; ++c){
		let& c1 = a1.col(c);
		let& c2 = a2.col(c);
		for(size_t r = 0; r < nr; ++r){
			let d = c1[r] - c2[r];
			if(std::abs(d) > ims_num_traits<Real>::almost_zero()){
				return d < 0 ? -1 : 1;
			}
		}

	}

	return 0;//identical
};


////////////////////////////////////////////////////////////////////////////////

void finder::check_extra_isomers()
{
	//reset flags
	for(auto& v : m_metric_hash){
		auto& elems = v.second.elemetns;
		for(auto* e : elems){
			e->m_flags.checked = false;
		}
	}

	////////////////////////////////////////////////////////////////////////////
	//mark duplicates
	std::vector<oper_block*> dups;
	for(auto& v : m_metric_hash){
		auto& elems = v.second.elemetns;
		for(auto* e : elems){
			e->m_flags.marked = false;
		}
		let sz = elems.size();
		if(sz < 2)continue;
		for(size_t i = 0; i < sz; ++i){
			auto* e = elems[i];
			if(e->m_flags.marked)continue;//already processed
			auto* s = e->m_calc_data->m_structure;
			if(!s || s->second.num_ref < 2)continue;

			dups.clear();
			dups.emplace_back(e);

			for(size_t j = i + 1; j < sz; ++j){
				auto* ej = elems[j];
				if(ej->m_calc_data->m_structure != s)continue;
				//duplicate
				dups.emplace_back(ej);
				ej->m_flags.marked = true;
			}

			if(dups.size() < 2)continue;
			//in the dups array you can delete all but one
			size_t num_del = 0;
			for(auto* ej : dups){
				//don't touch the best and those who can't
				if(ej == elems[0] || !ej->can_be_replaced())continue;
				ej->m_flags.checked = true;//delete
				++num_del;
			}
			if(num_del == dups.size()){//cancel the deletion of the first one
				dups[0]->m_flags.checked = false;
			}
		}
	}


	////////////////////////////////////////////////////////////////////////////
	//mark extra isomers; duplicates are already marked
	//goal: leave no more than m_max_isomers isomers unmarked
	for(auto& v : m_metric_hash){
		auto& elems = v.second.elemetns;
		for(auto* e : elems){
			e->m_flags.marked = false;
		}
		let sz = elems.size();
		if(sz < 2)continue;

		for(size_t i = 0; i < sz; ++i){
			auto* e = elems[i];
			if(e->m_flags.marked)continue;//already processed

	
			let base_block_id = get_base_block_id(e);

			dups.clear();
			dups.emplace_back(e);

			for(size_t j = i + 1; j < sz; ++j){
				auto* ej = elems[j];

				if(get_base_block_id(ej) != base_block_id)continue;
				//the same graph
				dups.emplace_back(ej);
				ej->m_flags.marked = true;
			}

			//there should be no more than m_max_isomers unmarked elements left in the dups array
			size_t remains = 0;//not marked
			for(auto* ej : dups){
				if(!ej->m_flags.checked){
					++remains;
				}
			}
			
			for(auto* ej : dups){
				if(remains <= m_max_isomers)break;//reached the desired
				if(ej->m_flags.checked)continue;
				//try to mark
				if(ej == elems[0] || !ej->can_be_replaced())continue;
				ej->m_flags.checked = true;//delete
				--remains;
			}
		}
	}

}

size_t finder::get_num_isomers(
	const metric_key::metric_val::arr& arr, 
	block_id_t base_block_id)
{
	//TODO: should be reworked
	size_t num_isomers = 0;
	for(let* e : arr){
		if(get_base_block_id(e) == base_block_id){
			++num_isomers;
		}
	}
	return num_isomers;
};



static bool compute_radius(
	block_info& bi,
	dist_solver& ds,
	size_t ver)
{
	auto& me = bi.m_im.me[ver];

	geom_input_data d;

	d.gm = &bi.get_fg();
	d.ri = bi.m_em;
	d.vb = bi.m_vb;
	d.root = ver;
	d.eps = ims_num_traits<block_info::Real>::epsilon();
	d.max_queue_size = 2'000'000;
	d.max_result_size = 1000;


	ds.compute(d, me.C);
	if (ims_need_stop())return false;

	let& res = ds.m_result;

	me.NR = (uint32_t)res.size();

	if (me.NR > 0) {
		me.R2 = (res.front()[0] - me.C).squaredNorm();
	}

	return true;
}

//search thread
struct search_contex 
{
	using real_number = finder::real_number;

	variator_ex vex;

	integer_ims imsc;
	neighbors_data nb;

	ims_full_graph fgc;
	ims_graph fgc_g;
	affine_builder ab;

	ims_full_inter fgi;

	uniq_array<real_number> sim_nfo;

	//all graph maps sorted by linear part
	std::vector<const ims_val*> smaps;

	//gives the serial number of the determinant based on the map number in the graph
	std::vector<size_t> map_det_idx;

	//is the map used?
	std::vector<bool> l_map_used;

	ims_graph inters;

	ims_graph inters_copy;

	std::vector<uint32_t> inter_counter;

	ims_graph::color_refinement_data crd;
	ims_graph_base topo_graph;

	uniq_array<real_number> dim_uniq;//dimensions of components in ascending order

	ims_graph inter_graph;
	std::vector<size_t> temp_inter;

	ifs_metrics<real_number> boundary_measure;

	dist_solver dhb;

	std::vector<ver_info_ex> thinfo_vers;

	graph_init_data_ptr idata2;

	std::vector<real_number> rad_invariant;

	std::vector<size_t> temp_hash;

	std::vector<size_t> idx_available_to_replace;

	search_info br;

	structure_info thinfo;

	std::vector<edge_ball> vb;

	affine_point_calc card_calc;

	//use per_graph to avoid reinitialization during runtime
	//TODO: It seems this optimization never worked because
	//block_info::eval_maps definitely calls block_calc::set_graph2
	struct thread_data
	{
		block_info m_bi2;//cannot be put into vector(
		std::unique_ptr<oper_block> m_block_sq;

		void reset() 
		{
			if (!m_block_sq) {
				m_block_sq = std::make_unique<oper_block>();
			} else {
				m_block_sq->clear();
			}
		}

		oper_block& get_block()
		{
			return *m_block_sq;
		}


		affine_calc m_bc;
		eval_info m_ev;
		eval_context m_ec;
		ast_maps m_am;
	
		////////////////////////////////////////////////////////////////////////////
		//the real block from which all the others originated
		const oper_block* m_base_x = nullptr;
		//maybe m_base or from the list of virtual ones - we vary it
		const oper_block* m_proto_x = nullptr;

	};
	std::vector<std::unique_ptr<thread_data>> thread_graphs;


#if 0
	struct result
	{
		finder::callback_reason val;
		oper_block* sr;
	};
#endif

	bool prepare_for_init(
		const finder::map& bi_map, 
		const oper_block* sr, 
		const variator_params& var)
	{
		if(sr->m_calc_data){//previously processed
			return false;
		}

		check_block(sr);

		let sr_idx = get_search_index(bi_map, sr);
		if (sr_idx == ims_max) {
			return false;
		}

		if (!sr->can_exists()) {
			return false;//not applicable
		}

		if(!sr->can_be_proto_ex()){
			return false;//not applicable
		}

		auto* ccl = thread_graphs[sr_idx].get();

		ccl->reset();
		ccl->m_block_sq->inherit_from(*sr, var, ccl->m_ev.m_opinfo2, false);

		return true;
	};

	oper_block* extract_block_for_full_search(
		finder& fnd4)
	{

		auto& irn = ims_random::getR();

		bi_cl* bcl = &fnd4.m_bi_map_vec[irn.rng() % fnd4.m_bi_map_vec.size()];
		
		if(bcl->m_complete){
			return nullptr;
		}

		++fnd4.m_num_attempts;

		//TODO - should not be accessible from here

		let base_block_id = bcl->m_base_block->m_block_id;
		let index = fnd4.m_bi_map[base_block_id];

		thread_data* ccl = thread_graphs[index].get();

		ccl->reset();

		auto& sr = ccl->get_block();

		ccl->m_proto_x = ccl->m_base_x = bcl->m_base_block;

		let fstatus = bcl->m_fs.next(sr);

		if(fstatus == full_search::status::complete){
			bcl->m_complete = true;
			--fnd4.m_num_active_graphs;
			return nullptr;
		};

		if(fstatus == full_search::status::ignore){
			return nullptr;
		}

		return &sr;
		
	}
	
	oper_block* extract_block(
		finder& fnd4,
		std::mutex& hot_mut,
		int32_t& num_changed) //how many maps have changed
	{

		auto& irn = ims_random::getR();

		//existing or newly created block
		oper_block* sr = nullptr;

		bi_cl* bcl = nullptr;

		//TODO - should not be accessible from here
		thread_data* ccl = nullptr;

		
		num_changed = 0;

		auto sd = fnd4.m_search_domain;

		//the user forced the prototype
	
		const oper_block* next_proto = nullptr;

		if(sd == search_domain::Current){//pull

			//TODO: it's better to redisign it
			let* q = get_cur_block();

			if(q && q->can_be_proto()){

				let base_block_id = get_base_block_id(q);
				let idx = fnd4.m_bi_map[base_block_id];
				if(idx != ims_max){
					next_proto = q;
					bcl = &fnd4.m_bi_map_vec[idx];
					ccl = thread_graphs[idx].get();
					bcl->set_attempts(1);
				}
			}

			if(!next_proto){
				sd = search_domain::All;//fall back
			}
		}

		if(fnd4.m_next_proto){//pushed
			if(!bcl && fnd4.m_search_attempts > 0){
				auto* q =fnd4.m_next_proto;

				if(q->can_be_proto()){
					let base_block_id = get_base_block_id(q);
					let idx = fnd4.m_bi_map[base_block_id];
					if(idx != ims_max){
						next_proto = q;
						bcl = &fnd4.m_bi_map_vec[idx];
						ccl = thread_graphs[idx].get();

						//return the current block back to the hot list
						if(ccl->m_base_x){
							std::scoped_lock lock(hot_mut);
							bcl->add_hot(ccl->m_base_x);
						}
							
						bcl->set_attempts(fnd4.m_search_attempts);
					}
						
				}
					
			}
			fnd4.m_next_proto = nullptr;
		}

		if(next_proto){
			bcl->force_proto(next_proto);
			ccl->m_base_x = ccl->m_proto_x = next_proto;
		}
		


		if(!bcl){
			bcl = &fnd4.m_bi_map_vec[irn.rng() % fnd4.m_bi_map_vec.size()];
		}

		if(bcl->m_complete){
			return nullptr;
		}

		++fnd4.m_num_attempts;


		let base_block_id = bcl->m_base_block->m_block_id;
		let idx = fnd4.m_bi_map[base_block_id];

		ccl = thread_graphs[idx].get();

		ccl->reset();

		

		

		//decrement the counter and try to extract the virtual
		if(bcl->m_attempts_remain == 0){
			if(fnd4.m_virtual_quota > 0 &&
				bcl->m_num_virtual > 0 && 
				bcl->m_virtual_checked < fnd4.m_search_attempts)
			{
				//update the prototype
				assert(ccl->m_base_x);//base - the same

				let index = irn.rng() % bcl->m_num_virtual;
				ccl->m_proto_x = bcl->m_virt_list[index].get();

				//complex logic, prototype extracted from a common list
				//can produce a virtual one; we'll check this virtual one to make sure it doesn't disappear,
				//but only once, otherwise it will be occurred on every failed
				//element that produced the virtual one
				if(bcl->m_attempts_remain_init > 0){
					++bcl->m_virtual_checked;
				} else{//check the one we just selected, but we won't do it again
					bcl->m_num_virtual = 0;
				}
			} else{
				//virtual processing completed, updating the base
				ccl->m_base_x = nullptr;
			}
		} else{
			--bcl->m_attempts_remain;
		}

		//trying to extract hot element
		if(!ccl->m_base_x){

			std::scoped_lock lock(hot_mut);

			if(fnd4.m_search_attempts == 0){//instant hot element cleaning
				bcl->clear_hot();
			} else if(!bcl->m_hot_list.empty()){

				auto* qq = bcl->extract_base_from_hot(irn);
				ccl->m_proto_x = ccl->m_base_x = qq;
					

				//"skip maximized" logic
				if(fnd4.m_skip_maximized && fnd4.m_max_isomers > 0){
					let* u = ccl->m_base_x->m_calc_data.get();

					let* best = u->get_best();

					//give the best a chance in any case
					if(u && best && best != ccl->m_base_x){
						let num_isomers =
							fnd4.get_num_isomers(
								u->m_metric->elemetns,
								get_base_block_id(ccl->m_base_x));

						if(num_isomers >= fnd4.m_max_isomers){
							bcl->set_attempts(0);//skip, update the base
							return nullptr;//repeat everything, because the hot list may not be empty yet
						}
					}
				}
				//one attempt will be made right now
				bcl->set_attempts(fnd4.m_search_attempts - 1);
			}
		}

	
		//extract from the entire list
		if(!ccl->m_base_x){
			auto* qq = bcl->extract_base_from_list(irn, sd);
			ccl->m_proto_x = ccl->m_base_x = qq;
			if(!qq){
				--fnd4.m_num_active_graphs;
				return nullptr;
			}
			//one attempt will be made right now
			bcl->set_attempts(0);
		}


		
		sr = &ccl->get_block();
	
	
		num_changed = (int32_t)vex.variate(
			*sr,
			*ccl->m_proto_x,
			fnd4.m_var_par,
			irn);

		if(0 == num_changed){
			return nullptr;
		};

			
		if(ccl->m_proto_x != ccl->m_base_x){
			num_changed = -num_changed;
		}
		
		assert(get_base_block_id(sr) == bcl->m_base_block->m_block_id);

		return sr;
	};


	
	
	finder::check_result one_step(
		ifs_list& full_list,
		oper_block*& drx,//can be replaced
		finder& fnd2,
		const columns& cols)
	{
		oper_block* sr = drx;


		let base_block_id = get_base_block_id(sr);
		let sr_index = fnd2.m_bi_map[base_block_id];
		
		auto* ccl = thread_graphs[sr_index].get();

		let eps = ims_num_traits<real_number>::almost_zero();
		auto& irn = ims_random::getR();

		auto& bi = ccl->m_bi2;
		auto& bc = ccl->m_bc;
		auto& ei = ccl->m_ev;
		auto& ec = ccl->m_ec;
		auto& am = ccl->m_am;
	

		assert(ccl->m_block_sq);

		
		
		ccl->m_bi2.set_to_recalc_graph();
	
		bi.init4(
			ccl->get_block(), 
			ec, 
			am,
			ei.m_idata4.get(), 
			bc);

		if(ims_need_stop()){
			return finder::interrupted;
		};

		if (!bi.exists()) {
			return finder::other;
		}

		let dim = bi.common_dim_proj();
		if (dim == 0) {
			return finder::other;
		}

		let& dig = bi.get_fg();//only after eval_maps

		//disabling edges should not significantly change the graph structure
		//we require the number of components to be preserved
		if(!fnd2.m_mode_full_search && !fnd2.m_search_init_mode){
			let& g1 = dig;
			let& g2 = sr->get_graph()->m_g1;

			if(g1.m_comp.size() != g2.m_comp.size() ||
				g1.num_ver() != g2.num_ver()){
				return finder::other;
			}
		}
	

		////////////////////////////////////////////////////////////////////
		let num_maps = bi.m_em.size();

		l_map_used.resize(num_maps);
		std::fill(l_map_used.begin(), l_map_used.end(), false);
		for(let& q : dig.m_edges){
			l_map_used[q.m] = true;
		}

		sim_nfo.m_arr.clear();
		smaps.clear();

		bool bad_det = false;//the zero determinant crumples everything

		bool all_sim = true;//all maps are similarities
		for(size_t i = 0; i < num_maps; ++i){
			if(!l_map_used[i])continue;

			let emi = bi.m_em[i];

			let d = emi.det_rootn;
			if(d < eps || d > 1 / eps){
				bad_det = true;
				break;
			}

			smaps.emplace_back(emi.mg.get());
			if (!emi.is_sim) {
				all_sim = false;
			}

			sim_nfo.m_arr.emplace_back(d);
		};
		if(bad_det){
			return finder::other;
		}
		sim_nfo.init();

		//////////////////////////////////////////////////

	

		std::sort(smaps.begin(), smaps.end(), [](let* m1, let* m2)
		{
			return compare_matrix(m1, m2) < 0;
		});

		size_t max_eq_interval = 0;
		size_t cur_eq_interval = 1;
		for(size_t i = 1; i < smaps.size(); ++i){
			if(compare_matrix(smaps[i - 1], smaps[i]) == 0) {
				++cur_eq_interval;
			} else{
				max_eq_interval = std::max(max_eq_interval, cur_eq_interval);
				cur_eq_interval = 1;
			}
		}
		max_eq_interval = std::max(max_eq_interval, cur_eq_interval);

		//////////////////////////////////////////////////

		map_det_idx.resize(num_maps);
		size_t num_det_neg = 0;

		for(size_t i = 0; i < num_maps; ++i){
			if(!l_map_used[i])continue;
			let& e = bi.m_em[i];
			if(e.neg_det){
				++num_det_neg;
			} else{

			}
			map_det_idx[i] = sim_nfo.find(e.det_rootn);
			assert(map_det_idx[i] < sim_nfo.m_arr.size());
		}

		////////////////////////////////////////////////////////////////////////

		fnd2.m_last_attempts++;
		auto* bcl = &fnd2.m_bi_map_vec[sr_index];
		////////////////////////////////////////////////////////////////////////
		//find intersections by 2


		inter_result ires;

		if(!fnd2.m_search_init_mode){

			let  only_osc = cols.only_osc();

			auto* bdata = ccl->m_base_x->m_calc_data.get();

			auto mi = cols.get_complexity();
			if (bdata) {
				let cx = size_t(fnd2.m_proto_complexity_mul * bdata->m_ir.m_gcx);
				mi = std::max(mi, cx);
			}

			integer_ims::settings settings
			{
				.max_inters = mi,
				.max_depth = cols.get_max_search_depth(),
				.max_bits = fnd2.m_max_bits,
				.prec = fnd2.m_find_prec,
				.mode_ori = false,
				.stop_on_overlap = only_osc,
				.stop_on_incomplete = true,//all or nothing
			};

			//IMPORTANT: check the structure
			ires  = imsc.calc_inter(nb, bi, settings);

			if(bdata){
				bdata->m_num_mut++;
			}

			let is_overlapped = ires.m_over_depth > 0;


			////////////////////////////////////////////////////////////////////
			if(is_overlapped){
				++bcl->m_num_overlapped;
			}

			if(!ires.m_completed){
				if(ires.m_overflowed){
					++bcl->m_num_overflowed;
				} else if(!is_overlapped){
					++bcl->m_num_complicated;
				}
				return finder::other;
			}

			if(is_overlapped){
				if(only_osc){
					return finder::other;
				}
				//overlap is acceptable, let's continue
			} else{
				++bcl->m_num_separated;
			}
			////////////////////////////////////////////////////////////////////

	
			br.clear();
			br.m_proto_id = ccl->m_base_x->m_block_id;
			br.m_generation = bdata ? bdata->m_generation + 1 : 1;
		} else{

			//in initialization mode we use a much higher complexity value
			integer_ims::settings settings
			{
				.max_inters = finder::get_extra_complexity(cols),
				.max_depth = ims_max,
				.max_bits = fnd2.m_max_bits,
				.prec = fnd2.m_find_prec,
				.mode_ori = false,
				.stop_on_overlap = false,
				.stop_on_incomplete = true,
			};
			
			//IMPORTANT: check the structure
			ires = imsc.calc_inter(nb, bi, settings);

			br.clear();
		}

		br.m_prec = static_cast<double>((ires.m_mode == intersect_mode::real) ? fnd2.m_find_prec : 0);
		br.m_ir = ires;
		br.m_dim_proj = (uint32_t)dim;
		br.m_melp = max_eq_interval;
		br.m_refl = static_cast<uint32_t>(num_det_neg);
		br.m_data.all_sim = all_sim;

		let DP = static_cast<real_number>(dim);
		

		//connectivity dimension
		real_number cdim_min = DP;
		//real_number cdim_max = -1;

		bool graph_ok = ires.m_completed && ires.m_depth > 0;

		if (graph_ok) {
			nb.set_idx_graph();
			graph_ok = nb.create_boundary(dig, inters);
		}

		if(graph_ok){

			let is_cantor_set = nb.m_data.empty();
			
			inters.init(idata2.get());


			//collect information about each vertex of the graph
			thinfo.m_base_block_id = get_base_block_id(sr);
			thinfo.m_num_comp = inters.m_comp.size();
			thinfo.m_num_ver = inters.num_ver();
			thinfo.m_num_edg = inters.m_edges.size();
			thinfo.m_ver_d0_fin = inters.m_ver_d0_fin;
			thinfo.m_ver_d0_inf = inters.m_ver_d0_inf;
			thinfo.m_com_graph_hash = inters.get_comp_hash();
			thinfo.m_com_graph_types = inters.num_comp_types();

			thinfo_vers.resize(inters.num_ver());
			for(size_t i = 0; i < thinfo_vers.size(); ++i){
				auto& q = thinfo_vers[i];
				q.num_enter = 0;
				q.num_exit = 0;
				q.num_self = 0;
				q.lab = i;
			}
			for(let& q : inters.m_edges){
				if(q.first == q.second){
					thinfo_vers[q.first].num_self++;
				} else{
					thinfo_vers[q.first].num_exit++;
					thinfo_vers[q.second].num_enter++;
				}
			}

			br.m_num_ver = inters.num_ver();
			br.m_num_edg = inters.m_edges.size();
			br.m_num_comp = 0;
			for(let& q : inters.m_comp){
				if(q.has_self)++br.m_num_comp;
			}


			////////////////////////////////////////////////////////////////
			//find the cardinality
			card_calc.process(vb, bi.m_em, inters);
			br.set_cardinality(0, card_calc.get_status());

			if(fnd2.m_calc_pdim2){
				inters_copy = inters;
			}


			////////////////////////////////////////////////////////////////
			//here inters is replaced, but becomes affine-invariant

			//replace the maps with the indices of their determinants
			for(auto& q : inters.m_edges){
				q.m = map_det_idx[q.m];
			};

			inters.color_refinement(crd);

			inters.init(idata2.get());

			thinfo.m_reduced_n_graph_hash = inters.get_hash();

			for(auto& q : thinfo_vers){
				q.lab = crd.lab[q.lab];
			}

			std::sort(thinfo_vers.begin(), thinfo_vers.end());

			thinfo.m_ver_info_hash = 0;
			for(let& q : thinfo_vers){
				q.hash_combine(thinfo.m_ver_info_hash);
			};

			////////////////////////////////////////////////////////////////

			//check if we've already encountered this structure
			auto res = fnd2.m_structure_hash.find(thinfo);
			if(res != fnd2.m_structure_hash.end()){
				br.m_structure = &*res;
			}

			bc.m_dim_calc.compute_all_dims(
				boundary_measure,
				inters,
				sim_nfo.m_arr);

			if (ims_need_stop()) {
				return finder::interrupted;
			}

			////////////////////////////////////////////////////////////////
			dim_uniq.m_arr.clear();

			for(size_t i = 0; i < inters.m_comp.size(); ++i){
				let& di = boundary_measure.di[i];
				//it's sufficient to check using your own dimensions
				if(di.DR == dim_relations::own){
					dim_uniq.m_arr.emplace_back(di.H);
				}
			}
			dim_uniq.init();

			let dim2 = dim_uniq.m_arr.empty() ? -1 : dim_uniq.m_arr.back();
			//early dimension check
			if(!fnd2.m_search_init_mode && !cols.is_bdim_ok(dim2)){
				return br.m_structure ?
					finder::other: //don't insert
					finder::virtual_found;
			}

			br.set_dim(0, dim2);

			////////////////////////////////////////////////////////////////////
			auto ndim2 = dim_uniq.m_arr.size();
			for(let q : dim_uniq.m_arr)if(q < eps){ --ndim2; break; }
			br.set_ndim(0, ndim2);
			////////////////////////////////////////////////////////////////////

			auto inter_mes2 = intersection_mes::all_fin;

			for(let& q : crd.lab){
				let idx = inters.m_ver2com[q];

				let& h = boundary_measure.di[idx];

				//only by positive dimension and infinite measure
				if(h.H == 0 || h.DR != dim_relations::equ)continue;

				let full_dim =
					std::abs(static_cast<real_number>(h.H - br.get_dim(0))) < eps;

				inter_mes2 = std::max(inter_mes2,
					full_dim ?
					intersection_mes::max_inf :
					intersection_mes::has_inf);
			};

			br.set_inter_mes(0, inter_mes2);

			////////////////////////////////////////////////////////////////
			//connectivity dimension

			//find the actual dimension of the components //hang test: a.c-2a.c-3c.a[D8]

			br.m_cnum = 0;

			for(size_t s = 0; s < dig.num_ver(); ++s){

				let sne = dig.num_edges(s);

				if(sne == 1){
					continue;//do not affect to the connectivity dimension
				}

				inter_graph.clear();

				//all pairwise intersections of the first rank
				for(size_t i = 0; i < sne; ++i){

					for(size_t j = i + 1; j < sne; ++j){


						let sij = nb.get_root_inter(s, i, j);

						let ri = nb.m_childs[sij];

						if(ri != ims_max){
							++br.m_cnum;
						};


						temp_inter.clear();
						if (!nb.append_item(temp_inter, sij)) {
							goto graph_not_ok;
						}
						

						real_number dc = -1;
						for(let q : temp_inter){
							let idx_graph = nb.m_data[q].idx_graph;
							let idx = inters.m_ver2com[crd.lab[idx_graph]];
							dc = std::max(dc, boundary_measure.di[idx].H);
						}

						if(dc >= 0){

							let idx = dim_uniq.find(dc);
							inter_graph.create_edge(i, j, idx);
							inter_graph.create_edge(j, i, idx);
						}
					};
				};


				real_number cdim_cur = -1;//the dimension of connectedness of the current set
				for(size_t k = 0; k < dim_uniq.m_arr.size(); ++k){

					inter_graph.init(idata2.get());

					if (inter_graph.m_comp.size() != 1 || 
						inter_graph.num_ver() != inter_graph.m_comp[0].num_ver ||
						inter_graph.num_ver() != sne)
					{
						break;
					}

					cdim_cur = dim_uniq.m_arr[k];

					//remove part for the next iteration
					ims_erase(inter_graph.m_edges, [k](let& e)
					{
						return e.m == k;
					});
				}

				cdim_min = std::min(cdim_min, cdim_cur);
				//cdim_max = std::max(cdim_max, cdim_cur);

			};

			if(is_cantor_set){
				br.m_data.con = connectedness::disconnected;
			} else{
				if(cdim_min > eps){
					if(cdim_min + eps > br.get_dim(0)){
						br.m_data.con = connectedness::strong;
					} else{
						br.m_data.con = connectedness::positive;
					}
				} else if(cdim_min >= 0){
					br.m_data.con = connectedness::regular;
				} else{
					br.m_data.con = connectedness::weak;
				}
			}
			////////////////////////////////////////////////////////////////

			//early connectedness check
			if(!fnd2.m_search_init_mode && 
				!cols.is_connectedness_ok((int64_t)br.m_data.con))
			{
				return br.m_structure ?
					finder::other : //don't insert
					finder::virtual_found;
			}


			if(fnd2.m_calc_nbh){

				if(fgc.find_neighborhoods(dig, nb, false)){
					fgc.get_nbh_graph(dig, fgc_g.m_edges);
					fgc_g.init(idata2.get());

					for(let& c : fgc_g.m_comp){
						if(c.has_other())continue;
						br.m_num_neighb += (uint32_t)c.num_ver;
					}
				}
				if(ims_need_stop()){
					return finder::interrupted;
				}

			}


			if(fnd2.m_calc_pdim2){
				int32_t npt;
				{
					CALC_TIME_HELPER(br.m_calc_time.ldim2);

					npt = (int32_t)ab.compute(
						inters_copy, 
						dim,
						bi.m_em,
						vb,
						ims_num_traits<real_number>::almost_zero(),
						1000);

				}


				if(ims_need_stop()){
					return finder::interrupted;
				}

				br.m_poly_dim2 = npt - 1;

				size_t num_sbs = 0;

				int ldim = -1;

				for(size_t v = 0; v < inters_copy.num_ver(); ++v){


					let& vd = ab.m_data[v];
					size_t sz = 0;
					size_t max_pt = 0;
					for(let& q : vd.m_sbs){
						max_pt = std::max(max_pt, q.num);
						if((int32_t)q.num == npt)++sz;
					}
					num_sbs = std::max(num_sbs, sz);

					//the dimension of the subspace containing the intersection
					int tdim = int(max_pt) - 1;
					let idx = inters.m_ver2com[crd.lab[v]];
					//Hausdorff dimension for intersection
					let hdim = boundary_measure.di[idx].H;

					if(std::abs(hdim - tdim) < eps){
						ldim = std::max(tdim, ldim);
					}
				}
				br.m_ldim2 = ldim;
				br.m_poly_mes2 = (uint32_t)num_sbs;
			}

			if(fnd2.m_calc_nbi){
				fgi.init0(dig);
				fgi.init1(nb);
				let inter_all_num = fgi.calc_inters(3);
				if(inter_all_num > 0){
					inters.clear();
					fgi.get_graph_x(inters.m_edges, dig, 2);
					inters.init(idata2.get());

					//find the power
					card_calc.process(vb, bi.m_em, inters);
					br.set_cardinality(1, card_calc.get_status());

					//replace the maps with the indices of their determinants
					for(auto& q : inters.m_edges){
						q.m = map_det_idx[q.m];
					};

					bc.m_dim_calc.compute_all_dims(
						boundary_measure,
						inters, sim_nfo.m_arr);

					if (ims_need_stop()) {
						return finder::interrupted;
					}


					real_number dim3 = -1;

					for(let& h : boundary_measure.di){
						//it's sufficient to check using own dimensions
						if(h.DR == dim_relations::own){
							dim3 = std::max(dim3, h.H);
						}
					}
					br.set_dim(1, dim3);

					auto inter_mes3 = intersection_mes::all_fin;

					for(let& h : boundary_measure.di){

						//only by positive dimension and infinite measure
						if(h.H == 0 || h.DR != dim_relations::equ)continue;

						let full_dim =
							std::abs(static_cast<real_number>(h.H  - br.get_dim(1))) < eps;

						inter_mes3 = std::max(inter_mes3,
							full_dim ?
							intersection_mes::max_inf :
							intersection_mes::has_inf);
					}

					br.set_inter_mes(1, inter_mes3);


					/*
					//Square
					i3 = k4 * i3
					i5 = k3 * i5
					i7 = k2 * i7
					i8 = k1 * i8
					i1 = k2 * (i1 | i3) | k4 * (i1 | i7)
					i2 = k3 * (i2 | i3) | k4 * (i2 | i5)
					i4 = k1 * (i4 | i5) | k3 * (i4 | i8)
					i6 = k1 * (i6 | i7) | k2 * (i6 | i8)

					k3 i5 i2 i3  k4
						i4    i1
					k1 i8 i6 i7  k2

					*/

					//check the connectedness of the intersections of parts
#if 0
					bool con = true;

					let r1 = fgi.get_interval(1);
					for(auto i = r1.first; i < r1.second; ++i){


						let si = i - r1.first;
						let& ic = inters.m_comp[inters.m_ver2com[si]];
						if(ic.dim_zero && !ic.has_other()){//one point
							continue;
						}

						if(inters.num_edges(si) == 1){
							continue;//expressed through another
						}




						fgi.get_con_graph(inter_graph.m_edges, i);
						inter_graph.init(idata2.get());


						size_t nc = 0;
						for(size_t c = 0; c < inter_graph.m_comp.size(); ++c){
							let& comp = inter_graph.m_comp[c];
							if(comp.num_ver == 1){
								let v = inter_graph.m_ver_sorted[comp.idx_sorted];
								if(inter_graph.num_edges(v) == 0)continue;
							}
							++nc;
						}

						if(nc > 1){
							con = false;
							break;
						}
					}

					br.m_data.ct3 = con ? 1 : 0;
#endif
				}
			}



		}//if (graph_ok)

graph_not_ok:

		////////////////////////////////////////////////////////////////
		//centers of mass and the dimension of the set itself
		//calculate but don't insert the metric_id yet

		metric_key mk;
		let metric_ok = bi.compute_metrics(bc.m_dim_calc);

		if(ims_need_stop()){
			return finder::interrupted;
		};


		if(metric_ok){


			////////////////////////////////////////////////////////////////
			let nv = dig.num_ver();

			temp_hash.resize(nv);

			let& im = bi.m_im;

			real_number max_rad = 0;
			uint32_t num_rad = 0;


			DynVec<real_number> p, p2;

			for(size_t i = 0; i < nv; ++i){

				if (dig.is_ver_empty(i)) {
					continue;
				}

				{
					CALC_TIME_HELPER(br.m_calc_time.r2);
					compute_radius(bi, dhb, i);
				}

				if(ims_need_stop()){
					return finder::interrupted;
				};

				let& me = im.me[i];

				max_rad = std::max(max_rad, me.R2);

				/*auto maxm = me.R2;
				for (size_t j = 0; j < dim; ++j) {
					maxm = std::max(maxm, me.I(j));
				}*/

				size_t ver_hash = 0;
				//boost::hash_combine(ver_hash, metric_key::get_key(me.R2 / max_rad));
				for(size_t j = 0; j < dim; ++j){
					let vxx = metric_key::get_key(me.I(j) / me.R2);
					boost::hash_combine(ver_hash, vxx);
				}

				let& di = im.di[dig.m_ver2com[i]];
				boost::hash_combine(ver_hash, metric_key::get_key(di.H));
				boost::hash_combine(ver_hash, static_cast<size_t>(di.DR));
				boost::hash_combine(ver_hash, me.NR);

#if 1				
				rad_invariant.clear();

				for(let& rp : dhb.m_result){

					//radius in the coordinate system of the principal axes
					p = rp[0] - me.C;
					p = me.Q.transpose() * p;

					real_number rva = 0;

					//intersection of the radius and the normalized ellipsoid
					//(r_x*t/m_x)^2 + (r_y*t/m_y)^2 = 1 
					//where t = 0 and 1 corresponds to the beginning and end of the radius
					// 1/t^2 = [(r_x/m_x)^2 + (r_y/m_y)^2]
					for(size_t k = 0; k < dim; ++k){
						if(me.I(k) > eps){
							let val = p[k] * p[k] / me.I(k);
							rva += val;
						}
					}
					rad_invariant.emplace_back(rva);
				}
				std::sort(rad_invariant.begin(), rad_invariant.end());

				for(let& q : rad_invariant){
					boost::hash_combine(ver_hash, metric_key::get_key(q));
				}


				////////////////////////////////////////////////////////////////
#if 0			//it almost didn't help

				rad_invariant.clear();

				for(size_t j = 0; j < dhb.m_result.size(); ++j){
					p = dhb.m_result[j] - me.C;
					for(size_t k = j + 1; k < dhb.m_result.size(); ++k){
						p2 = dhb.m_result[k] - me.C;
						rad_invariant.emplace_back(std::abs(p.dot(p2)) / me.R2);
					}
				}
				std::sort(rad_invariant.begin(), rad_invariant.end());

				for(let& q : rad_invariant){
					boost::hash_combine(ver_hash, metric_key::get_key(q));
				}
#endif
				////////////////////////////////////////////////////////////////
#endif

				temp_hash[i] = ver_hash;

				num_rad += me.NR;
			}
			std::sort(temp_hash.begin(), temp_hash.end());


			size_t metric_hash = 0;
			for(let h : temp_hash){
				boost::hash_combine(metric_hash, h);
			}

			br.m_num_rad = num_rad;
			br.m_max_rad = max_rad;
			////////////////////////////////////////////////////////////
			//aspect ratio
			br.m_aspect_ratio = 0;
			for(size_t i = 0; i < nv; ++i){
				if (dig.is_ver_empty(i)) {
					continue;
				}
				let& h = im.me[i].I;
				br.m_aspect_ratio = std::max(br.m_aspect_ratio, h(0) / h(dim - 1));
			}


			//similarity dimension
			real_number sd = 0;
			for(let& q : bi.m_im.di){
				sd = std::max(sd, q.H);
			}
			br.m_dim = static_cast<search_info::Real>(sd);
			////////////////////////////////////////////////////////////////

			mk.mh = metric_hash;
			//mk.gr = &sr->m_g->g;
			mk.dim = dim;
			mk.sd = sd;
			mk.nr = (uint16_t)br.m_num_rad;

			//for approximate ones, the dimension of the boundary cannot be taken into account - it is also not exact
			if(graph_ok && br.m_prec == 0){
				mk.db = br.get_dim(0) / DP;
				mk.dc = cdim_min / DP;

				mk.mb = (uint8_t)br.get_inter_mes(0);

				auto card = br.get_cardinality(0);

				//there are homeomorphic examples in the cube family
				if(card == cardinality::finite)card = cardinality::point;

				mk.card = (uint8_t)card;

				mk.pdim2 = (uint8_t)(br.m_poly_dim2 + 1);
			} else{
				mk.db = 0;
				mk.dc = 0;
				mk.mb = 0;
				mk.card = 0;
				mk.pdim2 = 0;
			}

			auto res = fnd2.m_metric_hash.find(mk);
			if(res != fnd2.m_metric_hash.end()){//this moment has already occured
				br.m_metric = &res->second;
			}
		}//if (metric_ok)

		////////////////////////////////////////////////////////////////////
		//orientations
		integer_ims::settings settings
		{
			.max_inters = std::max(br.m_ir.m_gcx, cols.get_complexity()),
			.max_depth = ims_max,
			.max_bits = fnd2.m_max_bits,
			.prec = fnd2.m_find_prec,
			.mode_ori = true,
			.stop_on_overlap = false,
			.stop_on_incomplete = true,
		};
		
		ires = imsc.calc_inter(nb, bi, settings);

		let ori_ok = ires.m_completed;

		if(!ori_ok){
			br.m_num_orientations = search_info::s_inf_orientations;
			//TODO: even if there are many orientations, reflections can be determined
		} else{
			for(let& q : nb.m_data){
				if(q.s0 == q.s1 && q.res == inter_type::both_neg){
					br.m_data.has_reflections = true;
					break;
				}
			}

			inter_counter.resize(dig.num_ver());
			std::fill(inter_counter.begin(), inter_counter.end(), 0);
			for(let& q : nb.m_data){
				if(q.s0 != q.s1)continue;
				if(q.res == inter_type::both ||
					q.res == inter_type::overlapped){
					++inter_counter[q.s0];
				}
			}
			br.m_num_orientations = 0;
			for(let v : inter_counter){
				br.m_num_orientations = std::max(br.m_num_orientations, v);
			}
		}

		////////////////////////////////////////////////////////////////////
		//late filtering by parameters - works even for a complete search
		if(!fnd2.m_search_init_mode &&
			!cols.accepted(sr, &br, false))
		{
			return br.m_structure ?
				finder::other : //don't insert
				finder::virtual_found;
		}
		////////////////////////////////////////////////////////////////////
		//there is information at this point:
		//graph_ok, br.m_structure
		//metric_ok, br.m_metric

		//who will we replace?
		size_t idx_replace = ims_max;

		let can_replace = br.m_metric && !fnd2.m_search_init_mode && !fnd2.m_mode_full_search;

		if(can_replace){

			++bcl->m_num_duplicated;

			assert(metric_ok);

			let& me = br.m_metric->elemetns;
			assert(!me.empty());

			if(fnd2.m_max_isomers == 0){//isomers are prohibited
				return br.m_structure ?
					finder::other : //don't insert
					finder::virtual_found;
			}

			//an element with such metric and structure was already found before, and it was no worse
			bool drop = false;


			//We don't allow multiple instances with the same structure and metric
			//IMPORTANT - this prevents complete duplicates
			//TODO: Linear search is inefficient!
			for(size_t i = 0; i < me.size(); ++i){
				let* q = me[i]->m_calc_data.get();
				assert(q);
				//compare with the first (and, as a rule, the only) one with the same structure
				//and hence with the same graph
				if(q && q->m_structure == br.m_structure){
					if(me[i]->can_be_replaced() && better_iso_ex(*sr, br, *me[i]) > 0){
						idx_replace = i;//what if..
					} else{
						drop = true;
					}
					break;
				}
			}

			if(drop){
				return finder::other;//won't even add a virtual one
			}

		
			size_t num_isomers = fnd2.get_num_isomers(me, base_block_id);


			if(idx_replace == ims_max && num_isomers >= fnd2.m_max_isomers){
				//replace a random element with a new one

				idx_available_to_replace.clear();

				for(size_t i = 0; i < me.size(); ++i){
					if(get_base_block_id(me[i]) != base_block_id){
						continue;//from another graph - don't touch
					}

					if(!me[i]->can_be_replaced()){
						continue;
					}

					if(i == 0){
						let is_new_best = better_iso_ex(*sr, br, *me[0]) > 0;
						if(!is_new_best){
							//you can't replace the best if the new one is worse
							continue;
						}
					}

					idx_available_to_replace.emplace_back(i);
				}

				let sz = idx_available_to_replace.size();

				if(sz > 0){//there is someone to replace
					idx_replace = idx_available_to_replace[irn.rng() % sz];
				} else{//TODO: understand - whether it is worth inserting the isomer as a virtual one
					return br.m_structure ?
						finder::other : //don't insert
						finder::virtual_found;
				}
			}
		}

		////////////////////////////////////////////////////////////////////////
		//insertion or replacement will definitely occur

		if (!fnd2.m_search_init_mode) {

			sr->m_timestamp = ims_chrono::ms_since_epoch();

			if (fnd2.m_store_time_to_file) {
				sr->m_flags.has_timestamp = true;
			}

			if (idx_replace == ims_max) {
				full_list.move_block(ccl->m_block_sq, "");
			}else{
				//save the id of the old one
			}
		}

		
		//adding metrics
		if(metric_ok && !br.m_metric){
			br.m_metric = &fnd2.m_metric_hash[mk];
			br.m_metric->id = sr->m_block_id;
		}

		//adding a structure
		if(graph_ok && !br.m_structure){
			structure_info::dim_data dd;
			assert(sr->m_block_id != block_id_max);
			dd.id5 = sr->m_block_id;
			dd.num_ref = 0;
			auto res = fnd2.m_structure_hash.emplace(std::move(thinfo), dd);
			assert(res.second);
			br.m_structure = &*res.first;
		}




		////////////////////////////////////////////////////////////////////////
		//after this, it will no longer be possible to change br directly
		auto* ns = br.m_structure;
		auto* nm = br.m_metric;
		sr->m_calc_data.reset(new search_info(br));
#define br 
		////////////////////////////////////////////////////////////////////////

		if(idx_replace != ims_max){

			auto& me = nm->elemetns;


			//will be changed
			auto* dst = me[idx_replace];

			//the ability to replace the graph is currently disabled
			assert(get_base_block_id(sr) == get_base_block_id(dst));

			auto& q = dst->m_calc_data;

			////////////////////////////////////////////////////////////////////
			//we'll clear the structure of the one we've decided to replace
			auto* s = q->m_structure;
			if(s && ns != s){
				--s->second.num_ref;
				if(!s->second.num_ref){
					fnd2.m_structure_hash.erase(s->first);
					q->m_structure = nullptr;
				}
				if(ns){
					++ns->second.num_ref;
				}
			}

			////////////////////////////////////////////////////////////////////

			//reuse the name and flags
			let old_name(dst->m_name);
			let old_flags(dst->m_flags);
			let best_time = me.front()->m_timestamp;

			{
				std::scoped_lock lock(oper_block::s_access_lock);
				*dst = std::move(*sr);//destructive copy
				dst->m_name = old_name;
				dst->m_flags.checked = old_flags.checked;
				dst->m_flags.hidden = old_flags.hidden;
				if(old_flags.has_timestamp){
					dst->m_flags.has_timestamp = true;
				}

			}

			drx = dst;//return-replacement

			//update the best one
			if(idx_replace > 0 && better_iso(me[idx_replace], me.front())){
				std::swap(me.front(), me[idx_replace]);
				idx_replace = 0;
			}

			if(idx_replace > 0)
			{
				return finder::some_replaced;
			}


			//a new better one just came out and replaced the old one
			assert(me.front() == dst);

			if(best_time > 0){
				dst->m_timestamp = best_time;
			}


			return finder::best_replaced;
		}
#define idx_replace



		//register
		if(ns){
			++ns->second.num_ref;
		}
		if(nm){//insert new
			auto& me = nm->elemetns;
			me.emplace_back(sr);
			//update the best one if necessary
			if(me.size() > 1 && better_iso(me.back(), me.front())){
				std::swap(me.front(), me.back());
			}
		}

		if(fnd2.m_search_init_mode){
			return finder::other;
		}
		////////////////////////////////////////////////////////////////////
		//processing what was just found
		////////////////////////////////////////////////////////////////////

		if(sr->m_name.empty()){//full search assigns names automatically
			if(nm){
				let& me = nm->elemetns;
				if(!me.empty()){
					sr->m_name = me.front()->m_name;
				}
			}
			if(sr->m_name.empty()){
				reset_name(sr);//default name
			}
		}

		if(ccl->m_base_x->m_calc_data){
			ccl->m_base_x->m_calc_data->m_num_mut_success++;
		}

		if(!cols.accepted(sr, sr->m_calc_data.get(), true)){
			//the element was added, but is not visible
			if(fnd2.m_hide_filtered){
				sr->m_flags.hidden = true;
			}
		}

		if(fnd2.m_check_new_found){
			sr->m_flags.checked = true;//do not call set_checked!
		}


		

		ccl->m_block_sq.release();//must save sr

		return finder::new_found;

	}

	
};

void finder::find_set(
	finder& fnd3,
	ifs_list& full_list,
	const columns& cols,
	std::mutex& hot_mut,
	std::function<void(check_result r, oper_block* b)> search_cb)
{

	fnd3.m_mode_full_search = fnd3.m_use_full_search;//use at the start
	
	fnd3.m_num_active_graphs = fnd3.m_bi_map_vec.size();

	if(fnd3.m_mode_full_search){
		for(auto& s : fnd3.m_bi_map_vec){
			let frad = int64_t(ceil(fnd3.m_var_par.m_search_rad));
			if(!s.m_fs.reset(*s.m_base_block, frad)){
				s.m_complete = true;
				--fnd3.m_num_active_graphs;
			}

		}
	}

	fnd3.m_search_init_mode = true;
	fnd3.m_next_proto = nullptr;
	fnd3.m_start_time = ims_chrono::now();

	fnd3.m_calc_nbh = false;
	{
		fnd3.m_calc_nbh = cols.used(column_id::NBH);
	}

	fnd3.m_calc_pdim2 = false;
	{
		fnd3.m_calc_pdim2 = 
			cols.used(column_id::SDIM2) || 
			cols.used(column_id::SM2) || 
			cols.used(column_id::LDIM2);
	}

	fnd3.m_calc_nbi = false;
	{
		fnd3.m_calc_nbi =
			cols.used(column_id::INFM3) ||
			cols.used(column_id::DIM3) ||
			cols.used(column_id::Z3); //  ||	cols.used(column_id::CT3) 
	}

	ims_random::getR().seed();
	search_cb(check_result::init_started, nullptr);

	search_contex ctx;


	ctx.thread_graphs.resize(fnd3.m_bi_map_vec.size());
	for(auto& v: ctx.thread_graphs){
		v.reset(new search_contex::thread_data);
	}

	IMS_SCOPE([&]
	{
		fnd3.clear_search_domain();
		fnd3.m_search_init_mode = false;//TODO: maybe not necessary
	});


	//only after initialization of search structures
	if(fnd3.m_list_status < e_list_status::just_search){
		fnd3.m_list_status = e_list_status::just_search;
	}

	for(size_t i = 0; i < full_list.size(); ++i){
		fnd3.m_init_index = i;

		auto* sr = full_list.get_block_by_idx(i);
		let is_ok = ctx.prepare_for_init(fnd3.m_bi_map, sr, fnd3.m_var_par);
		if (!is_ok){
			continue;//try further
		}

		//sr = &bi->get_block();

		let res = ctx.one_step(full_list, sr, fnd3, cols);

		if(res == check_result::interrupted){
			search_cb(res, sr);
			return;
		}

		assert(res == check_result::other);
			
	}

	fnd3.m_search_init_mode = false;

	fnd3.m_start_time = ims_chrono::now();
	search_cb(search_started, nullptr);

	for(;;){

		if(fnd3.m_num_active_graphs == 0){//nothing to do
			break;
		}

		int32_t num_changed = 0;

		//existing or newly created block

		oper_block* sr = nullptr;
		
		if(fnd3.m_mode_full_search){
			sr = ctx.extract_block_for_full_search(fnd3);
		} else{
			sr = ctx.extract_block(fnd3, hot_mut, num_changed);
		}

		if (ims_need_stop()) {
			break;
		}

		if(!sr){
			continue;//try further
		}

		//can replace sr!
		let res = ctx.one_step(full_list, sr, fnd3, cols);

		if(	res == check_result::interrupted)
		{
			search_cb(res, sr);
			return;
		}

		if (res == check_result::some_replaced)
		{
			search_cb(res, sr);
			continue;
		}

		if(res == check_result::other){
			continue;
		}


		let base_block_id = get_base_block_id(sr);
		let sr_index = fnd3.m_bi_map[base_block_id];

		auto& bcl = fnd3.m_bi_map_vec[sr_index];

		switch(res){

		case check_result::best_replaced:

			if(!sr->m_flags.hidden){
				//give more attempts if a visible element is found
				bcl.set_attempts(fnd3.m_search_attempts);
				if(fnd3.m_search_attempts > 0){
					std::scoped_lock lock(hot_mut);
					bcl.add_hot(sr);
				}
			}
#if 0 //it's not clear whether it's useful
			if(fnd2.m_check_new_found){
				sr->m_flags.checked = true;//do not call set_checked!
				bcl.m_domain_checked.emplace_back(sr);
			}
#endif

			search_cb(check_result::best_replaced, sr);
			break;
		case check_result::new_found:

			sr->m_calc_data->m_changed_maps = num_changed;


			++bcl.m_num_found;
			bcl.m_domain_all.emplace_back(sr);

			if(fnd3.m_check_new_found){
				bcl.m_domain_checked.emplace_back(sr);
			}

			if(!sr->m_flags.hidden){
				//give more attempts if a visible element is found
				bcl.set_attempts(fnd3.m_search_attempts);
				if(fnd3.m_search_attempts > 0){
					std::scoped_lock lock(hot_mut);
					bcl.add_hot(sr);
				}
			}

			
			search_cb(finder::new_found, sr);
			break;
		case finder::virtual_found:
			
			if(!fnd3.m_mode_full_search && fnd3.m_virtual_quota > 0)
			{
				if(bcl.m_num_virtual < fnd3.m_search_attempts){
					bcl.add_virtual(sr);
				}
			}
			break;
		default:
			break;
		}

	}

}
