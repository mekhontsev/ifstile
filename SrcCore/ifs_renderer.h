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
#include "ims_bitmap.h"
#include "ims_info.h"
#include "block_info.h"
#include "standard_vars.h"

struct ifs_renderer 
{
	bool init(const std::string& aifs);

	bool render(ims_bitmap& dst, float quality, float thickness);
	
	ims_info m_nfo;
	block_info m_bi;
	standard_vars m_sv;
	const oper_block* m_bb;
};
