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

template<typename T>
struct ims_visitor
{
	struct elem
	{
		size_t id = 0;
		T d;

		bool visited(size_t sid)
		{
			if (sid == id)return true;
			id = sid;
			return false;
		}
	};

	T& get(size_t idx) { return data[idx].d; };
	const T& get(size_t idx) const { return data[idx].d; };


	bool visited(size_t idx)
	{
		return data[idx].visited(id);
	}
	

	void next()
	{
		++id;
	}

	std::vector<elem> data;

	size_t id = 1;//0 is forbidden
};
