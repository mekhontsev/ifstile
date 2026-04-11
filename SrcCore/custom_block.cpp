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

#include "custom_block.h"
#include "oper_block.h"
#include "block_info.h"
#include "derived_ifs.h"
#include "integer_ims.h"
#include "neighbors_data.h"
#include "edge_map.h"


bool create_custom_block(std::unique_ptr<oper_block>& dst, 
	inter_result& ires,
	block_info& ci, 
	const oper_block& src, 
	ims_identifiers& idf, 
	ifs_object_type mode, 
	const report_params& rp)
{
	

	integer_ims cs;
	neighbors_data nb;

	integer_ims::settings settings
	{
		.max_inters = rp.max_complexity,
		.max_depth = ims_max,//do not limit in this mode
		.max_bits = rp.max_bits,
		.prec = rp.find_prec2,
		.mode_ori = false,
		.stop_on_overlap = false,
		.stop_on_incomplete = false,
	};

	ires = cs.calc_inter(nb, ci, settings);

	if (!ires.m_completed) {
		return false;
	};

	if (ims_need_stop()) {
		return false;
	}

	using Real = block_info::Real;
	auto& dim_calc = ci.m_dim_calc;

	//boundary must be initialized
	auto filter_func = [&dim_calc, &ci](
		std::vector<bool>& used,
		const report_params::filter_type flt,
		const ims_graph& boundary)
		{
			used.resize(boundary.num_ver());

			for (size_t v = 0; v < boundary.num_ver(); ++v) {
				used[v] = boundary.num_edges(v) > 0;
			}

			if (flt == report_params::filter_type::all) {
				return;
			}
			if (flt == report_params::filter_type::positive) {
				for (size_t v = 0; v < boundary.num_ver(); ++v) {
					let& c = boundary.m_comp[boundary.m_ver2com[v]];
					used[v] = !c.countable;
				}
				return;
			}

			/////////////////////////////////////////////
			//remove non-maximum dimensions

			ifs_metrics<Real> metr;
			dim_calc.compute_all_dims(metr, boundary,
				ims_view(&ci.m_em.data()->det_rootn, sizeof(edge_map)));

			if (ims_need_stop()) {
				return;
			}

			Real dim = 0;
			for (let& q : metr.di) {
				dim = std::max(q.H, dim);
			}

			let& eps = ims_num_traits<Real>::almost_zero();

			bool mes_inf = false;
			for (let& q : metr.di) {
				if (q.H + eps >= dim) {
					if (q.DR == dim_relations::equ) {
						mes_inf = true;
						break;
					}
				}
			}

			assert(boundary.m_comp.size() == metr.di.size());

			for (size_t v = 0; v < boundary.num_ver(); ++v) {
				let& d = metr.di[boundary.m_ver2com[v]];

				if (d.H + eps < dim ||
					(mes_inf && d.DR != dim_relations::equ))
				{
					used[v] = false;
				}
			}

		};

	auto flt = rp.filer;
	let bmode = mode == ifs_object_type::boundary;
	if (bmode) {
		if (ci.common_dim_proj() > 1) {
			flt = report_params::filter_type::positive;
		} else {
			flt = report_params::filter_type::all;
		}
	}


	let num_ver = nb.set_idx_graph();

	if (num_ver == 0) {
		return false;
	}

	ims_graph boundary;
	if (!nb.create_boundary(ci.get_fg(), boundary)) {
		return false;
	}

	//pre-filter
	std::vector<bool> used;

	boundary.init();//finds components of dimension 0

	if (rp.only_strong_intres) {
		boundary.remove_non_strong_edges();
	}
	filter_func(used, flt, boundary);
	if (ims_need_stop()) {
		return false;
	}

	//////////////////////////////////

	//take into account empty ones
	for (auto& q : nb.m_data) {
		if (q.idx_graph != ims_max && !used[q.idx_graph]) {
			q.res = inter_type::empty;
		}
	}

	//clean out those who have become empty.
	nb.collapse_empty();

	//nbm also contains right-hand ones, so we need to create a numbering system
	//in custom mode, we list everyone, even the right-hand ones
	let num_neighbours = nb.set_idx_graph(!bmode);
	if (!num_neighbours) {
		return false;
	}


	dst = std::make_unique<oper_block>();
	return create_neghbours(
		idf,
		*dst,
		src,
		nb,
		ci.get_fg(),
		bmode ? nullptr : &rp,
		filter_func);//post-filter
}
