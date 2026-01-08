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

#pragma  once
#include "operator_ptr.h"

struct oper_block;
struct ims_random;

struct variator_params
{
	//how many non-empty maps can be changed in a prototype
	size_t m_kernel_defect = 1;

	//maximum number of disabled maps allowed
	size_t m_max_disabled = 0;

	//default variance
	double m_search_rad = 0.3;

	//use relative shift when varying
	bool m_relative_shift = true;

	//always change the maximum allowed number
	bool m_change_all = false;

	void set_default()
	{
		m_kernel_defect = 1;
		m_max_disabled = 0;
		m_search_rad = 0.3;
		m_relative_shift = true;
	}
};


struct variator_ex
{
	control_values2 m_opinfo2;

	//size equals the number of variables in the variable block
	std::vector<override_info> m_ovr_arr;

	//list of enabled variables that can be disabled and changed
	std::vector<size_t> ar_enab;
	//list of disabled variables that can only be enabled
	std::vector<size_t> ar_disa;

	//enabled variables that can't be disabled (only changed)
	std::vector<size_t> ar_change;

	//returns the number of changed variables
	size_t variate(
		oper_block& dst,
		const oper_block& src,
		const variator_params& vp,
		ims_random& irn);

};


