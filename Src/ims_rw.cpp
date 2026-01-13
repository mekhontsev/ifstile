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
#include "ims_rw.h"
#include "ims_file.h"


bool ims_reader::open(const std::string& filename, const std::string& data)
{
	if (!data.empty()) {
		m_resource.str(data);
		m_stream = &m_resource;
		return true;
	}

	ims_file::open(m_file, filename);

	if (m_file.bad()) {
		return false;
	}
	m_stream = &m_file;
	return true;
}



////////////////////////////////////////////////////////////////////////////////
ims_writer::~ims_writer()
{
	close();
}

bool ims_writer::open_stream(const std::string& filename)
{
	close();
	ims_file::open(m_fs, filename);
	if (!m_fs.is_open())return false;
	return true;
}

void ims_writer::set_file(FILE* f)
{
	close();
	m_f = f;
	std::stringstream().swap(m_data);//clear
}

void ims_writer::close() 
{
	if (m_f) {
		let d = m_data.str();
		fwrite(d.data(), 1, d.size(), m_f);
		fclose(m_f);
		m_f = nullptr;
	} else {
		m_fs.close();
	}
}

std::ostream& ims_writer::stream()
{
	if (m_f) {
		return m_data;
	} else {
		return m_fs;
	}
	
}

