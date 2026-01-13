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
#include "projector.h"
#include "operator_ptr.h"

struct eval_context;

struct proj_data
{
	using Integer = int64_t;

	struct proj_elem
	{
		ast_context m_ptr;//key for searching the projector
		std::vector<double> m_sbt;
		DynMat<Integer> m_A;
		projector m_projector;
		bool ready;
	};

	std::vector<proj_elem> m_projs;

	DynMat<Integer> m_A_temp;
	std::vector<double> m_sbt_temp;

	//get the projector index
	size_t get_proj(const ast_context& p, eval_context& ec);
	void recheck();
	void clear();
};
