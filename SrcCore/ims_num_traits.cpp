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
#include "ims_num_traits.h"

using FNB = ims_num_traits<float_number_boost>;

float_number_boost FNB::m_epsilon{};
float_number_boost FNB::m_almost_zero{};

void ims_num_traits_init_all() 
{
	FNB::m_epsilon		=	std::numeric_limits<float_number_boost>::epsilon();
	FNB::m_almost_zero	=	sqrt(FNB::m_epsilon);
};
