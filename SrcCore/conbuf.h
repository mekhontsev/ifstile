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

struct ims_conbuf : public std::stringbuf
{
public:
	std::streambuf* redirect(std::ostream& os);
	//does nothing if str.size() == current buffer size
	void sync_string(std::string& str) const;
	void clear();
protected:
	int_type overflow(int_type c = EOF) override;
private:
	mutable std::mutex m_lock;
};
