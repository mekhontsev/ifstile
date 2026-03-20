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

#include "block_info.h"
#include "standard_vars.h"
#include "variator.h"


struct report_params;
struct ims_identifiers;
struct oper_block;

struct build_data
{
	using Integer = int64_t;
	using Real = double;

	//fully materialized block
	block_info m_bi;
	variator_params m_vp;
	//TODO: It is only stored here and is almost never used in build_data
	standard_vars m_special;

	std::unique_ptr<oper_block> m_block_sq;

	//the parent of the custom block.
	std::unique_ptr<oper_block> m_normal_parent;

	//which one from the list it corresponds to (or null_ptr if deleted)
	const oper_block* m_bb = nullptr;

	bool m_changed = false;//was it changed after creation?

	////////////////////////////////////////////

	void clear();

	oper_block* get_direct(ifs_object_type m);
	

	//setting the correct root node based on the changed
	void adjust_roots(ifs_object_type tsrc);

	background* get_background();

	void set_block(std::unique_ptr<oper_block>& other);
	void on_change_mode();

	//performs initialization, finds the root set
	void pre_init(
		const oper_block* db, //block from the list
		const variator_params& vp);

	bool init_normal_block();

	bool init_custom_block(
		ims_identifiers& idf,
		ifs_object_type mode,
		const report_params& rp);

	////////////////////////////////////////////////////////////////////////////

	oper_block& get_block();
	const oper_block& get_block() const;


	bool empty() const;


	//the number of the vertex to be built in the final graph
	size_t get_froot() const;

	bool can_create_view() const;
};

