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
#include "conbuf.h"

std::streambuf* ims_conbuf::redirect(std::ostream& os)
{
	clear();
	return os.rdbuf(this);
}

void ims_conbuf::sync_string(std::string& str) const
{
	std::scoped_lock lock(m_lock);
	auto b = view();
	if (str.size() == b.size())return;

	if (str.size() > b.size()) {
		str.assign(b);
	} else {
		str.append(b.begin() + str.size(), b.end());
	}
}

void ims_conbuf::clear()
{
	std::scoped_lock lock(m_lock);
	str("");
}

std::stringbuf::int_type ims_conbuf::overflow(int_type c)
{
	std::scoped_lock lock(m_lock);
	return std::stringbuf::overflow(c);
}
