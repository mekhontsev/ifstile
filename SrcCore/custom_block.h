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

struct oper_block;
struct block_info;
struct ims_identifiers;
struct report_params;
struct inter_result;

bool create_custom_block(
	std::unique_ptr<oper_block>& dst,
	inter_result& ires,
	block_info& ci,
	const oper_block& src,
	ims_identifiers& idf,
	ifs_object_type mode,
	const report_params& rp);

