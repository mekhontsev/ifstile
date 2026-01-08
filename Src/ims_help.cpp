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
#include "ims_help.h"
#include "ims_streambuf.h"

#include "embed/manual.h"

void ims_help::load()
{
	ims_streambuf buf(embed::manual, std::size(embed::manual));
	std::istream reader(&buf);

	m_map.clear();
	std::string line;

	//https://stackoverflow.com/a/20885980/24535365
	constexpr std::string_view marker = "[//]:#(";

	std::string* cur_str = nullptr;
	while(std::getline(reader, line)){
		if(line.find(marker) == 0){//if marker first
			line = line.substr(marker.size(), line.size() - marker.size());
			boost::algorithm::trim_if(line, boost::algorithm::is_any_of(" \t\r\n"));
			if (!line.empty() && line.back()==')') {
				line.pop_back();
			}
			cur_str = &m_map[line];
		} else if(cur_str){
			boost::algorithm::trim_right_if(line, boost::algorithm::is_any_of(" \t\r\n"));
			*cur_str += line;
			cur_str->push_back('\n');
		}
	}
}
