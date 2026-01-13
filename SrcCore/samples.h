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


struct samples
{
	samples();

	struct category 
	{
		struct entry
		{
			std::string name;
			std::string path;
			const uint8_t* data = nullptr;
			size_t size = 0;
		};

		std::string name;
		std::vector<entry> smp;
	};


	category& get_recent() { return m_samples.front(); };
	void add_recent(std::string_view filename, bool to_front=true);
	void remove_recent(std::string_view filename);


	static const size_t s_max_recent = 100;
	std::vector<category> m_samples;
};