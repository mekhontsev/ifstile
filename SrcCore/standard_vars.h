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
#include "geometry.h"
#include "palette.h"
#include "ims_operator.h"


struct oper_block;
struct render_params;
struct eval_context;


struct standard_vars
{
	using Real = double;

	bool m_si_empty = true;//section status

	subspace_info<Real> m_si2;
	camera_ex m_xcam2;
	palette m_pal;
	background m_bac;
	colorize_params m_colorize;
	light_params<Real> m_light;

	std::array<bool, c_num_builtins> m_has_builtin;

	void clear8();

	bool chas_builtin(builtin_ids bid) const;

	//synchronizes the block with the m_has_builtin array
	//add_all - add everything, ignoring m_has_builtin
	void sync_builtins(bool add_all, oper_block& b);


	void add_all_builtins(
		oper_block& dst,
		const render_params& rp, bool def_back);

	//gets the reference directly without using eval_context
	size_t eval_root(const oper_block& b);
	
	//all except $root
	void eval_builtins(const oper_block& b,	eval_context& ec);

	static void add_palette(oper_block& b, const palette& pal);
	static void add_background(oper_block& b, const background& bac);
	static void add_root(oper_block& b, size_t root_ref);
	static void add_colorize(oper_block& b, const colorize_params& crz);
	static void add_screen_disk(oper_block& b, const screen_disk<Real>& sd);
	static void add_lights(oper_block& b, const light_params<Real>& c);
	static void add_camera(oper_block& b, const camera<Real>& c);
	static void add_section(oper_block& b, const subspace_info<Real>& si);

};





