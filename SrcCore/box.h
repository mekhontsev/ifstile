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
#include "dyn_mat_vec.h"

template<typename Real>
struct box
{
	using Point = DynVec<Real>;

	std::array<Point, 2> c;//corner points
	size_t numpt = 0;//how many points


	template<typename PT>
	void add_point(const PT& p, Real r = 0)
	{
		
		let n = (size_t)p.rows();

		if (numpt == 0) {
			c[0] = p;
			c[1] = p;

			if (r > 0) {
				for (size_t i = 0; i < n; ++i) {
					c[0](i) -= r;
					c[1](i) += r;
				}
			} 
			
			numpt = 1;
			return;
		}


		let n_old = (size_t)c[0].rows();
		if (n != n_old) {
	
			if (n_old < n) {
				for (auto& q : c) {
					q.conservativeResize(n);
					for (size_t i = n_old; i < n; ++i) {
						q[i] = 0;
					}
				}
			} else {
				for (size_t i = n; i < n_old; ++i) {
					c[0](i) = std::min(c[0](i), 0 - r);
					c[1](i) = std::max(c[1](i), 0 + r);
				}
			}
			
		}
		

		for (size_t i = 0; i < n; ++i) {
			c[0](i) = std::min(c[0](i), p(i)-r);
			c[1](i) = std::max(c[1](i), p(i)+r);
		}
		numpt += 1;
	};

	template<typename PT>
	bool contain(const PT& bc, const Real& br) const
	{
		if (empty())return false;
		let n = (size_t)c[0].size();
		for (size_t i = 0; i < n; ++i) {
			let& x = bc[i];
			if (x - br < c[0][i] || x + br > c[1][i])return false;
		}
		return true;
	}

	size_t get_dim() const { return numpt > 0 ? (size_t)c[0].size() : 0; };
	Real size(size_t idx) const { return numpt > 0?c[1](idx) - c[0](idx):0;}

	Real max_size() const 
	{
		Real ret = 0;
		let dim = get_dim();
		for (size_t i = 0; i < dim; ++i) {
			ret = std::max(ret, size(i));
		}
		return ret;
	}

	//inflates for single-point
	void adjust() 
	{
		if (empty() || max_size() > 0) {
			return;
		}

		let dim = get_dim();
		for (size_t i = 0; i < dim; ++i) {
			c[1](i) += 0.5;
			c[0](i) -= 0.5;
		}
	}

	bool empty() const { return numpt == 0; };

	void get_center(Point& v) const
	{
		v.noalias() = (c[0] + c[1]) / 2;
	};


	Point get_center() const
	{
		return (c[0] + c[1]) / 2;
	}

	void clear() { numpt = 0; };
};
