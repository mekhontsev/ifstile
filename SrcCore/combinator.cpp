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
#include "combinator.h"

void combinator::init(size_t n, size_t k)
{
	assert(k <= n);
	mask.resize(n);
	idxs.reserve(k);
	std::fill(mask.begin(), mask.begin() + k, true);
	std::fill(mask.begin() + k, mask.end(), false);
}

void combinator::get()
{
	idxs.clear();
	for (size_t i = 0; i < mask.size(); ++i) {
		if (mask[i]) {
			idxs.push_back(i);
		}
	}
}

bool combinator::next()
{
	return std::prev_permutation(mask.begin(), mask.end());
}

bool combinator1::next()
{
	for (auto& q : m_state) {
		++q.v;
		if (q.v < q.e) {
			return true;
		}
		q.v = q.b;
	}
	return false;
}



bool combinator2::next()
{
	//always start incrementing the second-to-last digit because
	//the last value is always calculated automatically
	if (m_state.size() < 2) {
		return false;
	}

	size_t i = m_state.size() - 2;

	for (;;) {
		auto& s = m_state[i];

		//what is the maximum in this group?
		let ma = std::min(s.num, s.max_cur);

		//what is the minimum in this group?
		//let mi = (s.num > s.max_other) ? s.num - s.max_other : (size_t)0;

		++s.v;
		if (s.v <= ma) {
			//fill the tail
			size_t num = s.num - s.v;//this much needs to be placed further
			size_t n = 0;
			for (size_t j = m_state.size() - 1; j > i; --j) {
				auto& q = m_state[j];

				q.v = std::min(num, q.max_cur);//placed everything we could
				n += q.v;
				q.num = n;
				num -= q.v;
			}
			if (num == 0) {
				return true;
			}
		}

		if (i == 0) {
			return false;
		}

		--i;
	}
}

bool combinator2::init(size_t num)
{
	size_t m = 0;
	size_t n = 0;
	for (auto& q : boost::adaptors::reverse(m_state)) {
		q.max_other = m;
		m += q.max_cur;


		q.v = std::min(num, q.max_cur);//placed everything we could

		n += q.v;
		q.num = n;
		num -= q.v;
	};

	return num == 0;//everyone has placed
}

void combinator3::init_group(size_t idx)
{
	auto& s = m_settings[idx];
	for (size_t i = 0; i < s.num; ++i) {
		m_state[s.idx + i] = i;
	}
}

bool combinator3::next_group(size_t idx)
{
	auto& s = m_settings[idx];

	if (s.num == 0)return false;


	//always start increasing the last digit
	size_t i = s.num - 1;
	for (;;) {
		auto& q = m_state[s.idx + i];
		++q;

		//i will have q, i+1 will have at least q+1, k will have q+k-i
		//s.num-1 will have at least q+s.num-1-i
		//then q+s.num-1-i < s.max
		//q+s.num-1-i < s.max
		if (q + s.num - 1 - i < s.max) {
			++i;
			if (i == s.num) {
				return true;//were able to increase the last digit
			}

			m_state[s.idx + i] = q;//will immediately increase on the next iteration
		} else {
			if (i == 0) {
				return false;
			}

			--i;
		}
	}
}

void combinator3::init()
{
	size_t idx = 0;
	for (auto& q : m_settings) {
		q.idx = idx;
		idx += q.num;
	}
	m_state.resize(idx);

	for (size_t i = 0; i < m_settings.size(); ++i) {
		init_group(i);
	}
}

bool combinator3::next_from(size_t idx)
{
	size_t i = idx;
	for (;;) {


		if (next_group(i)) {
			for (size_t j = i + 1; j < m_settings.size(); ++j) {
				init_group(j);
			}
			return true;
		}
		if (i == 0) {
			return false;
		}

		--i;
	}
}
