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


void console_writer::redirect()
{
	m_err_buf.redirect();
	m_out_buf.redirect();
}

void console_writer::revert()
{
	m_err_buf.revert();
	m_out_buf.revert();
}

std::string console_writer::fetch_error()
{
	return m_err_buf.fetch_data(m_lock);
}

std::string console_writer::fetch_string()
{
	return m_out_buf.fetch_data(m_lock);
}
