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

#include "operator_ptr.h"

struct oper_block;

struct full_search
{
	enum class status
	{
		ok,
		complete,
		ignore,
	};
	
	status next(oper_block& dst);

	//fall_back_rad - what range should we search in if the distribution is normal
	bool reset(const oper_block& src, int64_t fall_back_rad);

	bool next_vec();

	

private:

	uint64_t m_next_idx = 0;


	struct permutation_state
	{
		std::span<const uint64_t> p;
		std::vector<uint64_t> s;

		void reset();
	};

	std::vector<permutation_state> m_ps;

	struct state_elem
	{
		int64_t v, b, e;//b<=v<e
		size_t idx = ims_max;//index in m_ps
	};

	std::vector<state_elem> m_state;

	std::unique_ptr<oper_block> m_tm;

	struct control_values_arr
	{
		struct elem : public control_values
		{
			//link to the one which overridden it
			ast_context ptr2;


			//the graph operator was overridden in the inherited block
			bool vis;

			//forced copy
			bool vis2;
		};

		std::vector<elem> a;
	};

	control_values_arr m_opinfo4;

};