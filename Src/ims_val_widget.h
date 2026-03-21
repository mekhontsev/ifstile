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
#include "pool_ptr.h"

struct ims_val;
struct param_walker;
struct param_action;

struct ims_val_widget
{

	void show(const ims_val* d, int& next_id);
	void reset();
	const ims_val* get_val() { return m_value.get(); }
	pool_ptr m_value;
private:

	bool handler(
		const param_walker& t,
		const ims_val * d,
		ims_val* v,
		param_action& res);

	int m_next_id;

};
