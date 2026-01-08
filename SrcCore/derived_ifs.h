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
#include "report_params.h"


struct neighbors_data;
struct oper_block;
struct block_impl;
struct ims_graph;
struct ims_identifiers;

using dim_filter_type = 
std::function<void(std::vector<bool>&, report_params::filter_type, ims_graph&)>;

bool create_neghbours(
	ims_identifiers& idf,
	oper_block& dst,
	const oper_block& src,
	neighbors_data& nb,
	const ims_graph& dig,
	const report_params* rp,
	const dim_filter_type& filter_func);

