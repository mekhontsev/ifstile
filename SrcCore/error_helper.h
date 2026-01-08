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


namespace error_helper
{	
	void set_var(std::string_view name);
	void set_line(size_t idx);
	void set_to_buf(bool r);
	std::string get_buf();
	
	struct var
	{
		var(std::string_view name) { set_var(name); };
		~var() { set_var({}); }
	};

	struct line
	{
		line(size_t line) {set_line(line); };
		~line() { set_line(0); }
	};

	struct use_buf
	{
		use_buf() {set_to_buf(true); };
		~use_buf() { set_to_buf(false); }
		//get and clear the buffer
		std::string get_buf() {return error_helper::get_buf();};
	};
};

