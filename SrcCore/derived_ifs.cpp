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
#include "derived_ifs.h"
#include "neighbors_data.h"
#include "oper_block.h"
#include "ims_full_graph.h"
#include "block_class.h"
#include "ims_graph.h"
#include "clock_print.h"
#include "graph_init_data_ptr.h"
#include "ims_identifiers.h"
#include "block_graph.h"
#include "eval_context.h"
#include "variable.h"

bool create_neghbours(
	ims_identifiers& idf,
	oper_block& dst,
	const oper_block& src,
	neighbors_data& nb,
	const ims_graph& dig,
	const report_params* rp,
	const dim_filter_type& filter_func
)
{
	test_clock_print clock("create_neghbours= ");

	const std::string 
		eva_prefix = "q",
		map_prefix = "k",
		lab_prefix = "e",
		ver_prefix = "v",
		nbm_prefix = "m",
		rel_prefix = "id_",
		nbi_prefix = "i",
		nbu_prefix = "u",
		fi_prefix = "j",//intersections by 3,4...
		fu_prefix = "w";//unions by 3,4...


#ifndef NDEBUG
	size_t num_neighbours = 0;
	for (let& q : nb.m_data) {
		if (q.idx_graph != ims_max)++num_neighbours;
	}
	assert(num_neighbours > 0);
#endif


	let* srcG = src.get_class();
	let& bg = *src.get_graph();
	let& ec = *src.ctx();
	let& ms = bg.m_am.m_ixm.m_maps;
	let nm = ms.size();

	//create a mapping:
	//each graph node has a user operator number, or ims_max
	struct user_ref
	{
		//is_user==true: index of the user operator in the graph
		//is_user==false: index of the additionally added operator
		size_t idx;
		//vertex in the graph
		size_t ver;
		bool is_user() const { return ver != ims_max; };
	};

	std::vector<user_ref> user_refs;
	user_refs.resize(dig.num_ver());
	for (auto& q : user_refs) {
		q.ver = ims_max;
	}

	//size_t num_user = 0;


	for (size_t idx = 0; idx < ec.m_refs5.size(); ++idx) {//by all user references

		let ver = bg.ref2fg(idx);
		if (ver == ims_max)continue;


		auto& h = user_refs[ver];
		h.idx = idx;
		h.ver = ver;
		//++num_user;
	}

	//how many additional vertices do we add?
	size_t nuser_ver = 0;
	for (auto& q : user_refs) {
		if (q.is_user())continue;
		q.idx = nuser_ver++;
	}


	ims_graph tgraph;//for temporary needs
	////////////////////////////////////////////////////////////
	ims_full_graph fgc;
	ims_graph  fgc_G;

	graph_init_data_ptr idata2;

	std::vector<bool> used_vers;//vertices used during filtering

	//stores indices of the required neighborhoods (global, local)
	std::vector<std::pair<size_t, size_t>> nbh_sort[2];
	std::vector<size_t> nbh_sort_inv[2];

	bool nbx[2] = {
		!rp || rp->neighbourhoods || rp->neighbourhoods_graph,
		rp && rp->nboundary
	};


	if (nbx[0] || nbx[1]) {

		bool res;
		{
			test_clock_print clock2("find_neighborhoods= ");
			res = fgc.find_neighborhoods(dig, nb, nbx[1]);
		}

		if (res) {
			fgc.get_nbh_graph(dig, fgc_G.m_edges);

			fgc_G.init(idata2.get());

			for (size_t i = 0; i < 2; ++i) {
				if (!nbx[i])continue;

				auto& ns = nbh_sort[i];
				auto& nsi = nbh_sort_inv[i];

				ns.clear();
				for (size_t j = 0; j < fgc_G.num_ver(); ++j) {
					let ho = fgc_G.m_comp[fgc_G.m_ver2com[j]].has_other();
					if (i == 0 && ho || i == 1 && !ho) {
						continue;
					}

					ns.emplace_back(j, ims_max);
				}


				std::stable_sort(ns.begin(), ns.end(), [&](let& i1, let& i2) {
					let q1 = user_refs[fgc.get_elem(i1.first).ver].idx;
					let q2 = user_refs[fgc.get_elem(i2.first).ver].idx;
					return q1 < q2;
					});

				nsi.resize(fgc_G.num_ver());
				std::fill(nsi.begin(), nsi.end(), ims_max);

				std::vector<size_t> next_idx(dig.num_ver(), 0);
				for (size_t k = 0; k < ns.size(); ++k) {
					auto& nsk = ns[k];
					let idx = nsk.first;
					nsk.second = next_idx[fgc.get_elem(idx).ver]++;
					nsi[idx] = k;
				}

				if (i == 0)continue;

				tgraph.clear();

				for (size_t k = 0; k < ns.size(); ++k) {
					let& nsk = ns[k];
					let v = nsk.first;
					let ne = fgc_G.num_edges(v);

					for (size_t j = 0; j < ne; ++j) {
						let& e = fgc_G.get_edge(v, j);
						if (nsi[e.second] == ims_max)continue;
						tgraph.create_edge(k, nsi[e.second], e.m);
					}
				}


				//filter connections by 2, 3, 4...
				tgraph.init(idata2.get());
				filter_func(used_vers, rp->filer_post, tgraph);
				if (ims_need_stop()) {
					return false;
				}



				for (size_t k = 0; k < ns.size(); ++k) {
					auto& nsk = ns[k];
					let idx = nsk.first;
					if (!used_vers[k]) {
						ns[k].first = ims_max;
						nsi[idx] = ims_max;
					}
				}

			}

		}
	}

	ims_full_inter fi;
	size_t inter_all_num = 0;
	size_t inter_from = 2;

	if (rp && rp->max_inter > 2 && (rp->connections || rp->intersections)) {
		fi.init0(dig);
		fi.init1(nb);

		{
			test_clock_print clock2("calc_inters time= ");
			inter_all_num = fi.calc_inters(rp->max_inter);
		}

		if (inter_all_num > 0 && rp->only_max_inters) {

			inter_from = fi.num_intervals() - 1;
			assert(inter_from >= 2);
			let q = fi.get_interval(inter_from);
			inter_all_num = q.second - q.first;
		}
	}

	////////////////////////////////////////////////////////////

	dst.clear();
	//leave all flags cleared!

	dst.set_parent(&src);
	dst.m_class.reset();
	dst.m_flags.ready = false;
	
	

	bool con_by_2 = rp && rp->connections;
	bool int_by_2 = !rp || rp->intersections;

	if (rp && rp->only_max_inters && inter_all_num > 0) {
		con_by_2 = false;
		int_by_2 = false;
	}


	bool use_nmaps = rp && (rp->connections || rp->neighbourhoods);

	

	ankerl::unordered_dense::set<size_t> used_vars;
	std::vector<size_t> var2unk;


	auto ref2str = [&](size_t ref)
	{
		return std::string{ idf.get_str_from_unk(var2unk[ref]) };
	};

	for (let& q : srcG->m_refs) {
		used_vars.emplace(q.unk_id);
		var2unk.emplace_back(q.unk_id);
	}


	auto get_uniq_var = [&](std::string_view prefix)
	{
		assert(!prefix.empty());
		std::string str{ prefix };

		for (size_t i = 1;; ++i) {

			let unk_id = idf.get_unk_id(str);
			auto res = used_vars.emplace(unk_id);
			if (res.second) {
				var2unk.emplace_back(unk_id);
				return unk_id;
			}
			str = prefix;
			str += std::to_string(i);
		}
	};


	let was = srcG->m_refs.size();
	size_t next_var = was;
	auto make_var = [&](
		uint32_t& prev,
		std::string_view name,
		bool is_subs = false)
	{
		let unk_idx = get_uniq_var(name);
		++next_var;
		return dst.add_var(prev, unk_idx, is_subs);
	};

	
	auto make_ref = [&](size_t ops_idx, size_t ref)
	{
		dst.m_ops[ops_idx].hdr.set_reference(ref);	
	};

	dst.m_name = rp ? "custom" : "boundary";

	dst.clear_ops();
	uint32_t pos = 0;


	//at the very beginning - what was additionally calculated
	for (size_t i = was; i < ec.m_refs5.size(); ++i) {
		let ds = make_var(pos, eva_prefix + std::to_string(i - was + 1));
		dst.insert_op_ex(ds, ec.m_refs5[i].c, ec);
	}

	//start adding additional variables



	let start_maps = next_var;
	for (size_t i = 0; i < nm; ++i) {

		let ds = make_var(pos, map_prefix + std::to_string(i + 1));

		let& a = ms[i];

		if (a.num == 0) {
			dst.m_ops[ds].hdr.set_id();
		} else if (a.num == 1) {
			dst.insert_op_ex(ds, bg.m_am.get_graph_atom(a.start), ec);
		} else {
			let na = dst.add_args(ds, ETYPE::mul, a.num);
			for (size_t j = 0; j < a.num; ++j) {
				dst.insert_op_ex(na + j, bg.m_am.get_graph_atom(j + a.start), ec);
			}
		}
	}

	//identity maps as labels
	if (use_nmaps || (rp && rp->intersections)) {
		for (size_t i = 0; i < nm; ++i) {
			let ds = make_var(pos, lab_prefix + std::to_string(i + 1));
			dst.m_ops[ds].hdr.set_id();
		}
	}

	let start_sets = next_var;
	//additional sets
	for (size_t v = 0; v < user_refs.size(); ++v) {
		let& q = user_refs[v];
		if (q.is_user())continue;

		let ds = make_var(pos, ver_prefix + std::to_string(q.idx + 1));

		let ne = dig.num_edges(v);

		//union elements
		let ofs = dst.add_args(ds, ETYPE::uni, ne);

		for (size_t j = 0; j < ne; ++j) {
			let& e = dig.get_edge(v, j);

			assert(e.m < nm);

			let a = dst.add_args(ofs + j, ETYPE::mul, 2);
			
			make_ref(a, e.m + start_maps);

			let& uvt = user_refs[e.second];
			if (uvt.is_user()) {
				make_ref(a + 1, uvt.idx);
			} else {
				make_ref(a + 1, start_sets + uvt.idx);
			}
		}
	}

	
	let start_neighbours1 = next_var;
	if (!rp || int_by_2) {

		ims_graph boundary;
		//Caution! Uses idx_graph
		nb.create_boundary(dig, boundary, rp? nm: 0);

		boundary.init(idata2.get());

		for (size_t j = 0; j < nb.m_data.size(); ++j) {
			let& cn = nb.m_data[j];
			let idx_graph = cn.idx_graph;
			if (idx_graph>=boundary.num_ver())continue;
			
			let ds = make_var(pos,
				nbi_prefix + std::to_string(idx_graph + 1),
				!cn.inter_type_left());


			let ne = boundary.num_edges(idx_graph);

			if (ne == 0) {
				dst.add_args(ds, ETYPE::empty, ne);
			} else {
				//union elements
				let ofs = dst.add_args(ds, ETYPE::uni, ne);

				for (size_t k = 0; k < ne; ++k) {
					let& e = boundary.get_edge(idx_graph, k);

					let a = dst.add_args(ofs + k, ETYPE::mul, 2);

					let vti = e.second + start_neighbours1;
					let mi = e.m + start_maps;

					if (e.m < nm) {
						make_ref(a, mi);
						make_ref(a + 1, vti);
					} else {//lab_prefix
						make_ref(a, vti);
						let pa = dst.set_power(a + 1, -1);
						make_ref(pa, mi);
					}
				}
			}
		}
	}


	//neighbor maps
	let start_neighbours2 = next_var;

	bool use_relators = rp && rp->relators;
	if (use_nmaps || use_relators) {
		std::vector<size_t> mapf, mapr;

		std::vector<neighbors_data::neghbour_map> nbm;

		neighbors_data::relators rel;

		nb.get_neighbor_maps(nbm, use_relators?&rel:nullptr, dig);

		if (use_nmaps) {
			for (size_t j = 0; j < nbm.size(); ++j) {
				let& cn = nb.m_data[j];
				let idx_graph = cn.idx_graph;
				if (ims_max == idx_graph)continue;

				let ds = make_var(pos,
					nbm_prefix + std::to_string(idx_graph + 1), 
					!cn.inter_type_left());

				let& h = nbm[j];

				mapf.clear();
				mapr.clear();

				if (ims_max != h.f)mapf.emplace_back(h.f);
				if (ims_max != h.r)mapr.emplace_back(h.r);

				size_t pidx = h.par;
				while (ims_max != pidx) {
					//if (ims_max != nb.m_data[pidx].idx_graph)break;

					let& ph = nbm[pidx];
					if (ims_max != ph.f)mapf.emplace_back(ph.f);
					if (ims_max != ph.r)mapr.emplace_back(ph.r);

					pidx = ph.par;
				};
				std::reverse(mapr.begin(), mapr.end());
				std::reverse(mapf.begin(), mapf.end());

				///////////////////////////////////////////////////////////////////

				//composition elements r^-1*?*f
				let num_mul =
					mapf.size() +
					(mapr.empty() ? 0 : 1) +
					(ims_max == pidx ? 0 : 1);

				auto mul_arg = dst.add_args(ds, ETYPE::mul, num_mul);

				if (!mapr.empty()) {
					size_t pa = dst.set_power(mul_arg++, -1);

					if (mapr.size() == 1) {
						make_ref(pa, start_maps + mapr.front());
					} else {
						auto a = dst.add_args(pa, ETYPE::mul, mapr.size());
						for (let mi : mapr) {
							make_ref(a++, start_maps + mi);
						}
					}
				}

				if (ims_max != pidx) {
					let pver = nb.m_data[pidx].idx_graph;
					make_ref(mul_arg++, start_neighbours2 + pver);
				}

				for (let mi : mapf) {
					make_ref(mul_arg++, start_maps + mi);
				}
			}

		}
	
		if (use_relators) {
			for (size_t j = 0; j < rel.m_data.size(); ++j) {

				let& rj = rel.m_data[j];

				let ds = make_var(pos, rel_prefix + std::to_string(j + 1));

				///////////////////////////////////////////////////////////////////

				//composition elements r^-1*?*f
				let num_mul = rj.prod.size();
				assert(num_mul > 0);

				auto mul_arg = dst.add_args(ds, ETYPE::mul, num_mul);

				for (size_t i = 0; i < rj.prod.size(); ++i) {
					auto v = rj.prod[i];
					size_t pa = mul_arg++;
					if (v < 0) {
						v = -v;
						pa = dst.set_power(pa, -1);
					}
					let mi = size_t(v) - 1;
					make_ref(pa, start_maps + mi);
				}


				
			}
		}
	}

	if (rp && con_by_2) {

		//pairwise unions
		for (size_t j = 0; j < nb.m_data.size(); ++j) {
			let& cn = nb.m_data[j];
			let idx_graph = cn.idx_graph;
			if (ims_max == idx_graph)continue;


			let ds = make_var(pos, 
				nbu_prefix + std::to_string(idx_graph + 1), 
				!cn.inter_type_left());

			let& v0 = user_refs[cn.s0];
			let& v1 = user_refs[cn.s1];

			//union elements
			let ofs = dst.add_args(ds, ETYPE::uni, 2);
			make_ref(ofs, v0.idx);
			let a = dst.add_args(ofs + 1, ETYPE::mul, 2);
			make_ref(a, idx_graph + start_neighbours2);
			make_ref(a + 1, v1.idx);
		}
	}

	////////////////////////////////////////////////////////////////////////
	
	if (inter_all_num > 0) {
		for (size_t inum = inter_from;; ++inum) {

			let r = fi.get_interval(inum);
			if (r.first == r.second) {
				break;
			}

			if (rp->intersections) {
				fi.get_graph_x(tgraph.m_edges, dig, inum);
			
				tgraph.init(idata2.get());//finds components of dimension 0

				//filter intersections by 3,4,...
				if (rp->only_strong_intres) {
					tgraph.remove_non_strong_edges(idata2.get());
				}

				filter_func(used_vers, rp->filer_post, tgraph);
				if (ims_need_stop()) {
					return false;
				}
				

				assert(tgraph.num_ver() == r.second - r.first);

				std::string ipref = fi_prefix + std::to_string(inum + 1) + "_";


				let start_fi = next_var;


				for (size_t j = r.first; j < r.second; ++j) {

					let v = j - r.first;
				

					
					let ds = make_var(pos, ipref + std::to_string(v + 1));

					if (!used_vers[v]) {
						dst.add_args(ds, ETYPE::empty, 0);
					} else {
						let ne = tgraph.num_edges(v);

						size_t real_ne = 0;
						for (size_t k = 0; k < ne; ++k) {
							let& e = tgraph.get_edge(v, k);
							if (!used_vers[e.second])continue;
							++real_ne;
						}

						//union elements
						let ofs = dst.add_args(ds, ETYPE::uni, real_ne);

						size_t edge_idx = 0;

						for (size_t k = 0; k < ne; ++k) {
							let& e = tgraph.get_edge(v, k);
							if (!used_vers[e.second])continue;


							let a = dst.add_args(ofs + edge_idx, ETYPE::mul, 2);
							++edge_idx;

							make_ref(a, e.m + start_maps);

							let op_idx = e.second;
							make_ref(a + 1, op_idx + start_fi);
						}
					}
				}
			}



			if (rp->connections) {
				std::string upref = fu_prefix + std::to_string(inum + 1) + "_";

				for (size_t j = r.first; j < r.second; ++j) {

					
					let ds = make_var(pos, upref + std::to_string(j - r.first + 1));

					let& e = fi.get_elem(j);

					let& v0 = user_refs[e.ver];

					//union elements
					let ofs = dst.add_args(ds, ETYPE::uni, e.sz + 1);
					make_ref(ofs, v0.idx);

					for (size_t k = 0; k < e.sz; ++k) {

						let& h = nb.m_data[fi.get_inter(e.idx + k)];
						assert(h.s0 == e.ver);

						let& v1 = user_refs[h.s1];

						let a = dst.add_args(ofs + k + 1, ETYPE::mul, 2);
						make_ref(a, h.idx_graph + start_neighbours2);
						make_ref(a + 1, v1.idx);
					}
				}
			}
		}
	}

	for (size_t i = 0; i < 2; ++i) {
		if (!rp)continue;
		if (i == 0 && !rp->neighbourhoods_graph)continue;
		if (i == 1 && !rp->nboundary)continue;

		let& nsi = nbh_sort_inv[i];

		let start_nbh = next_var;

		let& gm = fgc_G;

		for (let& nsk : nbh_sort[i]) {

			let v = nsk.first;
			
			let vs_remap = nsk.second;



			if (v == ims_max) {
				let ds = make_var(pos, "e");
				dst.add_args(ds, ETYPE::empty, 0);
			}else{
				let& q = user_refs[fgc.get_elem(v).ver];
				let str_id = ref2str(q.idx) + (i == 0 ? "_" : "_b") + std::to_string(vs_remap + 1);

				let ds = make_var(pos, str_id);

				let ne = gm.num_edges(v);

				size_t real_ne = 0;
				for (size_t k = 0; k < ne; ++k) {
					let& e = gm.get_edge(v, k);
					if (nsi[e.second] != ims_max)++real_ne;
				}

				//union elements
				let ofs = dst.add_args(ds, ETYPE::uni, real_ne);

				size_t idx_e = 0;
				for (size_t k = 0; k < ne; ++k) {
					let& e = gm.get_edge(v, k);
					let& vt_remap = nsi[e.second];
					if (vt_remap == ims_max)continue;
					let a = dst.add_args(ofs + idx_e, ETYPE::mul, 2);
					make_ref(a, e.m + start_maps);
					make_ref(a + 1, vt_remap + start_nbh);
					++idx_e;
				}
			}
		}
		
	}
	

	if (!rp || rp->neighbourhoods) {

	
		std::vector<bool> printed(user_refs.size(), false);

		for (let& ns: nbh_sort[0]) {

			let v = ns.first;
			let vs_remap = ns.second;

			let& fe = fgc.get_elem(v);

			if (!rp && printed[fe.ver])continue;
			printed[fe.ver] = true;

			let& q = user_refs[fe.ver];

			let str_id = ref2str(q.idx) + "_n" + std::to_string(vs_remap + 1);
			let ds = make_var(pos, str_id);

			

			bool use_self = (rp != nullptr);
			//union elements
			size_t ofs = dst.add_args(ds, ETYPE::uni, fe.sz + (use_self ? 1 : 0));

			if (use_self) {
				make_ref(ofs++, q.idx);
			}

			for (size_t j = 0; j < fe.sz; ++j) {//by neighbors list

				let& h = nb.m_data[fgc.get_inter_direct(fe.idx + j)];

				if (!rp) {
					make_ref(ofs++, start_neighbours1 + h.idx_graph);
				} else {
					auto a = dst.add_args(ofs++, ETYPE::mul, 2);
					make_ref(a++, start_neighbours2 + h.idx_graph);

					let& uvt = user_refs[h.s1];

					if (uvt.is_user()) {
						make_ref(a++, uvt.idx);
					} else {
						make_ref(a++, start_sets + uvt.idx);
					}
				}
			}
			
			
			

		}

	}

	return true;
}
