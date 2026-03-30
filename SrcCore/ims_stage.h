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

struct ims_stage : public boost::noncopyable
{
	//amount of work completed, from 0 to 1
	double m_work_done = 0;

	double	m_work_mul = 1;

	std::string m_stage_name;

	void work_reset() { m_work_done = 0; };
	void work_add(double w) { m_work_done += w * m_work_mul; };
	double work_done() const { return m_work_done; };
	static ims_stage& get();
};
