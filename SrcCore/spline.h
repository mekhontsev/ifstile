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

//Makima - Modified Akima piecewise cubic Hermite interpolation
template<typename Real>
struct aspline
{
	using Vec = Eigen::Matrix<Real, Eigen::Dynamic, 1>;
	using Map = Eigen::Map<Vec>;
	using CMap = Eigen::Map<const Vec>;

	std::vector<Real> data;//contains points, tangent vectors, and time

	size_t m_dim = 0;
	
	size_t get_offset(size_t idx) const
	{
		return idx * stride();
	}

	size_t stride() const
	{
		return 2 * m_dim;
	}

	//p, m - total m_dim + m_dim components
	Real* get_data(size_t idx)
	{
		return &data[get_offset(idx)];
	};

	const Real* get_data(size_t idx) const
	{
		return &data[get_offset(idx)];
	};
	
	Real* init(size_t dim, size_t num)
	{
		m_dim = dim;
		data.resize(get_offset(num));
		return data.empty() ? nullptr : &data[0];
	};

	size_t get_num() const
	{
		return data.size() / (2 * m_dim);
	}

	Map M(Real* q) const
	{
		return Map(q, m_dim, 1);
	}

	CMap M(const Real* q) const
	{
		return CMap(q, m_dim, 1);
	}

	void create(const Real* t, std::vector<Real>& temp)
	{
		//find mi
		let n = get_num();
		assert(n > 1);

		let step = stride();

		auto* p = &data[0];
		auto* m_start = &data[0]+m_dim;
		auto* m = m_start;


		Real w1, w2;

		for (size_t i = 0; i + 1 < n; ++i) {
			auto* pn = p + step;

			M(m).noalias() = (M(pn) - M(p)) / (t[i+1] - t[i]);

			p = pn;
			m += step;
		}

		if (n < 4) {
			if (n == 1) {
				M(m_start).setZero();
				return;
			}

			M(m) = M(m - step);//last derivative

			if (n == 3) {
				M(m - step) = (M(m) + M(m_start)) / 2;
			}

			return;
		}

		
		temp.resize(7*m_dim);
		auto* td = temp.data();
		auto v0 = M(td); td += m_dim;
		auto v1 = M(td); td += m_dim;
		auto v2 = M(td); td += m_dim;
		auto x1 = M(td); td += m_dim;
		auto x2 = M(td); td += m_dim;
		auto s0 = M(td); td += m_dim;
		auto s1 = M(td); td += m_dim;
		
		////////////////////////////////////////////////////////////////////////
		td = data.data();
		{
			auto y0 = M(td); td += step;
			auto y1 = M(td); td += step;
			auto y2 = M(td); td += step;
			auto y3 = M(td); td += step;

			let t0 = t[0];
			let t1 = t[1];
			let t2 = t[2];
			let t3 = t[3];

			v0.noalias() = (y1 - y0) / (t1 - t0);
			v1.noalias() = (y2 - y1) / (t2 - t1);
			v2.noalias() = (y3 - y2) / (t3 - t2);
			x1.noalias() = 2 * v0 - v1;
			x2.noalias() = 2 * x1 - v0;
			w1 = (v1 - v0).norm() + (v1 + v0).norm() / 2;
			w2 = (x1 - x2).norm() + (x1 + x2).norm() / 2;
			if (w1 + w2  > 0) {
				s0.noalias() = (w1 * x1 + w2 * v0) / (w1 + w2);
			} else {
				s0.setZero();
			}
			
			w1 = (v2 - v1).norm() + (v2 + v1).norm() / 2;
			w2 = (v0 - x1).norm() + (v0 + x1).norm() / 2;
			if (w1 + w2 > 0) {
				s1.noalias() = (w1 * v0 + w2 * v1) / (w1 + w2);
			} else {
				s0.setZero();
			}
		}

		////////////////////////////////////////////////////////////////////////


		m = m_start;

		v0 = M(m); m += step;//i-2
		v1 = M(m); m += step;//i-1
		
		for (size_t i = 2; i + 2 < n; ++i) {
		
			auto mi = M(m);
			let mi1 = M(m + step);
			
			w1 = (mi1 - mi).norm() + (mi1 + mi).norm() / 2;
			w2 = (v1 - v0).norm() + (v1 + v0).norm() / 2;

			if (w1 + w2 > 0) {
				v2.noalias() = (v1 * w1 + mi * w2) / (w1 + w2);
			} else {
				v2.setZero();
			}
			v0 = v1;
			v1 = mi;
			mi = v2;

			m += step;
		}


		m = m_start;
		M(m) = s0;
		M(m+step) = s1;
		////////////////////////////////////////////////////////////////////////
		td = get_data(n-1);
		{
			auto y0 = M(td); td -= step;
			auto y1 = M(td); td -= step;
			auto y2 = M(td); td -= step;
			auto y3 = M(td); td -= step;

			let t0 = t[n - 1];
			let t1 = t[n - 2];
			let t2 = t[n - 3];
			let t3 = t[n - 4];

			//absolutely the same block as above
			v0.noalias() = (y1 - y0) / (t1 - t0);
			v1.noalias() = (y2 - y1) / (t2 - t1);
			v2.noalias() = (y3 - y2) / (t3 - t2);
			x1.noalias() = 2 * v0 - v1;
			x2.noalias() = 2 * x1 - v0;
			w1 = (v1 - v0).norm() + (v1 + v0).norm() / 2;
			w2 = (x1 - x2).norm() + (x1 + x2).norm() / 2;
			if (w1 + w2 > 0) {
				s0.noalias() = (w1 * x1 + w2 * v0) / (w1 + w2);
			} else {
				s0.setZero();
			}
			w1 = (v2 - v1).norm() + (v2 + v1).norm() / 2;
			w2 = (v0 - x1).norm() + (v0 + x1).norm() / 2;
			if (w1 + w2 > 0) {
				s1.noalias() = (w1 * v0 + w2 * v1) / (w1 + w2);
			} else {
				s1.setZero();
			}
		}
		////////////////////////////////////////////////////////////////////////
		m = get_data(n - 1) + m_dim;
		M(m) = s0;
		M(m - step) = s1;
	}

	static size_t find_idx(const Real* t_arr, const size_t sz, Real t)
	{
		if (t <= t_arr[0])return 0;
		if (t >= t_arr[sz-1])return sz - 2;//cannot return the last one

		auto* it = std::lower_bound(t_arr, t_arr + sz, t);

		return (size_t)(it - t_arr - 1);
	};

	struct internal_point
	{
		void init(const Real time, const Real* t_arr, size_t idx_) 
		{
			idx = idx_;
			let t0 = t_arr[idx];
			let t1 = t_arr[idx + 1];
			
			dt = t1 - t0;
			t = (time - t0) / dt;
		}

		size_t idx;
		Real t, dt;
	};

	void get(Real* dst, const internal_point& d) const
	{
		auto* p0 = get_data(d.idx);
		auto* p1 = p0 + stride();
		auto* m0 = p0 + m_dim;
		auto* m1 = p1 + m_dim;

		M(dst) =
			(1 - d.t) * (1 - d.t) *
			(M(p0) * (1 + 2 * d.t) + d.dt * M(m0) * d.t)
			+ d.t * d.t *
			(M(p1) * (3 - 2 * d.t) + d.dt * M(m1) * (d.t - 1));
	}

	void get2(Real* dst, const Real time, const Real* t_arr,  size_t idx) const
	{
		internal_point kp;
		kp.init(time, t_arr, idx);
		get(dst, kp);
	}
};
