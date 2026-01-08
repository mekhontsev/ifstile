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


template<typename Real>
struct ray3d
{
	using Vec3 = Eigen::Matrix<Real, 3, 1>;
	Vec3 p;//location
	Vec3 d;//direction
};

//3D ball
template<typename Real>
struct ball3d
{
	using Vec3 = Eigen::Matrix<Real, 3, 1>;

	Vec3 c;
	Real r;

	

	bool defined() const { return r >= 0; };
	void set_undef() { r = -1; }


	ball3d() {};
	ball3d(const Vec3& pp, Real rr) { set(pp, rr); };

	void set(const Vec3& pp, Real rr) { c = pp; r = rr; };

	void set(Real xx, Real yy, Real zz, Real rr)
	{
		c.set(xx, yy, zz);
		r = rr;
	};

	//returns true if the ball this contains b
	bool is_contain(const ball3d<Real>* b) const
	{
		if (b->r > r)return false;//this ball is smaller than b
		double dr = r - b->r;//>=0 due to the previous condition
		return c.dist2(b->c) <= dr * dr;
	};

	//find intersection points along the ray (with unit direction)
	//returns false if it does not intersect
	//t1<=t2 - intersection points in the ray coordinate system
	bool intersect(const ray3d<Real>& ry, Real* t1, Real* t2)
	{
		auto q = c- ry.p;
		Real a = q.dot(ry.d);
		Real h = r * r + a * a - q.squaredNorm();
		if (h < 0)return false;
		h = sqrt(h);
		*t1 = a - h;
		*t2 = a + h;
		return true;
	};

	//the ball this is replaced with the minimum ball containing this and the ball (p,r)
	//returns true if this has changed
	bool circumscribe(const Vec3& p, Real pr)
	{
		Real d = (c-p).squaredNorm();
		const Real rd = r - pr;
		if (d <= rd * rd) {//one is contained within the other
			if (r >= pr)return false;
			set(p, pr);
			return true;
		}

		d = sqrt(d);

		r = (r + pr + d) / 2;

		c.sub(p);
		c.mul((r - pr) / d);
		c.add(p);

		return true;
	};
	
};

