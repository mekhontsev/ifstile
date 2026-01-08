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
#include "inter_type.h"
#include "ims_val.h"


bool inter_elem::is_eq(const inter_elem& a, const inter_elem& b, double prec)
{
	if (a.s0 != b.s0 || a.s1 != b.s1)return false;

	assert(a.m->gt() == b.m->gt());
	assert(a.m->is(ims_val_b::ETP::matrix));
	
	if (a.m->gs() != b.m->gs())return false;
	if (a.m->raw_size() != b.m->raw_size())return false;

	let sz = a.m->extent(0) * a.m->extent(1);

	switch (a.m->gs())
	{
	case ims_val::EST::rational:
	{
		let* p = a.m->p_i();
		let* pe = p + sz;
		return std::equal(p, pe, b.m->p_i());
	}
	case ims_val::EST::big_rational:
	{
		let* p = a.m->p_b();
		let* pe = p + sz;
		return std::equal(p, pe, b.m->p_b());
	}
	case ims_val::EST::real:
	{
		if (a.res2 != b.res2)return false;
		let* p = a.m->p_r();
		let* pb = b.m->p_r();
		for (size_t i = 0; i < sz; ++i) {
			if (std::abs(p[i] - pb[i]) >= prec) {
				return false;
			}
		}
		return true;
	}
	default:
		assert(false);
		return false;
	}
}

size_t inter_elem::get_hash(const inter_elem& a, double mul_prec)
{
	using boost::hash_combine;

	let* m = a.m.get();

	size_t h = m->raw_size();

	hash_combine(h, a.s0);
	hash_combine(h, a.s1);

	hash_combine(h, m->gs());

	let sz = m->extent(0) * m->extent(1);

	switch (m->gs())
	{
	case ims_val::EST::rational:
	{
		let* p = m->p_i();
		let* pe = p + sz;
		while (p < pe) hash_combine(h, *p++);
		break;
	}
	case ims_val::EST::big_rational:
	{
		let* p = m->p_b();
		let* pe = p + sz;
		while (p < pe) hash_combine(h, *p++);
		break;
	}
	case ims_val::EST::real:
	{
		hash_combine(h, a.res2);

		let* p = m->p_r();
		let* pe = p + sz;
		while (p < pe) {
			let v = static_cast<ims_val::Integer>(std::round((*p++) * mul_prec));
			hash_combine(h, v);
		}
		break;
	}
	default:
		assert(false);
		break;
	}
	return h;
}

size_t inter_elem::get_dim() const
{
	return m->extent(0);
}

size_t inter_elem::get_sz() const
{
	return m->extent(0) * m->extent(1);
}

