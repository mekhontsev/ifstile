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

#include "pch.h"
#include "edge_ball.h"
#include "ims_val.h"


bool edge_ball::defined2() const
{
	return get() && radius() >= 0;
}

void edge_ball::set_undef2()
{
	if (get())set_radius(-1);
}
////////////////////////////////////////////////////////////////////////////////

size_t edge_ball::dim() const
{
	return dim(get());
}

edge_ball::Real* edge_ball::center_data() const
{
	return center_data(get());
}

edge_ball::Real* edge_ball::radius_data() const
{	
	return radius_data(get());
}


void edge_ball::set_radius(Real r)
{
	set_radius(get(), r);
}

ims_val_b::MVecReal edge_ball::center() const
{
	return center(get());
}

edge_ball::Real edge_ball::radius() const
{
	return radius(get());
}

////////////////////////////////////////////////////////////////////////////////

void edge_ball::set_radius(const ims_val* v, Real r)
{
	*radius_data(v) = r;
}

edge_ball::Real* edge_ball::center_data(const ims_val* v)
{
	return v->p_r() + 1;
}

edge_ball::Real* edge_ball::radius_data(const ims_val* v)
{
	return v->p_r();
}

size_t edge_ball::dim(const ims_val* v)
{
	return v->get_size() - 1;
}

edge_ball::Real edge_ball::radius(const ims_val* v)
{
	return *radius_data(v);
}


ims_val_b::MVecReal edge_ball::center(const ims_val* v)
{
	assert(v->is(ims_val::ETP::vector, ims_val::EST::real));
	return ims_val_b::MVecReal(center_data(v), dim(v));
}

