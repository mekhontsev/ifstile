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
#include "ims_info_counted_map.h"

template <typename... Ts>
struct std::hash<boost::dynamic_bitset<Ts...>>
	: boost::hash<boost::dynamic_bitset<Ts...>> {
};


//substitution - instead of copying, the content is substituted
//furthermore, it can't be important and the user can't access it from the GUI

//for each reference, it stores whether it is a substitution

struct var_subs_data
{
	using key = boost::dynamic_bitset<>;
};

using var_subs_info = ims_info_counted_map<var_subs_data>;



////////////////////////////////////////////////////////////////////////////////

struct var_subs_info_helper
{
	void start(var_subs_info::V* par)
	{
		if (!par) {
			m_s.clear();
		} else {
			m_s = par->first;
		}
	};

	void set_sub(size_t idx)
	{
		if (m_s.size() <= idx) {
			m_s.resize(idx + 1);
		}
		m_s[idx] = 1;
	};

	void override_sub(size_t idx, bool s)
	{
		if (m_s.size() <= idx) {
			m_s.resize(idx + 1);
		}
		m_s[idx] = s ? 1 : 0;
	};


	void finish(size_t size)
	{
		if (m_s.size() <= size) {
			m_s.resize(size);
		}
	};

	var_subs_info::key m_s;//for temporary needs
};

