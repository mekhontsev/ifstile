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
#include "operator_ptr.h"

struct ovr_data
{
	struct elem
	{
		operator_ptr p;
		bool is_subs = false;
	};
	std::vector<elem> m_arr;
	builtin_arr m_builtins;
	const oper_block* m_p;
	//returns the first block that was not included in the merge
	void init(const oper_block& b, bool without_own_ctx = false);

	//merges the hierarchy into one block
	void merge_from(oper_block& dst, const oper_block& src) const;
};
