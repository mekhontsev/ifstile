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
#include "ims_full_graph.h"
#include "neighbors_data.h"
#include "clock_print.h"
#include "ims_graph_base.h"

ims_inter_lists::elem ims_inter_lists::prepare(size_t ver, size_t idx)
{
	let it = m_lists.begin() + idx;
	let ite = m_lists.end();
	std::sort(it, ite);

	//remove duplicates for overlapped
	m_lists.erase(std::unique(it, ite), ite);
	
	
	elem q;
	q.ver = ver;
	q.idx = idx;
	q.sz = m_lists.size() - idx;

	return q;
};

ims_inter_lists::HashTable::value_type&
ims_inter_lists::insert(const elem& q)
{
	auto res = m_hash.emplace(q, 0);

	if (!res.second) {//already been before
		m_lists.resize(q.idx);//restoring
	} else {
		res.first->second = m_data.size();
		data_el e;
		e.h = &*res.first;
		m_data.emplace_back(e);
	}

	return *res.first;
};



bool ims_inter_lists::need_stop() const
{
	if (ims_need_stop()) {
		return true;
	}
	if (m_lists.size() > m_max_list_size) {
		return true;
	}
	return false;
}



bool ims_inter_lists::exists(const elem& e) const
{
	return m_hash.find(e) != m_hash.end();
}

void ims_inter_lists::clear()
{
	m_data.clear();
	m_hash.clear();
	m_elem_refs.clear();
	m_lists.clear();
};


//returns true if e1 is contained in e2 (sorted lists)
bool ims_inter_lists::first_in_second(
	const ims_inter_lists::elem& e1,
	const ims_inter_lists::elem& e2) const
{
	if (e1.ver != e2.ver)return false;//from different universes
	if (e1.sz > e2.sz)return false;

	let* p1 = &m_lists[e1.idx];
	let* p1e = p1 + e1.sz;

	let* p2 = &m_lists[e2.idx];
	let* p2e = p2 + e2.sz;

	for (; p1 < p1e; ++p1) {
		for (;;) {//searcH for an element from e1 inside e2
			if (p2 == p2e || (*p1) < (*p2)) {//use sorting
				return false;
			};
			if (*p2 == *p1) {//found
				break;
			}
			p2++;
		};
	}
	return true;
};
////////////////////////////////////////////////////////////////////////////////



bool ims_full_graph::find_neighborhoods(
	const ims_graph_base& dig,
	const neighbors_data& nb, 
	bool from_emtpy)
{
	clear();

	

	let& arr = nb.m_data;

	//add initial neighborhoods
	for (size_t v = 0; v < dig.num_ver(); ++v) {//by vertices
		if (nb.ver_invalid(v)) {
			continue;
		}

		let t = m_lists.size();

		if (!from_emtpy) {
			for(size_t i=0;i<arr.size();++i){
				let& q = arr[i];
				if (q.s0 == v && q.inter_type_left()) {
					m_lists.emplace_back(i);
				}
			}
		}

		insert(prepare(v, t));
	}

	//m_data grows during iterations!
	for (size_t pos=0; pos < m_data.size();++pos) {

		if (need_stop()) {
			return false;
		}

		auto& ev = m_data[pos];//may be invalidated after insert
		let& e = ev.h->first;
		

		let ne = dig.num_edges(e.ver);

		//allocate space for links
		ev.idx_child = m_elem_refs.size();
		m_elem_refs.resize(ev.idx_child + ne);

		for (size_t be = 0; be < ne; ++be) {
			let st = m_lists.size();

			//fill from the attached elements
			for (size_t j = 0; j < e.sz; ++j) {
				let idx = arr[m_lists[e.idx + j]].idx + be;
				if (!nb.append_item(m_lists, idx)) {
					return false;
				}
			}

			//fill from the part itself
			for (size_t j = 0; j < ne; ++j) {
				let idx = nb.get_root_inter(e.ver, be, j);
				if (!nb.append_item(m_lists, idx)) {
					return false;
				}
			}

			
			let& h = insert(prepare(dig.get_edge(e.ver, be).second, st));
			m_elem_refs[m_data[pos].idx_child + be] = h.second;
		}

	};


	test_out << "m_hash=" << m_hash.size() << "\n";
	test_out << "m_elem_refs=" << m_elem_refs.size() << "\n";
	test_out << "m_lists=" << m_lists.size() << "\n";

	return true;
};

void ims_full_graph::get_nbh_graph(
	const ims_graph_base& dig,
	std::vector<ims_edge>& dst)
{
	dst.clear();

	for (size_t j = 0; j < m_data.size(); ++j) {
		let& ev = m_data[j];
		let& e = ev.h->first;

		let ne = dig.num_edges(e.ver);

		for (size_t i = 0; i < ne; ++i) {
			auto& edg = dst.emplace_back();
			edg.first = j;
			edg.second = m_elem_refs[ev.idx_child + i];
			edg.m = dig.get_edge(e.ver, i).m;
		}
	}
}

#if 0
void ims_full_graph::get_neighbourhoods(
	std::vector<std::vector<const elem*>>& dst,
	const ims_graph& nbh) const
{
	dst.clear();

	for (size_t j = 0; j < m_data.size(); ++j) {

		if (nbh.m_comp[nbh.m_ver2com[j]].has_other()) {
			continue;
		}

		let& e = get_elem(j);
		if (dst.size() <= e.ver) {
			dst.resize(e.ver + 1);
		};

		dst[e.ver].emplace_back(&e);
	}

	for (auto& q : arr) {
		for (size_t i = 0; i < q.size(); ++i) {
			if (!q[i])continue;
			for (size_t j = 0; j < q.size(); ++j) {
				if (i == j || !q[j])continue;
				if (ims_need_stop()) {
					return;//the algorithm is ineffective, it freezes here
				}
				if (first_in_second(*q[i], *q[j])) {
					q[i] = nullptr;
					break;
				}
			}
		}
		q.erase(std::remove(q.begin(), q.end(), nullptr), q.end());
	}
};
#endif





////////////////////////////////////////////////////////////////////////////////


void ims_full_inter::clear()
{
	ims_inter_lists::clear();
	m_edges.clear();
	m_intervals.clear();
	m_nid.clear();
	m_roots.clear();
}


ims_full_inter::interval ims_full_inter::get_interval(const size_t num)
{
	let sz = m_intervals.size();
	
	interval r;
	if (num < sz) {
		r.first = m_intervals[num];
		if (num + 1 < sz) {
			r.second = m_intervals[num + 1];
		} else {
			r.second = m_data.size();
		}
	} else {
		r = { 0,0 };
	}

	return r;
}


ims_inter_lists::HashTable::value_type*
ims_full_inter::insert_ex(size_t ver, size_t idx)
{
	auto e = prepare(ver, idx);

	//testing a slightly shorter one
	//this optimization reduced memory consumption for Levy Curve by more than 100 times
	if (e.sz > 2) {
		--e.sz;
		if (!exists(e)){
			m_lists.resize(idx);
			return nullptr;
		}
		++e.idx;
		if (!exists(e)) {
			m_lists.resize(idx);
			return nullptr;
		}
		--e.idx;
		++e.sz;
	}
	return &insert(e);
}


size_t ims_full_inter::num_edges(size_t v) const
{
	return m_roots[v].nedg;
};

size_t ims_full_inter::num_child(const data_el& ev) const
{
	//total number of children
	size_t ne = 0;
	let nge = num_edges(ev.h->first.ver);
	for (size_t k = 0; k < nge; ++k) {
		ne += m_edges[ev.idx_child + k].sz3;
	}
	return ne;
}
size_t ims_full_inter::num_ver() const
{
	return m_roots.size();
}

size_t ims_full_inter::target_ver(size_t v, size_t ve) const
{
	return m_elem_refs[m_data[v].idx_child + ve];
};



const ims_full_inter::edge&
ims_full_inter::get_root_inter(size_t ver, size_t ei, size_t ej) const
{
	let& q = m_roots[ver];
	return m_edges[q.root + q.nedg * ei + ej];
}


size_t ims_full_inter::calc_inters(const size_t max_num)
{
	

	if (m_intervals.size() != 2) {
		return 0;
	}

	size_t sz3 = m_data.size();//from here the intersections of 3 begin

	for (size_t num = 2; num < max_num; ++num) {
		let sz = m_data.size();


		test_out << "-------------\n";
		test_out << "num=" << num << " " << m_data.size() << " " << m_lists.size() << "\n";

		if (!init()) {
			return 0;
		}

		test_out << "init=" << m_data.size() << " " << m_lists.size() << "\n";

		if (!step(sz)) {
			return 0;
		};

		test_out << "step=" << m_data.size() << " " << m_lists.size() << "\n";

		if (!clear_empty(sz)) {
			return 0;
		};

		if (m_data.size() > sz) {
			m_intervals.emplace_back(sz);
		}


		test_out << "clear=" << m_data.size() << " " << m_lists.size() << "\n";


#if 0
		{
			size_t se1 = 0;
			for (let& q : m_lists)if (q == ims_max)++se1;
			size_t se2 = 0;
			for (let& q : m_hash)se2 += q.first.sz;
			assert(se1 + se2 == m_lists.size());
			test_out << "m_lists(e1==e2)=" << " " << se1 << "==" << m_lists.size() - se2 << "\n";
		}
#endif

		if (m_data.size() == sz) {
			break;//completed
		}

#ifndef NDEBUG
		for (size_t i = sz; i < m_data.size(); ++i) {
			auto e(m_data[i].h->first);
			e.sz--;
			if (!exists(e)) {
				std::cout << "fail1=" << i << std::endl;
				assert(false);
			}
			++e.idx;
			if (!exists(e)) {
				std::cout << "fail2=" << i << std::endl;
				assert(false);
			}
		}
#endif

	}
	return m_data.size() - sz3;
}


void ims_full_inter::init0(const ims_graph_base& dig)
{
	clear();

	let nv = dig.num_ver();

	////////////////////////////////////////////////////////////////////////////
	//intersections by 1 (the set itself)
	m_intervals.emplace_back(m_data.size());
	for (size_t v = 0; v < nv; ++v) {//by vertices

		elem e;
		e.ver = v;
		e.idx = ims_max;//doesn't matter
		e.sz = 0;
		insert(e);
	}

	//intersections by 1 - add children
	m_roots.resize(nv);
	for (size_t i = 0; i < nv; ++i) {
		auto& s = m_data[i];

		let v = s.h->first.ver;

		let ne = dig.num_edges(v);
		m_roots[i].nedg = ne;

		let start_idx = m_edges.size();
		assert(m_elem_refs.size() == start_idx);

		m_edges.resize(start_idx + ne);
		m_elem_refs.resize(start_idx + ne);

		for (size_t ve = 0; ve < ne; ++ve) {

			let vt = dig.get_edge(v, ve).second;

			auto& lr = m_edges[start_idx + ve];
			lr.idx3 = start_idx + ve;//start
			lr.sz3 = 1;//size 

			m_elem_refs[lr.idx3] = vt;
		}

		s.idx_child = start_idx;
	}
};


void ims_full_inter::init1(const neighbors_data& nb)
{

	let& arr = nb.m_data;

	if (arr.empty()) {
		return;
	}

	for (let& q : arr) {
		if (q.res == inter_type::overlapped) {
			assert(false);
			return;
		}
	}

	////////////////////////////////////////////////////////////////////////////
	//intersections by 2
	m_intervals.emplace_back(m_data.size());
	
	m_nid.resize(m_data.size(), ims_max);
	m_idxs.resize(arr.size());//translates nid into an internal address
	for (size_t i = 0; i < arr.size(); ++i) {
		let& q = arr[i];
	
		if (!q.inter_type_left())continue;

		m_idxs[i] = m_data.size();

		m_lists.emplace_back(m_data.size());
		

		elem e;
		e.ver = q.s0;
		e.idx = m_lists.size() - 1;
		e.sz = 1;
		insert(e);
		

		m_nid.emplace_back(i);
	}

	let nv = num_ver();

	//intersections of 2 - add children
	for (index_t i = m_intervals[1]; i < m_data.size(); ++i) {
		auto& s = m_data[i];
		let& e = s.h->first;

		let v = e.ver;

		let ne = num_edges(v);
		let start_idx = m_edges.size();

		m_edges.resize(start_idx + ne);

		
		let idx = arr[m_nid[m_lists[e.idx]]].idx;


		for (size_t ve = 0; ve < ne; ++ve) {

			let list_sz = m_lists.size();
			nb.append_item(m_lists, idx + ve);

			auto& lr = m_edges[start_idx + ve];
			lr.idx3 = m_elem_refs.size();//start
			lr.sz3 = m_lists.size() - list_sz;//size 

			m_elem_refs.resize(m_elem_refs.size() + lr.sz3);

			for (size_t j = 0; j < lr.sz3; ++j) {
				m_elem_refs[lr.idx3 + j] = m_idxs[m_lists[list_sz + j]];
			}

			m_lists.resize(list_sz);//restore

		}

		s.idx_child = start_idx;
	};


	//root intersections
	for (size_t v = 0; v < nv; ++v) {

		if (nb.ver_invalid(v)) {
			continue;
		}

		let ne = num_edges(v);

		let vidx = m_edges.size();
		m_roots[v].root = vidx;
		m_edges.resize(vidx + ne * ne);


		for (size_t i = 0; i < ne; ++i) {
			for (size_t j = 0; j < ne; ++j) {

				

				let list_sz = m_lists.size();
				nb.append_item(m_lists, nb.get_root_inter(v, i, j));

				auto& lr = m_edges[vidx + ne * i + j];
				lr.idx3 = m_elem_refs.size();
				lr.sz3 = m_lists.size() - list_sz;

				m_elem_refs.resize(m_elem_refs.size() + lr.sz3);

				for (size_t k = 0; k < lr.sz3; ++k) {
					m_elem_refs[lr.idx3 + k] = m_idxs[m_lists[list_sz + k]];
				}

				m_lists.resize(list_sz);//restore
			}
		}

	}
}


bool ims_full_inter::init()
{
	index_t to = m_data.size();

	let num = m_intervals.size();
	assert(num >= 2);

	

	//divide each one once to create an intersection by num
	for (size_t i = 0; i < to; ++i) {

		let& e = m_data[i].h->first;

		let ne = num_edges(e.ver);

		let N = e.sz;

		m_idxs.resize(N + 1);

		for (size_t be = 0; be < ne; ++be) {

			let vt = target_ver(e.ver, be);

			//generate lists
			m_temp_temp.clear();

			auto& s = m_comb2.m_state;
			s.resize(N + 1);

			bool is_empty = false;

			//add from the attached parts
			for (size_t j = 0; j < N; ++j) {
				m_idxs[j] = m_temp_temp.size();

				let& sx = m_data[m_lists[e.idx + j]];
				let& lr = m_edges[sx.idx_child + be];
				for (size_t k = 0; k < lr.sz3; ++k) {
					m_temp_temp.emplace_back(m_elem_refs[lr.idx3 + k]);
				}
				

				//question - how to quickly find m_lists[e.idx + j] in m_data
				//to understand what it breaks down into...
				//you can use nid
				//let m_lists store the pairwise intersection indices in m_data

				s[j].max_cur = m_temp_temp.size() - m_idxs[j];

				if (s[j].max_cur == 0) {
					is_empty = true;
					break;
				}
				--s[j].max_cur;//one always has to be selected
			}

			if (is_empty)continue;

			//add from the part itself
			m_idxs[N] = m_temp_temp.size();
			for (size_t j = 0; j < ne; ++j) {

				let& lr= get_root_inter(e.ver, be, j);
				for (size_t k = 0; k < lr.sz3; ++k) {
					m_temp_temp.emplace_back(m_elem_refs[lr.idx3 + k]);
				}
			}
			s[N].max_cur = m_temp_temp.size() - m_idxs[N];//there may be 0 here

			//divide it in every possible way to select 0 or more
			//elements from each, num in total

			if (num < N)continue;


			if (!m_comb2.init(num - N)) {
				continue;
			}

			auto& s3 = m_comb3.m_settings;

			do {

				if (s[N].v > 0) {
					s3.resize(N + 1);
					s3[N].num = s[N].v;
					s3[N].max = s[N].max_cur;
				} else {
					s3.resize(N);
				}

				for (size_t k = 0; k < N; ++k) {
					s3[k].num = s[k].v + 1;
					s3[k].max = s[k].max_cur + 1;
				}

				m_comb3.init();

				size_t next_group = 0;

				do {

					if (need_stop()) {
						return false;
					}

					let bg = m_lists.size();//remember where we started inserting
					let& c3 = m_comb3.m_state;

					for (size_t j = 0; j < s3.size(); ++j) {
						auto& p = s3[j];

						next_group = j;

						for (size_t m = 0; m < p.num; ++m) {
							m_lists.emplace_back(m_temp_temp[m_idxs[j] + c3[p.idx + m]]);
						}

						//this optimization speeds up the calculation for Levy Curve 33 sec -> 7.5 sec
						if (j + 1 < s3.size() && m_temp_temp.size() - bg > 1) {
							if (!exists(prepare(vt, bg))) {
								break;
							}
						}
					}

					if (next_group + 1 < s3.size()) {
						m_lists.resize(bg);
						continue;
					}

					assert(m_lists.size() == bg + c3.size());//happens with overlapping

					insert_ex(vt, bg);
					//std::cout << "++ " << m_hash.size() << " " << m_lists.size() << std::endl;
				} while (m_comb3.next_from(next_group));
			} while (m_comb2.next());
		}
	}


	return true;
}


bool ims_full_inter::step(index_t from)
{
	//m_data changes during iterations - references are unstable
	for (; from < m_data.size();++from) {

		if (need_stop()) {
			return false;
		}

		let& e = m_data[from].h->first;
	
		let ne = num_edges(e.ver);

		//allocate space for links in m_edges

		let start_idx = m_edges.size();
		size_t nc = 0;

		m_edges.resize(start_idx + ne);

		for (size_t be = 0; be < ne; ++be) {

			let vt = target_ver(e.ver, be);

			auto& lr = m_edges[start_idx + be];
			lr.idx3 = m_elem_refs.size();//start
			lr.sz3 = 0;//size

			//generate lists - fill them up from the appended parts
			m_comb.m_state.resize(e.sz);
			m_temp_temp.clear();

			bool is_empty = false;

			for (size_t j = 0; j < e.sz; ++j) {
				auto& cs = m_comb.m_state[j];
				cs.v = cs.b = m_temp_temp.size();


				let& sx = m_data[m_lists[e.idx + j]];
				let& lx = m_edges[sx.idx_child + be];
				for (size_t k = 0; k < lx.sz3; ++k) {
					m_temp_temp.emplace_back(m_elem_refs[lx.idx3 + k]);
				}

				cs.e = m_temp_temp.size();//has already increased
				if (cs.b == cs.e) {
					is_empty = true;
					break;
				}
			}

			if (is_empty)continue;

			//add all possible combinations, one from each group m_temp_temp
			do {

				let st = m_lists.size();//remember where we started inserting

				for (size_t j = 0; j < e.sz; ++j) {
					m_lists.emplace_back(m_temp_temp[m_comb.m_state[j].v]);
				}

				let* h = insert_ex(vt, st);
				if (!h)continue;

				m_elem_refs.emplace_back(h->second);
				++lr.sz3;

			} while (m_comb.next());

			nc += lr.sz3;
		}


		auto& ev = m_data[from];
		if (nc > 0) {
			ev.idx_child = start_idx;
		} else {
			ev.idx_child = ims_max;
			m_edges.resize(start_idx);//roll back
		}
		
	};

	return true;
}


bool ims_full_inter::clear_empty(index_t from)
{
	let start_idx = from;
	
	m_idxs.clear();
	////////////////////////////////////////////////////////////////////////
	//depth-first traversal to finally get intersections
	for (index_t i = from; i < m_data.size(); ++i) {
		m_data[i].next_edge = 0;//indication that it has not been processed yet
	}


	for (index_t i = from; i < m_data.size(); ++i) {
		auto& s = m_data[i];

		//if empty or already checked/being checked - skip
		if (s.idx_child == ims_max || s.next_edge > 0) {
			continue;
		}


		//new candidate for testing
		m_idxs.push_back(i);
		while (!m_idxs.empty()) {

			if (need_stop()) {
				return false;
			}

			let cur_depth = m_idxs.size();

			auto& ev = m_data[m_idxs.back()];
	
			assert(ev.idx_child != ims_max);//happens when overlapped

			//the very first element of all children
			auto* child_list = &m_elem_refs[m_edges[ev.idx_child].idx3];

			//total number of children
			let ne = num_child(ev);
			
			//search for the next non-empty child
			auto& ediv = ev.next_edge;
			while (ediv < ne) {
				let idx = child_list[ediv++];
				if (idx!=ims_max) {
					let& c = m_data[idx];
					if (c.idx_child != ims_max && c.next_edge == 0){
						assert(idx >= from);
						m_idxs.emplace_back(idx);
						break;
					}
				}
			} 


			if (cur_depth < m_idxs.size()) {
				continue;//found a child, let's go deeper
			}

			assert(ediv == ne);
			//check all child elements, we can find the intersection
			//we'll also remove references to empty ones
			assert(ev.idx_child != ims_max);
			bool emp = true;
			for (size_t j = 0; j < ne; ++j) {
				auto& ref = child_list[j];
				if (ref != ims_max) {
					if (m_data[ref].idx_child == ims_max) {
						ref = ims_max;
					} else {
						emp = false;
					}
				}

			}
			if (emp) {//divided into empty
				ev.idx_child = ims_max;
			}

			m_idxs.pop_back();
			
		}
	}

	////////////////////////////////////////////////////////////////////////////
	//compact it
	
	//renumbering table
	m_idxs.clear();
	
	for (index_t i = from; i < m_data.size(); ++i) {
		auto& s = m_data[i];
		auto& hs = s.h->second;

		if (s.idx_child == ims_max) {
			hs = ims_max;
			continue;
		}

		if (m_idxs.size() <= hs) {
			m_idxs.resize(hs + 1, ims_max);
		}
		m_idxs[hs] = from;

		hs = from;
		m_data[from++] = s;
	}
	m_data.resize(from);

	//renumber and order m_elem_ref
	//excludes ims_max from m_edges lists
	for (index_t i = start_idx; i < from; ++i) {
		let& ev = m_data[i];
		
		let ne = num_edges(ev.h->first.ver);
		assert(ne > 0);
		for (size_t j = 0; j < ne; ++j) {
			auto& lr = m_edges[ev.idx_child + j];

			size_t nsz = 0;
			for (size_t k = 0; k < lr.sz3; ++k) {
				auto q = m_elem_refs[lr.idx3 + k];
				if (q == ims_max)continue;
				if (q >= start_idx)q = m_idxs[q];//new - renumber
				m_elem_refs[lr.idx3 + nsz] = q;
				++nsz;
				
			}
			lr.sz3 = nsz;
		}
	}


	//clean the hash
	for (auto it = std::begin(m_hash); it != std::end(m_hash);) {
		if (it->second == ims_max) {
			auto i = m_lists.begin() + it->first.idx;
			let e = i + it->first.sz;
			it = m_hash.erase(it);
			std::fill(i, e, ims_max);//key modification - at the very end
		} else {
			++it;
		}
	}

	//TODO - you could also collapse m_edges, m_elem_refs, m_lists
	//but experiments show that there isn't much extra in them (2.5% for Levy Curve)

	return true;

};



void ims_full_inter::get_graph_x(
	std::vector<ims_edge>& dst,
	const ims_graph_base& dig,
	const size_t num)
{
	dst.clear();

	let r = get_interval(num);

	for (size_t i = r.first; i < r.second; ++i) {

		let& ev = m_data[i];
		let& e = ev.h->first;

		let ne = num_edges(e.ver);

		let vs = i - r.first;

		for (size_t be = 0; be < ne; ++be) {
			let m = dig.get_edge(e.ver, be).m;

			let& lr = m_edges[ev.idx_child + be];

			for (size_t j = 0; j < lr.sz3; ++j) {
				auto& de = dst.emplace_back();
				de.first = vs;
				de.second = m_elem_refs[lr.idx3 + j] - r.first;
				de.m = m;
			};

		}
	}
}

void ims_full_inter::get_con_graph(std::vector<ims_edge>& dst, index_t eidx)
{
	dst.clear();

	let& ev = m_data[eidx];
	let& e = ev.h->first;

	let v = e.ver;

	let ne = num_edges(v);

	let orig_len = m_lists.size();

	for (size_t i = 0; i < ne; ++i) {

		for (size_t j = i + 1; j < ne; ++j) {
			
			let* sij = &get_root_inter(v, i, j);
			if (sij->sz3 == 0)continue;
			
			size_t qi, qj;

			if (sij->sz3==1) {
				qi = i;
				qj = j;
			} else {
				qi = j;
				qj = i;

				sij = &get_root_inter(v, j, i);
				assert(sij->sz3==1);
			};

			let rj = m_elem_refs[sij->idx3];

			let vt = target_ver(v, qi);

			//at least one intersection by 3 is not empty
			bool not_empty = false;

			//list of intersections in which part qi participates
			let& lr = m_edges[ev.idx_child + qi];

			//loop through all intersections
			for (size_t k = 0; k < lr.sz3; ++k) {
				let idx= m_elem_refs[lr.idx3 + k];
				
				let& lst = m_data[idx].h->first;

				for (size_t m = 0; m < lst.sz; ++m) {
					let& le = m_lists[lst.idx + m];
					m_lists.emplace_back(le);
				}

				m_lists.emplace_back(rj); //fi^-1*fj

				not_empty = exists(prepare(vt, orig_len));

				m_lists.resize(orig_len);//restore

				if (not_empty) {
					break;
				}
			};

			if (not_empty) {
				{
					auto& edg = dst.emplace_back();
					edg.first = qi;
					edg.second = qj;
					edg.m = 0;
				}

				{
					auto& edg = dst.emplace_back();
					edg.first = qj;
					edg.second = qi;
					edg.m = 0;
				}
			}
		};
	};


}