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
#include "creator_state.h"
#include "ims_info.h"
#include "oper_block.h"
#include "ast_stack.h"




oper_block* create_ifs5(
	ims_info* nfo,
	size_t idx, 
	const creator_state& cs, 
	const std::string& name)
{

	auto ub = std::make_unique<oper_block>();
	auto& b = *ub;

	
	let* fb = cs.get_found(idx);

	let& fval = *fb->second;
	creator_state::create_block2(
		nfo->m_list.m_idf,
		b,
		cs.m_cg.m_ig2,
		cs.m_cg.m_pows,
		fb->first,
		fval.cyc,
		fval.si.data());
	b.m_name = name;

	ast_stack ai;
	ims_info::link_refs_for_block(*nfo, &b, ai);
	

	auto* ret = ub.get();
	nfo->m_list.move_block(ub, "G");
	
	return ret;
};
