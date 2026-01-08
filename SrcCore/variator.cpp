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
#include "variator.h"
#include "oper_block.h"
#include "ims_random.h"
#include "eval_context.h"
#include "block_class.h"
#include "variable.h"

size_t variator_ex::variate(
	oper_block& dst,
	const oper_block& src,
	const variator_params& vp,
	ims_random& irn)
{
	check_block(&src);
	
	auto kernel_defect = vp.m_kernel_defect;
	auto max_empty = vp.m_max_disabled;
	
	let* g = src.ctx();

	auto* c = src.get_class();

	//flag that we are inheriting and not changing the prototype
	const oper_block* inhf = nullptr;

	if (src.own_ctx()) {
		if (g->has_vars()) {
			inhf = &src;
		} else {
			return 0;
		}
	} else {
		if (src.m_flags.free_var) {
			inhf = &src;
		} else {
			assert(src.m_flags.only_var);
		}
	}

	if (inhf) {

		ar_enab.clear();

		//all elements have changed, all of them can be changed
		dst.inherit_from(*inhf, vp, m_opinfo2, false);

		size_t num_change = 0;

		for (let& q : dst) {
			if (q.is_builtin())continue;
			++num_change;
			if (c->m_refs[q.gr()].can_be_empty) {
				ar_enab.emplace_back(q.pos5);
			};
		};


		if (max_empty == 0) {
			return num_change;
		}

		//turn off several of the changed ones

		size_t num_empty = std::min(max_empty, ar_enab.size());

		if (num_empty == 0) {
			return num_change;//don't disable anything
		}

		std::shuffle(ar_enab.begin(), ar_enab.end(), irn.rng);
		ar_enab.resize(irn.rng() % (num_empty + 1));

		//disabling
		for (let i : ar_enab) {
			dst.m_ops[i].hdr.set_xempty();
		}

		return num_change;

	}

	std::scoped_lock lock(oper_block::s_access_lock);

	
	////////////////////////////////////////////////////////////////////////
	//here are those that have "only_var"

	//on each map we can do:
	//1) nothing
	//2) enable+change (for disabled)
	//3) disable (for enabled)
	//4) change (for enabled)

	//for each map specified
	//1) is it possible to change/enable/disable
	//2) can we disable it /leave it enabled?

	//there are 2 parameters
	//1) WISH - the maximum number of maps that can be changed/disabled/enabled
	//	if 0, then they cannot be disabled,
	//	and you can change/enable one, or change one if there are no disabled ones
	//
	//2) What is the maximum number of maps that can be disabled?

	///////////////////////////////////////////////////////////////////////////
	//1 - enable empty ones to make sure the number is no less than necessary
	// each such case is considered a change
	///////////////////////////////////////////////////////////////////////////


	ar_enab.clear();
	ar_disa.clear();
	ar_change.clear();

	

	//how many are empty now?
	size_t num_empty = 0;

	//how much has been changed already
	size_t already_changed = 0;


	m_ovr_arr.clear();

	let& ecr = src.ctx()->m_refs5;

	for(let& q: src){
		if (q.is_builtin())continue;

		let i = m_ovr_arr.size();
		m_ovr_arr.emplace_back();
		auto& oi = m_ovr_arr[i];
		oi.ref2 = q.gr();
		oi.src = ast_context(src.get_ptr(q.pos5), 0);
		oi.proto2 = oi.src;

		bool is_empty = oi.src.h.is_xempty();

		let& v = c->m_refs[q.gr()];

		if (!v.var_is_locked) {
			if (v.can_be_empty) {
				if (is_empty) {
					ar_disa.emplace_back(i);
				} else {
					ar_enab.emplace_back(i);
				}
			} else {
				if (is_empty) {
					oi.src = ecr[oi.ref2].c;//enable + change
					is_empty = false;
					++already_changed;
				} else {
					ar_change.emplace_back(i);
				}
			};
		};

		if (is_empty) {
			++num_empty;
		}
	};


	if (kernel_defect == 0) {

		if (num_empty == 0) {
			kernel_defect = 1;
			//if there are no empty maps, then one of them can be replaced
			ar_change.insert(ar_change.end(), ar_enab.begin(), ar_enab.end());
		} else {
			//if there are empty maps, we can't change anything, just enable some maps
			ar_change.clear();
		}
		//no one can be disabled
		ar_enab.clear();
	}
	

	//how much will have to be changed at least?
	size_t min_change = 0;
	if (max_empty < num_empty) {//we need to have more enabled maps
		min_change = num_empty - max_empty;
		if (min_change > ar_disa.size()) {//there are such numbe of empty maps to enable
			return 0;
		};
	};

	//what is the maximum number of maps that can be changed?
	auto max_change = ar_disa.size() + ar_enab.size() + ar_change.size();
	if (max_change < min_change) {//there isn't enough to change
		return 0;
	}


	let kd = std::max(kernel_defect, (size_t)1);
	if (kd > already_changed) {
		//there is no point to change a larger number of maps
		let mv = std::max(kd - already_changed, min_change);
		max_change = std::min(max_change, mv);
	} else {
		max_change = min_change;
	}

	if (max_change == 0) {
		if (already_changed > 0) {
			dst.copy_ovr(src, vp, m_opinfo2.cv, m_ovr_arr);
		}
		return already_changed;
	}

	//change at least one
	min_change = std::max(min_change, (size_t)1);
	assert(min_change <= max_change);


	//how many maps will we actually change: min_change .. max_change
	let num_change = vp.m_change_all? max_change:
		(irn.rng() % (max_change - min_change + 1) + min_change);

	//////////////////////////////////////////////////////////////

	//how many empty maps can there be in total [we1, we2]
	//we1 - we will spend all (can_enab) on enabling empty maps:
	auto can_enab = std::min(num_change, ar_disa.size());
	size_t we1 = num_empty > can_enab ? num_empty - can_enab : 0;
	assert(we1 <= max_empty);//we1 = num_empty - num_change  <= num_empty - min_change = max_empty
	//spend all (can_disab) on disabling non-empty maps
	auto can_disab = std::min(num_change, ar_enab.size());
	size_t we2 = (kernel_defect == 0) ? we1 :
		std::min(num_empty + can_disab, max_empty);
	assert(we1 <= we2);
	let will_be_empty = irn.rng() % (we2 - we1 + 1) + we1;

	//////////////////////////////////////////////////////////////////////
	size_t num_enable, num_disable;

	//d==difference between the number of enabled and disabled maps
	//2x+d<=num_change
	if (will_be_empty > num_empty) {//we need to disable more than enable
		//x<can_enab
		//x+d<=can_disab
		//x<=min(can_enab, can_disab-d)
		let d = will_be_empty - num_empty;
		let mx = std::min(std::min(can_enab, can_disab - d), (num_change - d) / 2);
		assert(can_disab >= d);
		let x = irn.rng() % (mx + 1);
		num_enable = x;
		num_disable = x + d;
	} else {
		//x+d<=can_enab
		//x<=can_disab
		//x<=min(can_disab, can_enab-d)
		let d = num_empty - will_be_empty;
		assert(can_enab >= d);
		let mx = std::min(std::min(can_disab, can_enab - d), (num_change - d) / 2);
		let x = irn.rng() % (mx + 1);
		num_enable = x + d;
		num_disable = x;
	}

	assert(num_disable + num_enable <= num_change);
	assert(will_be_empty + num_enable == num_empty + num_disable);

	


	//////////////////////////////////////////////////////////////////////
	if (num_enable > 0) {
		std::shuffle(ar_disa.begin(), ar_disa.end(), irn.rng);
	}
	
	ar_disa.resize(num_enable);//here are those who we will include (randomly change)
	
	for (let idx : ar_disa) {
		auto& oi = m_ovr_arr[idx];
		oi.src = ecr[oi.ref2].c;
	}
	//////////////////////////////////////////////////////////////////////
	std::shuffle(ar_enab.begin(), ar_enab.end(), irn.rng);
	
	//move the tail into the ar_change list
	for (size_t i = num_disable; i < ar_enab.size(); ++i) {
		ar_change.emplace_back(ar_enab[i]);
	}
	
	ar_enab.resize(num_disable);//here are the maps that we will disable

	//////////////////////////////////////////////////////////////////////
	std::shuffle(ar_change.begin(), ar_change.end(), irn.rng);
	
	let num_izm = num_change - (num_disable + num_enable);
	if (num_izm < ar_change.size()) {
		ar_change.resize(num_izm);//here are the maps that we will randomly change
	}
	
	
	for (let idx : ar_change) {
		auto& oi = m_ovr_arr[idx];
		oi.src = ecr[oi.ref2].c;
	}
	//////////////////////////////////////////////////////////////////////

	//disabling non-empty
	for (let idx : ar_enab) {
		m_ovr_arr[idx].src.h.set_xempty();
	}

	dst.copy_ovr(src, vp, m_opinfo2.cv, m_ovr_arr);

	return num_change + already_changed;

}