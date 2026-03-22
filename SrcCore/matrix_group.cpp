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
#include "matrix_group.h"
#include "matrix_funcs.h"


template<typename Integer>
size_t get_degree(const DynMat<Integer>& m, size_t max_degree)
{
	//maximum modulus of matrix elements
	static const Integer s_max_coeff = 100000;

	DynMat<Integer> T[2];
	T[0].resize(m.rows(), m.cols());
	T[1].resize(m.rows(), m.cols());
	T[0] = m;
	for (size_t p = 1; p < max_degree; ++p) {
		auto& Q = T[(p - 1) & 1];


		if (is_zero(Q) || is_id(Q)) {
			return p;
		}

		auto& Qn = T[p & 1];
		if (!mul_checked(Qn, Q, m)) {
			return 0;//overflow
		}


		if (norm_max(Qn) > s_max_coeff) {
			return 0;
		}
	};
	return 0;//degree is too large

};


void matrix_group::clear()
{
	m_gens.clear();
	m_elems.clear();
	m_hash.clear();
}

void matrix_group::add_generator(const Matrix& m)
{
	//you can't add generators after init
	assert(m_elems.empty());

	let idx = m_gens.size();

	auto& q = m_gens.emplace_back();


	q.idx = idx;

	q.m = m;

	to_normal_form_d(q.m);
}


void matrix_group::sort_gens()
{
	std::sort(m_gens.begin(), m_gens.end(), [](let& e1, let& e2) {
		return less(e1.m,e2.m);
	});
}

void matrix_group::init()
{
	m_overflow = false;

	assert(!m_gens.empty());//should have been added in add_generator

	assert(m_elems.empty());
	assert(m_hash.empty());

	for (auto& q : m_gens) {
		q.deg = get_degree(q.m, factor::max_degree);
	};

	let dim = m_gens[0].m.rows();

	m_temp.resize(dim, dim);

	//the identity matrix must always be in the table
	{
		m_temp.setIdentity();
		H.idx = 0;
		H.weight = 0;
		H.factors.clear();
		auto res = m_hash.insert({ m_temp,H });
		assert(res.second);
		m_elems.emplace_back(&*res.first);
	};

	
	m_tempM.resize(dim, dim);

	//add all the powers of finite-order generators
	//as well as the first powers of infinite-order generators
	for (size_t i = 0; i < m_gens.size(); ++i) {
		auto& g = m_gens[i];
		if (g.deg == 1)continue;//identity matrix

		//maximum degree to add
		let d = g.deg > 0 ? g.deg - 1 : 1;

		//first power of the generator
		m_temp = g.m;

		for (size_t j = 1; j <= d; ++j) {

			H.weight = 1;
			H.factors.clear();
			H.factors.emplace_back(i, j);
			auto res = m_hash.insert({ m_temp, H });
			if (res.second) {
				m_elems.emplace_back(&*res.first);

				///////////////////////////////////
				auto& v = res.first->second;
				if (m_weights.size() < v.weight) {
					m_weights.resize(v.weight);
				}
				auto& wa = m_weights[v.weight - 1];
				v.idx = wa.size();
				wa.emplace_back(&*res.first);
				///////////////////////////////////

			}


			if (j < d) {
				mul_x(m_tempM, m_temp, g.m);
				m_temp = m_tempM;
				to_normal_form_d(m_temp);
			}
		};
	};



	m_i = 1;
	m_j = 1;
	m_k = 0;
}

const matrix_group::elem_info* matrix_group::find(const Matrix& m) const
{
	auto it = m_hash.find(m);
	if (it == m_hash.end()) {
		return nullptr;
	}
	return &it->second;
}

bool matrix_group::finite() const
{
	return m_i >= m_elems.size();
}

bool matrix_group::extend()
{
	if (finite() || m_overflow) {
		return false;
	}

	assert(m_j <= m_i);
	assert(m_i < m_elems.size());

	let& gi = *m_elems[m_k == 0 ? m_i : m_j];
	let& gj = *m_elems[m_k == 0 ? m_j : m_i];

	if (!mul_checked(m_temp, gi.first, gj.first)) {
		m_overflow = true;
		return false;
	}


	to_normal_form_d(m_temp);

	let& A = gi.second.factors;
	let& B = gj.second.factors;

	//create an analytical product
	auto& F = H.factors;
	F = A;
	for (auto it = B.begin(); it != B.end(); ++it) {

		if (F.empty() || F.back().idx != it->idx) {
			F.insert(F.end(), it, B.end());
			break;//the rest of B
		}

		auto& q = F.back();


		q.pw += it->pw;
		if (q.pw >= std::numeric_limits<typename factor::type>::max() / 2) {
			m_overflow = true;
			return false;
		}


		let d = m_gens[q.idx].deg;
		if (d > 0) {
			q.pw %= d;
		}

		if (q.pw == 0) {
			F.pop_back();

		}
	}

	H.weight = 0;
	for (let& q : F) {
		let d = m_gens[q.idx].deg;
		H.weight += d > 0 ? 1 : q.pw;
	}


	////////////////////////////////////////////////////////////////////////

	auto res = m_hash.insert({ m_temp ,H });

	auto* rf = &*res.first;
	auto& v = rf->second;
	let ow = v.weight;

	if (res.second) {//a new element was inserted
		m_elems.emplace_back(rf);
		assert(H.weight > 0);
		///////////////////////////////////

		if (m_weights.size() < ow) {
			m_weights.resize(ow);
		}
		auto& wa = m_weights[ow - 1];
		v.idx = wa.size();
		wa.emplace_back(rf);
		///////////////////////////////////

	} else {//checking, maybe we found a better element

		let oidx = v.idx;

		let weight_chnaged = H.weight < ow;

		if (weight_chnaged ||
			(H.weight == ow && H.factors.size() < v.factors.size()))
		{
			assert(H.weight > 0);

			if (weight_chnaged) {
				//remove from the old array
				auto& wa = m_weights[ow - 1];
				if (oidx + 1 != wa.size()) {
					wa[oidx] = wa.back();
					wa[oidx]->second.idx = oidx;
				}
				wa.pop_back();

				//insert at the end of the new array
				if (m_weights.size() < H.weight) {
					m_weights.resize(H.weight);
				}
				auto& wn = m_weights[H.weight - 1];
				H.idx = wn.size();
				wn.emplace_back(rf);
			} else {
				H.idx = oidx;
			}

			v = H;

		}
	};



	////////////////////////////////////////////////////////////////////////
	++m_k;
	if (m_k == 2) {
		m_k = 0;
		++m_j;
		if (m_j > m_i) {
			++m_i;
			m_j = 1;
		}
	}

	return true;
}

size_t matrix_group::matrix_hash::operator()(const Matrix& e) const
{
	return get_hash(e);
}

matrix_group::factor::factor(size_t i, size_t p)
{
	idx = static_cast<type>(i);
	pw = static_cast<type>(p);
}
