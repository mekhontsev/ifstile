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

namespace ims_file
{

//open the file for writing in exclusive mode
FILE* open_exclusive_write(const std::string& filename);


void open(std::ofstream& fs, const std::string& filename,
		std::ios_base::openmode mode = std::ios_base::binary);

//using binary mode
void open(std::ifstream& fs, const std::string& filename);

//check file existence - not very fast
bool is_exists(const std::string& filename);

//read the contents of the file into a string
bool get_contents(const std::string& filename, std::string& contents);

//get file extension from path, converts to lowercase
std::string get_extension(std::string_view filename);

//remove the extension together with the dot
std::string_view remove_extension(std::string_view path);

//move up the hierarchy, specifically removing the file name from the path
//if there was only a file name, then returns an empty string
//otherwise, the returned path always ends with /
std::string get_parent(const std::string& fn, std::string* name=nullptr);

bool is_root(const std::string& fn);

//remove .. from the path, replaces back \ with forward /
std::string adjust(std::string_view path);

}

