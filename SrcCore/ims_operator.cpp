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
#include "built_in_func.h"
#include "ims_operator.h"
#include "ims_keywords.h"
#include "string_view_hash.h"

uint64_t ims_operator::as64() const
{
	return *reinterpret_cast<const uint64_t*>(this);
}

void ims_operator::clear()
{
	*reinterpret_cast<uint64_t*>(this) = 0;
	assert(is_xundef());
}


size_t ims_operator::get_u24() const
{
	return u24[0] | (u24[1] << 8) | (u24[2] << 16);
}

void ims_operator::set_u24(size_t v)
{
	assert(v < 0x1000000);

	u24[0] = v & 0xFF;
	u24[1] = (v >> 8) & 0xFF;
	u24[2] = (v >> 16) & 0xFF;
}

intptr_t ims_operator::get_i24() const
{
	let v = static_cast<intptr_t>(get_u24());
	if (v <= 0x7FFFFF) return v;
	return v - intptr_t(0x1000000);
}

bool ims_operator::is_i24(intptr_t v)
{
	return v < 0x800000 && -v <= 0x800000;
}

bool ims_operator::set_i24(intptr_t v)
{
	if (!is_i24(v)) {
		return false;
	}
	
	u24[0] = v & 0xFF;
	u24[1] = (v >> 8) & 0xFF;
	u24[2] = (v >> 16) & 0xFF;

	return true;
}

void ims_operator::set_i32(intptr_t v)
{
	i32 = static_cast<int32_t>(v);
}


void ims_operator::set_u32(size_t v)
{
	u32 = static_cast<uint32_t>(v);
}

////////////////////////////////////////////////////////////////////////////////
bool ims_operator::is_xempty() const {return tt == ETYPE::empty;}
void ims_operator::set_xempty(){	tt = ETYPE::empty;};
bool ims_operator::is_id() const { return tt == ETYPE::id; }
void ims_operator::set_id() { tt = ETYPE::id; }
void ims_operator::set_color() { tt = ETYPE::color_style; }
void ims_operator::set_thickness() { tt = ETYPE::thickness; }
void ims_operator::set_xundef() { tt = ETYPE::undef; }
bool ims_operator::is_xundef() const { return tt == ETYPE::undef; }


bool ims_operator::is_closed() const
{
	return false;//for the future (for example, a unit ball)
}

bool ims_operator::is_template() const
{
	return	tt >= ETYPE::template_first && tt <= ETYPE::template_last;
}

void ims_operator::set_small_int(int32_t v)
{
	tt = ETYPE::number_imm;
	ts = ESUBTYPE::integer;
	i32 = v;
}

void ims_operator::set_small_rational(int16_t n, int16_t d)
{
	tt = ETYPE::number_imm;
	ts = ESUBTYPE::rational;
	r16[0] = n;
	r16[1] = d;
}

bool ims_operator::is_distirib_def() const
{
	return tt == ETYPE::distribution_int && ts == ESUBTYPE::dist_normal_def;
}

intptr_t ims_operator::get_pow_exponent_imm() const
{
	assert(tt == ETYPE::power_imm);
	return get_i24();
}

intptr_t ims_operator::get_elem_index() const
{
	assert(tt == ETYPE::index_imm);
	return get_i24();
}

size_t ims_operator::get_builtin_args() const
{
	return built_in_func::num_args(get_builtin_func());
}

void ims_operator::set_reference(size_t offset, ESUBTYPE ts_)
{
	tt = ETYPE::reference;
	ts = ts_;
	set_u32(offset);
}

void ims_operator::set_offset(size_t offset)
{
	set_u32(offset);
}

void ims_operator::set(ETYPE t_, size_t primary, size_t secondary)
{
	tt = t_;
	set_u32(primary);
	set_u24(secondary);
}

bool ims_operator::get_int_imm(int64_t& numerator, int64_t& denominator) const
{
	assert(tt == ETYPE::number_imm);

	switch (ts) {

	case ESUBTYPE::integer:
		numerator = i32;
		denominator = 1;
		return true;
	case ESUBTYPE::rational:
		numerator = r16[0];
		denominator = r16[1];
		return true;
	default:
		return false;
	}
}

////////////////////////////////////////////////////////////////////////////////

ims_static std::array<std::string, (size_t)ETYPE::min_priority> s_keywords;
ims_static ankerl::unordered_dense::map <
	std::string, ETYPE, string_view_hash_dense, std::equal_to<> > s_map;

static void s_init() 
{
	static bool inited = false;
	if (inited) return;
	inited = true;

	for (size_t i = 0; i < s_keywords.size(); ++i) {
		auto& d = s_keywords[i];
		switch ((ETYPE)i) {
		case ETYPE::empty:				d = "e";		break;
		case ETYPE::id:					d = "i";		break;
		case ETYPE::set_interval:		d = "number";	break;
		case ETYPE::set_semigroup:		d = "semigroup";break;
		case ETYPE::set_vector:			d = "vector";	break;
		case ETYPE::set_binary:			d = "binary";	break;
		case ETYPE::distribution_int:	d = "integer";	break;
		case ETYPE::distribution_real:	d = "real";		break;
		case ETYPE::companion:			d = "companion";break;
		case ETYPE::vector_func:		d = "";			break;
		case ETYPE::charpoly:			d = "charpoly";	break;
		case ETYPE::diagonal:			d = "diagonal";	break;
		case ETYPE::exchange:			d = "exchange";	break;
		case ETYPE::csg:				d = "csg";		break;
		case ETYPE::inversion:			d = "inversion";break;
		case ETYPE::color_style:		d = "style";	break;
		case ETYPE::thickness:			d = "thickness";break;
		case ETYPE::condition:			d = "if";		break;
		case ETYPE::call:				d = "new";		break;
		default:										continue;
		}
		s_map[d] = static_cast<ETYPE>(i);
	}
}

std::string_view  ims_operator::to_string(ETYPE t)
{
	s_init();
	let& ret = s_keywords[static_cast<size_t>(t)];
	return ret;
}

ETYPE ims_operator::from_string(std::string_view str)
{
	s_init();

	auto it = s_map.find(str);
	if (it == s_map.end()) {
		return ETYPE::undef;
	}

	return it->second;
}

////////////////////////////////////////////////////////////////////////////////
ims_static const std::string s_kw[] =
{
	ims_keywords::subspace,
	ims_keywords::root,
	ims_keywords::section,
	ims_keywords::camera,
	ims_keywords::palette,
	ims_keywords::background,
	ims_keywords::colorize,
	ims_keywords::light,
	ims_keywords::time,
};

builtin_ids get_builtin_id(std::string_view name)
{
	for (size_t i = 0; i < std::size(s_kw); ++i) {
		if (s_kw[i] == name)return (builtin_ids)i;
	}
	return builtin_ids::num_ids;
}

std::string_view get_builtin_name(builtin_ids id)
{
	return s_kw[(size_t)id];
}

////////////////////////////////////////////////////////////////////////////////

void ims_operator::hash_combine(size_t& ret) const
{
	boost::hash_combine(ret, tt);

	switch (tt) {
	case ETYPE::reference:
	case ETYPE::unk_reference:
		boost::hash_combine(ret, get_offset());
		break;
	case ETYPE::number_imm:
		boost::hash_combine(ret, ts);
		boost::hash_combine(ret, u32);
		break;
	case ETYPE::set_vector:
	case ETYPE::set_binary:
	case ETYPE::call_built_in:
	case ETYPE::power_imm:
	case ETYPE::index_imm:
		boost::hash_combine(ret, get_i24());
		break;
	case ETYPE::distribution_int:
	case ETYPE::distribution_real:
		boost::hash_combine(ret, ts);
		break;
	default:
		break;//depends on additional data
	};
}
template<typename T>
bool cmp_int(T v1, T v2, intptr_t& res) 
{
	res = ((intptr_t)v1) - ((intptr_t)v2);
	return res != 0;
}

intptr_t ims_operator::lexic_compare(ims_operator h1, ims_operator h2)
{
	intptr_t v = 0;
	if (cmp_int(h1.tt, h2.tt, v))return v;
	switch (h1.tt) {
	case ETYPE::reference://call_offset is only important here
	case ETYPE::unk_reference:
		if (cmp_int(h1.get_offset(), h2.get_offset(), v))return v;
		break;
	case ETYPE::number_imm: 
		if (cmp_int(h1.ts, h2.ts, v))return v;
		if (cmp_int(h1.u32, h2.u32, v))return v;
		break;
	case ETYPE::set_vector:
	case ETYPE::set_binary:
	case ETYPE::call_built_in:
	case ETYPE::power_imm:
	case ETYPE::index_imm:
		cmp_int(h1.get_i24(), h2.get_i24(), v);
		break;
	case ETYPE::distribution_int:
	case ETYPE::distribution_real:
		if (cmp_int(h1.ts, h2.ts, v))return v;
		break;
	default:
		break;
	};
	return v;//depends on additional data
}

size_t ims_operator::data_args() const
{
	switch (tt) {
	case ETYPE::number:
		return 1;
	case ETYPE::distribution_int:
	case ETYPE::distribution_real:
		return 2;//distribution - the same layout as vector_imm[real, 2]
	case ETYPE::vector_imm:
		return num_args();
	default:
		break;
	};
	return 0;
}

size_t ims_operator::oper_args() const
{
	switch (tt) {
		//types without additional operator arguments
	case ETYPE::reference:
	case ETYPE::unk_reference:
	case ETYPE::number_imm:
	case ETYPE::undef:
	case ETYPE::number:
	case ETYPE::vector_imm:
	case ETYPE::exchange:
	case ETYPE::inversion:
	case ETYPE::id:
	case ETYPE::empty:
	case ETYPE::this_vector:
		//distribution - the same layout as vector_imm[real, 2]
	case ETYPE::distribution_int:
	case ETYPE::distribution_real:
		return 0;
	case ETYPE::power_imm:
	case ETYPE::index_imm:
	case ETYPE::neg:
	case ETYPE::inv:
	case ETYPE::charpoly:
	case ETYPE::set_interval://argument - distribution
	case ETYPE::set_vector:
	case ETYPE::set_binary:
	case ETYPE::color_style:
	case ETYPE::thickness:
	case ETYPE::set_semigroup://vector
		return 1;
	case ETYPE::power:
		return 2;
	case ETYPE::csg:
		return 4;
	case ETYPE::call_built_in:
		return get_builtin_args();
	case ETYPE::call:
	case ETYPE::mul:
	case ETYPE::sum:
	case ETYPE::uni:
	case ETYPE::vector:
	case ETYPE::vector_func:
	case ETYPE::companion:
	case ETYPE::diagonal:
	case ETYPE::index:
	case ETYPE::condition:
		return num_args();//dynamic number of operator arguments
	default:
		assert(false);
		return 0;
	};
}
