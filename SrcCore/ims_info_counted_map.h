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

struct ims_info;

template<typename T>
struct ims_info_counted_map 
{
	struct data: public T
	{
		size_t count = 0;
		ims_info* nfo = nullptr;
	};

	using key = typename T::key;
	//do not move, we will refer to the elements
	using map_type = UNAMESPACE::unordered_map<key, data>;
	using V = map_type::value_type;

	map_type m_map;

	////////////////////////////////////////////////////////////////////////////

	V* insert(ims_info* nfo, const key& k)
	{
		auto res = m_map.try_emplace(k);
		auto* v = &*res.first;
		if (res.second){
			v->second.nfo = nfo;
		}		
		return v;
	};

	//returns true if p is no longer valid
	bool release(V* p)
	{
		assert(p->second.count > 0);
		if (--p->second.count == 0) {
			m_map.erase(p->first);		
			return true;
		}
		return false;
	};

	static void add_ref(V* p)
	{
		++p->second.count;
	};


	static ims_info& nfo(V* p) 
	{
		return *p->second.nfo;
	}
	////////////////////////////////////////////////////////////////////////////

	using ptr = boost::intrusive_ptr<V>;

	friend void intrusive_ptr_add_ref(V* p)
	{
		add_ref(p);
	};

	friend void intrusive_ptr_release(V* p);

};