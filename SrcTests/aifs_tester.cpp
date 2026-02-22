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
#include "aifs_tester.h"
#include "ims_val.h"
#include "oper_block.h"
#include "block_class.h"
#include "ifs_data_text.h"
#include "pool_ptr.h"
#include "variable.h"
#include "block_graph.h"
#include "edge_ball.h"
#include "edge_map.h"
#include "aifs_load.h"

void print_operator(
	const ifs_list& lst,
	std::ostream& str,
	const operator_ptr& ptr,
	const ETYPE par_type,
	const char* fmt);

void print_ims_val_ex(std::ostream& str, const ims_val* v, const ifs_list* lst);


template<typename T>
bool aifs_tester::is_arr(std::string_view var, ims_val_b::ETP t, const std::initializer_list<T> arr)
{
	let* v = eval(var);

	constexpr auto sb = ims_val::get_subtype<T>();
	if (!v || !v->is(t, sb)) {
		return false;
	}

	if (v->num_el() != arr.size()) {
		return false;
	}

	let* p = v->gp<T>();
	for (let q : arr) {
		if constexpr (sb == ims_val_b::EST::real) {
			if (std::abs(q - *p) > eps) { return false; }
		} else {
			if (q != *p) { return false; }
		}
		p++;
	}

	return true;
}

aifs_tester::aifs_tester(std::string_view v)
{
	aifs = v;
}

operator_ptr aifs_tester::get_var_ptr(const oper_block& b, std::string_view var_name)
{
	let* g = b.get_class();

	for (let& q : b) {
		if (g->get_var_name(q.gr()) == var_name) {
			return b.get_ptr(q.pos5);
		}
	}

	return {};
}

std::string aifs_tester::get_def(std::string_view var)
{
	let* b = get_last_block();
	let p = get_var_ptr(*b, var);

	std::stringstream dst;
	print_operator(nfo->m_list, dst, p, ETYPE::min_priority, nullptr);
	return dst.str();
}

const oper_block* aifs_tester::get_block(std::string_view id)
{
	return nfo->m_list.find_block(id);
}

const oper_block* aifs_tester::get_last_block() const
{
	if (nfo->m_list.empty())return nullptr;
	return nfo->m_list.m_id2data[nfo->m_list.m_blocks.back()].b.get();
}

const variable& aifs_tester::get_var(std::string_view name) const
{
	let* block = get_last_block();
	let idx = block->get_class()->find_var_by_name(name);
	assert(idx != ims_max);
	return block->ctx()->get_ref4(idx);
}

bool aifs_tester::is_closed(std::string_view name) const
{
	let* block = get_last_block();
	let idx = block->get_class()->find_var_by_name(name);
	assert(idx != ims_max);
	return block->get_graph()->closed2(idx);
}

std::string aifs_tester::eval_as_str(std::string_view var, bool is_geom /*= true*/)
{
	let* v = eval(var, is_geom);

	std::stringstream dst;
	print_ims_val_ex(dst, v, &nfo->m_list);
	return dst.str();
}

bool aifs_tester::equal_affine(std::string_view var, const std::initializer_list<ims_val_b::Rational> arr)
{
	return is_arr(var, ims_val_b::ETP::matrix, arr);
}

bool aifs_tester::approx_affine(std::string_view var, const std::initializer_list<ims_val_b::Real> arr)
{
	return is_arr(var, ims_val_b::ETP::matrix, arr);
}

bool aifs_tester::equal_vec(std::string_view var, const std::initializer_list<ims_val_b::Rational> arr)
{
	return is_arr(var, ims_val_b::ETP::vector, arr);
}


bool aifs_tester::init_ex()
{
	ims_err_reset();

	nfo = std::make_unique<ims_info>();

	std::stringstream str(aifs);
	using IIC = std::istreambuf_iterator<char>;
	auto nbeg = IIC(str);
	let nend = IIC();

	if (!ims_load7(*nfo, "", nbeg, nend, false)) {
		err_msg = ehb.get_buf();
		return false;
	}

	bi.clear_proj_data();

	return true;
}

bool aifs_tester::init()
{
	if (!init_ex()) {
		return false;
	}
	
	auto* b = const_cast<oper_block*>(get_last_block());
	if (!b)return true;

	bi.recalc_graph();
	if (!bi.init4(*b, ec, am, gid.get(), ac)) {
		return false;
	}

	//the block must be inheritable
	inh.clear();
	inh.inherit_from(*b, vp, cv, true);
	check_block(&inh);
	if (inh.is_invalid()) {
		return false;
	}

	return true;
}

const ims_val* aifs_tester::eval(std::string_view var, bool is_geom /*= true*/)
{
	let* b = get_last_block();
	let* g = b->get_class();
	let ref = g->find_var_by_name(var);
	if (ref == ims_max) {
		assert(false);
		return nullptr;
	}
	return ec.eval_ref(ref, is_geom);
}

bool aifs_tester::equal(std::string_view var, ims_val_b::Rational val)
{
	let* v = eval(var);
	if (!v || !v->is(ims_val_b::ETP::number, ims_val_b::EST::rational)) {
		return false;
	}
	return v->get_int() == val;
}

bool aifs_tester::approx(std::string_view var, ims_val_b::Real val)
{
	let* v = eval(var);
	if (!v || !v->is(ims_val_b::ETP::number, ims_val_b::EST::real)) {
		return false;
	}
	return std::abs(v->get_real() - val) <= eps;
}

bool aifs_tester::not_finite(std::string_view var)
{
	let* v = eval(var);
	
	if (!v || !v->is(ims_val_b::ETP::number, ims_val_b::EST::real)) {
		return false;
	}
	return !std::isfinite(v->get_real());
}

bool aifs_tester::approx_vec(std::string_view var, const std::initializer_list<ims_val_b::Real> arr)
{
	return is_arr(var, ims_val_b::ETP::vector, arr);
}
