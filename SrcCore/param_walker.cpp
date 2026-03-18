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
#include "param_walker.h"
#include "ims_val.h"
#include "eval_pool.h"
#include "pool_ptr.h"

size_t param_walker::checked_size(const ims_val* d)
{
	if (!d || !d->is(ims_val_b::ETP::vector)) {
		return 0;
	}
	let sz = d->get_size();
	if (d->is(ims_val_b::EST::other)) {
		for (size_t i = 0; i < sz; ++i) {
			if (!d->p_v(i)) {
				return 0;
			}
		};
	}
	return sz;
}

param_walker::node_type param_walker::classify(const ims_val* d)
{
	if (!d || !d->is(ims_val_b::ETP::vector)) {
		return node_type::invalid;
	}
	let sz = d->get_size();
	if (sz == 0) {
		return node_type::invalid;
	}
	if (d->is(ims_val_b::EST::other)) {
		let* d0 = d->p_v(0);
		if (d0->is(ims_val_b::ETP::number) && sz > 1) {
			let* d1 = d->p_v(1);
			if (d1->is(ims_val_b::ETP::string)) {
				return node_type::drop_down;
			}
			if (d1->is(ims_val_b::ETP::vector)) {
				return node_type::array;
			}
		}

		if (d0->is(ims_val_b::ETP::string)) {
			return node_type::string;
		};

		if (d0->is(ims_val_b::ETP::vector)) {
			return node_type::struct_type;
		};
	}

	//numbers
	if (sz < 3 || sz > 4) {
		return node_type::invalid;
	}

	int64_t v64;
	if (sz == 3 && d->get_i64(v64, 0)) {
		return node_type::i64;
	}
	return node_type::f64;
}

bool param_walker::check_type(const ims_val* d)
{
	let sz = checked_size(d);
	if (sz == 0) {
		return false;
	}

	let* d0 = d->is(ims_val_b::EST::other) ? d->p_v(0) : nullptr;
	m_t = classify(d);

	switch (m_t)
	{
	case param_walker::node_type::invalid:
		return false;
	case param_walker::node_type::drop_down:
	{
		if (!d0->get_i64(m_i[0])) {
			return false;
		}
		m_i[1] = 0;
		m_i[2] = (int64_t)sz - 2;

		if (m_i[0] < m_i[1] || m_i[0] > m_i[2]) {
			return false;
		}
		for (size_t i = 2; i < sz; ++i) {
			if (!d->p_v(i)->is(ims_val_b::ETP::string)) {
				return false;
			}
		}
		m_s = ims_val_b::EST::rational;
		return true;
	}
	case param_walker::node_type::array:
	{
		if (!d0->get_i64(m_i[0])) {
			return false;
		}
		m_i[1] = 0;
		m_i[2] = 1024;

		if (m_i[0] < m_i[1] || m_i[0] > m_i[2]) {
			return false;
		}

		let et = classify(d->p_v(1));//element type
		if (et == param_walker::node_type::invalid) {
			return false;
		}
		switch (et)
		{
		case param_walker::node_type::drop_down:
		case param_walker::node_type::i64:
			m_s = ims_val_b::EST::rational;
			break;
		case param_walker::node_type::f64:
			m_s = ims_val_b::EST::real;
			break;
		default:
			m_s = ims_val_b::EST::other;
			break;
		}
		return true;
	}
	case param_walker::node_type::struct_type:
	{
		m_i[0] = (int64_t)sz;
		m_s = ims_val_b::EST::rational;
		for (size_t i = 0; i < sz; ++i) {
			let* entry = d->p_v(i);

			let szi = entry->get_size();
			if (szi != 2) {
				return false;
			}
			let* id = entry->p_v(0);
			if (!id || !id->is(ims_val_b::ETP::string)) {
				return false;
			};
			let* tp = entry->p_v(1);
			if (!tp || !tp->is(ims_val_b::ETP::vector)) {
				return false;
			};

			if (m_s != ims_val_b::EST::other) {
				let et = classify(d->p_v(1));//element type
				if (et == param_walker::node_type::invalid) {
					return false;
				}
				switch (et)
				{
				case param_walker::node_type::drop_down:
				case param_walker::node_type::i64:
					if (i == 0) {
						m_s = ims_val_b::EST::rational;
					} else if (m_s != ims_val_b::EST::rational) {
						m_s = ims_val_b::EST::other;
					}
					break;
				case param_walker::node_type::f64:
					if (i == 0) {
						m_s = ims_val_b::EST::real;
					} else if (m_s != ims_val_b::EST::real) {
						m_s = ims_val_b::EST::other;
					}
					break;
				default:
					m_s = ims_val_b::EST::other;
					break;
				}
			}
		}
		return true;
	}
	case param_walker::node_type::string:
		return true;
	case param_walker::node_type::i64:
	{
		m_e1 = d0 && d->p_v(1)->is_empty();
		m_e2 = d0 && d->p_v(2)->is_empty();

		//int numbers
		bool is_i64 = (sz == 3) && d->get_i64(m_i[0], 0);
		is_i64 = is_i64 && (m_e1 || d->get_i64(m_i[1], 1));
		is_i64 = is_i64 && (m_e2 || d->get_i64(m_i[2], 2));

		if (!is_i64) {
			return false;
		}
		if (!m_e1 && m_i[0] < m_i[1]) {
			return false;
		}
		if (!m_e2 && m_i[0] > m_i[2]) {
			return false;
		}
		return true;
	}
	case param_walker::node_type::f64:
	{
		m_e1 = d0 && d->p_v(1)->is_empty();
		m_e2 = d0 && d->p_v(2)->is_empty();

		//real numbers
		bool is_f64 = d->get_f64(m_r[0], 0);
		is_f64 = is_f64 && (m_e1 || d->get_f64(m_r[1], 1));
		is_f64 = is_f64 && (m_e2 || d->get_f64(m_r[2], 2));

		if (!is_f64) {
			return false;
		}
		if (!m_e1 && m_r[0] < m_r[1]) {
			return false;
		}
		if (!m_e2 && m_r[0] > m_r[2]) {
			return false;
		}

		return true;
	}
	default:
		return false;
	}
}

const ims_val* param_walker::create_def(const ims_val* d)
{
	switch (m_t) {
	case node_type::drop_down:
		return eval_pool::ep.get_scalar_int(m_i[0]);
	case node_type::array:
	case node_type::struct_type:
	{
		let def_size = (size_t)m_i[0];
		switch (m_s)
		{
		case ims_val_b::EST::rational:
			return eval_pool::ep.get_vector_int(def_size);
		case ims_val_b::EST::real:
			return eval_pool::ep.get_vector_real(def_size);
		default:
			return eval_pool::ep.get_vector(def_size);
		}
	}
	case node_type::string:
		return eval_pool::ep.get_string(d->p_v(0)->get_string());
	case node_type::i64:
		return eval_pool::ep.get_scalar_int(m_i[0]);
	case node_type::f64:
		return eval_pool::ep.get_scalar_real(m_r[0]);
	default:
		return nullptr;
	}
}

bool param_walker::process(const ims_val* d, pool_ptr& v, size_t rec_level, const std::function<Func>& f)
{
	bool reset_nested = !v;
	if (reset_nested) {
		v.reset(create_def(d));
	}
	let sz = d->get_size();
	switch (m_t)
	{
	case node_type::string:
	{
		//process
		if (f) {
			f(*this, rec_level, d, v);
		}
		return true;
	}
	case node_type::i64:
	{
		if (f) {
			f(*this, rec_level, d, v);
		}
		return true;
	}
	case node_type::f64:
	{
		if (f) f(*this, rec_level, d, v);
		return true;
	}
	case node_type::drop_down:
	{
		if (f) {
			f(*this, rec_level, d, v);
		}
		return true;
	}
	case node_type::array:
	{
		param_walker trec;
		let* di = d->p_v(1);
		if (!trec.check_type(di)) {
			return false;
		}

		//process array size

		size_t reset_from = v->get_size();
		if (f) {
			ims_val_b::Rational arr_sz{ v->get_size() };
			pool_ptr psz(
				eval_pool::ep.get_pointer(&arr_sz, ims_val_b::EST::rational));
			if (f(*this, rec_level, d, psz)) {
				let nsz = numerator(arr_sz);
				v.reset(eval_pool::ep.update_vec_size(v.get_mut(), size_t(nsz)));
			}
		}

		for (size_t i = 0; i < v->get_size(); ++i) {

			pool_ptr pvi(eval_pool::ep.get_pointer(v.get_mut(), i));
		
			if (reset_nested || i >= reset_from) {
				switch (m_s)
				{
				case ims_val_b::EST::rational:
					pvi.get_mut()->set_i64(trec.m_i[0]);
					break;
				case ims_val_b::EST::real:
					pvi.get_mut()->set_f64(trec.m_r[0]);
					break;
				case ims_val_b::EST::other:
					assert(!pvi);
					pvi.reset(trec.create_def(di));
					v->p_v()[i] = pvi.get();
					pvi->add_ref();
					break;
				default:
					break;
				}
			}

			//recursive calls
			if (!trec.process(di, pvi, rec_level + 1, f)) {
				return false;
			};

			if (m_s == ims_val_b::EST::other) {
				let* old = v->p_v(i);
				v->p_v()[i] = pvi.release();
				if (old)eval_pool::ep.release(old);
			}
		}
		return true;
	}
	case node_type::struct_type:
	{
		
		for (size_t i = 0; i < sz; ++i) {
			let* entry = d->p_v(i);

			param_walker trec;
			let* di = entry->p_v(1);
			if (!trec.check_type(di)) {
				return false;
			}

			pool_ptr pvi(eval_pool::ep.get_pointer(v.get_mut(), i));
			if (f) {
				f(*this, rec_level, entry, pvi);
			}

			if (reset_nested) {
				switch (m_s)
				{
				case ims_val_b::EST::rational:
					pvi.get_mut()->set_i64(trec.m_i[0]);
					break;
				case ims_val_b::EST::real:
					pvi.get_mut()->set_f64(trec.m_r[0]);
					break;
				case ims_val_b::EST::other:
					assert(!pvi);
					pvi.reset(trec.create_def(di));
					v->p_v()[i] = pvi.get();
					pvi->add_ref();
					break;
				default:
					break;
				}
			}

			//recursive call
			if (!trec.process(di, pvi, rec_level + 1, f)) {
				return false;
			};

			if (m_s == ims_val_b::EST::other) {
				let* old = v->p_v(i);
				v->p_v()[i] = pvi.release();
				if (old)eval_pool::ep.release(old);
			}

		}
		return true;
	}

	default:
	{
		return false;
	}
	}
}
