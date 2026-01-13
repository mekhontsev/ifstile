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

struct ims_reader
{
	std::istream& stream() { return *m_stream; };
	bool open(const std::string& filename, const std::string& data = "");
private:
	//either m_file or m_resource
	std::istream* m_stream = nullptr;

	//closes only when ims_reader is destroyed
	std::ifstream m_file;

	std::stringstream m_resource;
};



struct ims_writer
{
	~ims_writer();

	bool open_stream(const std::string& filename);

	//case of indirect file access - for example "Browse" on Android
	void set_file(FILE* f);//there is no need to close the file - we will close it automatically

	bool accessed_by_name() const { return m_f == nullptr; };

	void close();
	std::ostream& stream();
	

private:

	//for systems with direct access to the file system
	std::ofstream m_fs;

	//for example for Android, where there is no direct access to all files
	FILE* m_f = nullptr;

	std::stringstream m_data;

};

