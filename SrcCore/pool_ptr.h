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

struct ims_val;

//smart shared pointer for ims_val
struct pool_ptr
{
	~pool_ptr() noexcept { reset(); };
	pool_ptr(const ims_val* v = nullptr) noexcept : m_v(v) {};
	pool_ptr(const pool_ptr& other) noexcept;
	pool_ptr(pool_ptr&& other) noexcept;
	pool_ptr& operator=(pool_ptr other) noexcept;//argument - by value
	void swap(pool_ptr& other) noexcept;
	explicit operator bool() const { return m_v; };
	const ims_val& operator*() const { return *m_v; };
	const ims_val* operator->() const { return m_v; };
	const ims_val* get() const { return m_v; };
	ims_val* get_mut() { return const_cast<ims_val*>(m_v); };
	void reset(const ims_val* nv = nullptr);
	void reset_if(const ims_val* nv);

	[[nodiscard]] const ims_val* release();
private:
	const ims_val* m_v;
};
