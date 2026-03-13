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
#include "ims_chrono.h"

#if defined(DEVELOPER_VERSION)
#define IMS_TEST_MODE 1
#endif


struct clock_print 
{
public:

	clock_print(std::string_view str) 
	{
		m_str = str;
		m_last = ims_chrono::now();
	};

	~clock_print()
	{
		next("");
	};

	void next(std::string_view str)
	{
		let now = ims_chrono::now();
		if (!m_str.empty()) {
			let df = ims_chrono::dif_micro(m_last, now) * 1e-3;
			fmt::println(std::cout, "{}: {:.6g} ms", m_str, df);
		}
		m_last = now;
		m_str = str;
	}


private:
	std::string m_str;
	ims_chrono m_last;
};



#if defined(IMS_TEST_MODE)
using test_clock_print = clock_print;
#define test_out std::cout 
#else
struct test_clock_print
{
	test_clock_print(std::string_view) {};
	void next(std::string_view) {}
};

struct ims_null_out 
{
	template<typename T>
	ims_null_out& operator<< (const T&) { return *this; }
};


#define test_out ims_null_out()


#endif

struct calc_time_helper
{
	using Real = float;

	calc_time_helper(Real& v) : m_v(v) {
		m_last = ims_chrono::now();
	};

	~calc_time_helper()
	{
		m_v = static_cast<Real>(m_last.to_now_micro() * 1e-3);
	};

private:
	Real& m_v;
	ims_chrono m_last;
};

#if defined(DEVELOPER_VERSION)
#define CALC_TIME_HELPER(v) calc_time_helper cth(v)
#else
#define CALC_TIME_HELPER(v)
#endif