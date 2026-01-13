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
#include "ims_val_b.h"
#include "pool_ptr.h"

struct edge_ball: public pool_ptr
{
	using pool_ptr::pool_ptr;

	using Real = ims_val_b::Real;

	bool defined2() const;
	void set_undef2();

	size_t dim() const;
	Real* center_data() const;
	ims_val_b::MVecReal center() const;
	Real* radius_data() const;
	Real radius() const;
	void set_radius(Real r);

	static size_t dim(const ims_val*);
	static Real* center_data(const ims_val*);
	static ims_val_b::MVecReal center(const ims_val*);
	static Real* radius_data(const ims_val*);
	static Real radius(const ims_val*);
	static void set_radius(const ims_val*, Real r);


};
