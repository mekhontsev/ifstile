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
#include "ims_random.h"

ims_random::ims_random() :rng(
#ifndef IMS_THREADS
	std::random_device{}()
#else
	static_cast<std::mt19937::result_type>(
		reinterpret_cast<std::uintptr_t>(this))
#endif
){}

ims_random::Real ims_random::get_normal()
{
	return distr(rng);
}

ims_random& ims_random::get()
{
	static thread_local ims_random s_gen;
	return s_gen;
};
