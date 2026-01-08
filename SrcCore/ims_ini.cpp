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
#include "ims_ini.h"

bool ims_ini::read(std::istream& fs)
{
	m_data.clear();

	bool ret = true;
	std::string q;
	while (std::getline(fs, q))
	{
		let p = q.find_first_of('=');
		if (p == q.npos) {
			ret = false;
			continue;
		}

		std::string val(q.data() + p + 1);
		boost::algorithm::trim(val);

		q.resize(p);
		boost::algorithm::trim(q);

		if (q.empty()) {
			ret = false;
			continue;
		}

		m_data[q] = val;
	}

	return ret;
}

void ims_ini::write(std::ostream& fs) const
{
	using V=decltype(m_data)::value_type;
	std::vector<const V*> keys;
	keys.reserve(m_data.size());
	for (let& q : m_data){
		keys.emplace_back(&q);
	}
	std::sort(keys.begin(), keys.end(), [](let* v1, let* v2) {
		return v1->first < v2->first;
	});
	for (let& q : keys) {
		fs << q->first << "=" << q->second << "\r\n";
	}
}
