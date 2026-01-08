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
#include "pool_ptr.h"

struct variable
{
	ast_context c;

	pool_ptr v[2];

	//was evaluated
	bool ready[2];

	//if it doesn't depend on cycles, then it's open
	bool dep_from_cycles;

	//if it doesn't depend on unions and is open, then it's geometric
	bool dep_from_unions;

	//is a substitution
	bool is_subs;

	//overloaded in the hierarchy
	bool overriden;

	//during evaluation
	bool in_eval;

	//directly or through substitutions depends on at least one template
	bool is_var2;

	////////////////////////////////////////////////////////////////////////

	bool is_var() const
	{
		return is_var2 && !is_subs && is_geom();
	}

	bool is_open() const
	{
		return !dep_from_cycles;
	}

	bool is_geom() const
	{
		return !dep_from_cycles && !dep_from_unions;
	}

	const ims_val* get_topo_val() const
	{
		return (is_open() || is_subs) ? nullptr : v[1].get();
	}
};