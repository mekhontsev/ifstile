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
#include "pool_ptr.h"
#include "eval_pool.h"

pool_ptr::pool_ptr(pool_ptr&& other) noexcept
{
	m_v = other.m_v;
	other.m_v = nullptr;
}

pool_ptr::pool_ptr(const pool_ptr& other) noexcept
{
	m_v = other.m_v;
	eval_pool::add_ref(m_v);
}


pool_ptr& pool_ptr::operator=(pool_ptr other) noexcept
{
	swap(other);
	return *this;
}

void pool_ptr::swap(pool_ptr& other) noexcept
{
	std::swap(m_v, other.m_v);
}

void pool_ptr::reset(const ims_val* nv /*= nullptr*/)
{
	if (m_v) {
		eval_pool::ep.release(m_v);
	};
	m_v = nv;
}

void pool_ptr::reset_if(const ims_val* nv)
{
	if (nv)reset(nv);
}

const ims_val* pool_ptr::release()
{
	auto* ret = m_v;
	m_v = nullptr;
	return ret;
}
