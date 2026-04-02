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
#include "gtest/gtest.h"
#include "spline.h"
#include <boost/math/interpolators/makima.hpp>

TEST(testSpline, testSpline)
{

	using Data = std::vector<double>;
	using makima = boost::math::interpolators::makima<Data>;

	struct TestData
	{
		Data times;
		Data points;
	};

	std::vector<TestData> tests_compare =
	{

		{
			{1, 5, 9 , 17, 20,},
			{1, 1, 1, 1, 1},
		},
		{
			{1, 5, 9 , 12, 17, 20,},
			{8, 17, 4, -3, 5, 7	},
		},
	};


	aspline<double> sp;
	std::vector<double> temp;

	for (auto& q : tests_compare) {

		let& times = q.times;
		let& points = q.points;
		let n = times.size();
		EXPECT_EQ(n, points.size());

		auto bsp = makima(Data(times), Data(points));

		//////////////////////////////////////

		sp.init(1, n);

		auto* d = sp.get_data(0);
		for (size_t i = 0; i < n; ++i)
		{
			d[0] = points[i];
			d += sp.stride();
		}

		sp.create(times.data(), temp);

		let t0 = times.front();
		let t1 = times.back();
		const size_t num_pt = 100;
		for (size_t i = 0; i <= num_pt; ++i) {
			let t = t0 + (double(i) / num_pt) * (t1 - t0);

			let jdx = sp.find_idx(times.data(), times.size(), t);
			double res = 0;
			sp.get2(&res, t, times.data(), jdx);

			double res2 = bsp(t);

			EXPECT_TRUE(!std::isnan(res));
			EXPECT_NEAR(res, res2, 1e-17);
		}
	}

	////////////////////////////////////////////////////////////////////////////

	std::vector<TestData> tests_linear =
	{

		{
			{1,2},
			{2,4},
		},
		{
			{1,2,3},
			{2,4,6},
		},
	};


	for (auto& q : tests_linear) {

		let& times = q.times;
		let& points = q.points;
		let n = times.size();
		EXPECT_EQ(n, points.size());

		sp.init(1, n);

		auto* d = sp.get_data(0);
		for (size_t i = 0; i < n; ++i)
		{
			d[0] = points[i];
			d += sp.stride();
		}

		sp.create(times.data(), temp);


		let t0 = times.front();
		let t1 = times.back();
		const size_t num_pt = 10;
		for (size_t i = 0; i <= num_pt; ++i) {
			let t = t0 + (double(i) / num_pt) * (t1 - t0);

			let jdx = sp.find_idx(times.data(), times.size(), t);
			double res = 0;
			sp.get2(&res, t, times.data(), jdx);

			EXPECT_TRUE(!std::isnan(res));
			EXPECT_NEAR(res, 2 * t, 1e-15);
		}
	}

}
