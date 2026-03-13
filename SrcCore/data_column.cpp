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
#include "data_column.h"
#include "ims_num_traits.h"
#include "search_info.h"
#include "oper_block.h"
#include "ims_chrono.h"


static void add_str(data_column::back_iter& str, const char* s)
{
	while (*s) { str = *s++; }
}

static void card2string(data_column::back_iter& str, cardinality c) {

	switch (c) {
	case cardinality::empty:		add_str(str, "0");	break;
	case cardinality::point:		add_str(str, "1");	break;
	case cardinality::finite:		add_str(str, ">1"); break;
	case cardinality::countable:	add_str(str, "oo"); break;
	case cardinality::nzero:		add_str(str, "#");	break;
	case cardinality::infmes:		add_str(str, "##"); break;
	case cardinality::error:		add_str(str, "e"); break;
	}
};
static char con2string(connectedness c) {

	switch (c) {
	case connectedness::disconnected:	return '-';
	case connectedness::weak:			return ' ';
	case connectedness::regular:		return '.';
	case connectedness::positive:		return '+';
	case connectedness::strong:			return '#';
	}
	assert(false);
	return 0;
};

const data_column::arr data_column::g_cols =
{ {

{
	"CH",
	"Checked",
	option::none | option::def_vis,
	false,//def_order
	[](let& b, let&)->Integer {return  b.m_flags.checked ? 1 : 0; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"HD",
	"Hidden",
	option::none,
	false,//def_order
	[](let& b, let&)->Integer {return  b.m_flags.hidden ? 1 : 0; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},

{
	"Name",
	"User-defined name",
	option::none | option::def_vis,
	true,//def_order
	nullptr,
	nullptr,
	//tostring
	[](let& b, let&, auto&& str)
	{
		if (!b.m_name.empty())
		{
			fmt::format_to(str, "{}", b.m_name);
			return;
		}
		if (b.has_id()) {
			fmt::format_to(str, "{}", b.str_id4());
			return;
		}

		fmt::format_to(str, "@{}", b.m_block_id);
	},
	{ {0,0},{0,0} },
},
{
	"Graph",
	"Graph ID",
	option::none,
	true,//def_order
	nullptr,
	nullptr,
	[](let& b, let&, auto&& str)
	{
		fmt::format_to(str, "{}", b.get_graph_id());
	},
	{ {0,0},{0,0} },
},
{
	"ID",
	"ID",
	option::contains_id,
	true,//def_order
	[](let& b, let&)->Integer {return b.m_block_id; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"GCX",
	"Complexity",
	option::need_search | option::def_vis,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_ir.m_gcx; },
	nullptr,
	nullptr,//tostring
	{ {0,4000},{0,0} },
},

{
	"NGV",
	"Neighbor Graph: vertices",
	option::need_search | option::def_vis,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_num_ver; },
	nullptr,
	nullptr,//tostring
	{ {0,100},{0,0} },
},
{
	"NGE",
	"Neighbor Graph: edges",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_num_edg; },
	nullptr,
	nullptr,//tostring
	{ {0,100},{0,0} },
},
{
	"NGC",
	"Neighbor Graph: components",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_num_comp; },
	nullptr,
	nullptr,//tostring
	{ {0,100},{0,0} },
},
{
	"CT",
	"Connection Type",
	option::need_search | option::def_vis,
	false,//def_order
	[](let&, let& u)->Integer {return (size_t)u.m_data.con; },
	nullptr,
	[](let&, let& u, auto&& str)
	{
		str = con2string(u.m_data.con);
	},
	{{ 2,4},{0,0} },
},
{
	"NDIM2",
	"Number of dimensions",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.get_ndim(0); },
	nullptr,
	nullptr,//tostring
	{ {0,100},{0,0} },
},

{
	"DIM2",
	"Boundary Dimension",
	option::need_search | option::def_vis,
	true,//def_order
	nullptr,
	[](let&, let& u)->Real {return u.get_dim(0); },
	nullptr,//tostring
	{ {0,0},{0,1.8},},
},
{
	"INFM2",
	"Infinite measure (boundary)",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return (size_t)u.get_inter_mes(0); },
	nullptr,
	[](let&, let& u, auto&& str)
	{
		let m = (size_t)u.get_inter_mes(0);
		if (m > 0) {
			fmt::format_to(str, "{}", m);
		}
	},
	{ {0,2},{0,0} },
},
{
	"DIM3",
	"Dim inter by 3",
	option::need_search,
	true,//def_order
	nullptr,
	[](let&, let& u)->Real {return u.get_dim(1); },
	nullptr,//tostring
	{ {0,0},{0,1.8}},
},
{
	"INFM3",
	"Infinite measure (inter by 3)",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return (size_t)u.get_inter_mes(1); },
	nullptr,
	[](let&, let& u, auto&& str)
	{
		let m = (size_t)u.get_inter_mes(1);
		if (m > 0) {
			fmt::format_to(str, "{}", m);
		}
	},
	{{ 0,2},{0,0} },
},

{
	"Z2",
	"Zero cardinality (inter by 2)",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return (size_t)u.get_cardinality(0); },
	nullptr,
	[](let&, let& u, auto&& str)
	{
		card2string(str, u.get_cardinality(0));
	},
	{ {0,2},{0,0} },
},
{
	"Z3",
	"Zero cardinality (inter by 3)",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return (size_t)u.get_cardinality(1); },
	nullptr,
	[](let&, let& u, auto&& str)
	{
		card2string(str, u.get_cardinality(1));
	},
	{ {0,2},{0,0} },
},
{
	"SDIM2",
	"Subspace dimension (inter by 2)",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_poly_dim2; },
	nullptr,
	[](let&, let& u, auto&& str)
	{
		
		let do_print = std::abs(u.m_poly_dim2 - u.m_dim) > 
			ims_num_traits<search_info::Real>::almost_zero();

		if (do_print) {
			fmt::format_to(str, "{}", u.m_poly_dim2);
		}
	},
	{ {0,0},{0,0} },
},
{
	"SM2",
	"Subspace measure (inter by 2)",
	option::need_search,
	false,//def_order
	[](let&, let& u)->Integer {return u.m_poly_mes2; },
	nullptr,
	[](let&, let& u, auto&& str)
	{
		let do_print = std::abs(u.m_poly_dim2 - u.m_dim) >
			ims_num_traits<search_info::Real>::almost_zero();

		if (do_print){
			fmt::format_to(str, "{}", u.m_poly_mes2);
		}
	},
	{ {0,0},{0,0} },
},
{
	"LDIM2",
	"Linear dimension (inter by 2)",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_ldim2; },
	nullptr,
	nullptr,
	{ {0,0},{0,0} },
},
#if 0
{
	"CT3",
	"Boundary Connectedness",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_data.ct3; },
	nullptr,
	[](let&, let& u, auto&& str)
	{
		if (u.m_data.ct3)str << "1";
	},
	{ {0,0},{0,0} },
},
#endif
{
	"DPT",
	"Search Depth",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_ir.m_depth; },
	nullptr,
	nullptr,//tostring
	{ {0,5},{0,0} },
},
{
	"FLI",
	"First level intersections",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_cnum; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"MELP",
	"Maximum of equal linear parts",
	option::need_search | option::early_calc,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_melp; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"NBH",
	"Number of neighborhoods",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_num_neighb; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"DIM",
	"Dimension",
	option::need_search | option::def_vis,
	false,//def_order
	nullptr,
	[](let&, let& u)->Real {return u.m_dim; },
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"DA",
	"Dimension algebraic",
	option::none,
	false,//def_order
	[](let& b, let&)->Integer {return b.get_dim(); },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},

{
	"DP",
	"Dimension proj",
	option::need_search | option::early_calc,
	false,//def_order
	[](let&, let& u)->Integer {return u.m_dim_proj; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},

{
	"SIM",
	"Self-similar",
	option::need_search,
	false,//def_order
	[](let&, let& u)->Integer {return u.m_data.all_sim?1:0; },
	nullptr,
	[](let&, let& u, auto&& str)
	{
		if (u.m_data.all_sim)str = '1'; 
	},
	{ {0,0},{0,0} },
},

{
	"ORI",
	"Orientations",
	option::need_search,
	false,//def_order
	[](let&, let& u)->Integer {return u.m_num_orientations; },
	nullptr,
	[](let&, let& u, auto&& str)
	{
		if (u.m_num_orientations < u.s_inf_orientations) {
			fmt::format_to(str, "{}", u.m_num_orientations);
		}
	},
	{ {3,search_info::s_inf_orientations},{0,0} },
},
{
	"RF",
	"Reflections",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_data.has_reflections; },
	nullptr,
	[](let&, let& u, auto&& str)
	{
		if (u.m_data.has_reflections)str = '1';
	},	
	{ {0,0},{0,0} },
},
{
	"RM",
	"Reflected maps",
	option::need_search | option::early_calc,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_refl; },
	nullptr,
	[](let&, let& u, auto&& str)
	{
		if (u.m_refl > 0) {
			fmt::format_to(str, "{}", u.m_refl);
		}
	},
	{{ 0,1000},{0,0} },
},
{
	"AR2",
	"Aspect Ratio^2",
	option::need_search,
	false,//def_order
	nullptr,
	[](let&, let& u)->Real {return u.get_aspect_ratio(); },
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"R2",
	"Radius^2",
	option::need_search,
	true,//def_order
	nullptr,
	[](let&, let& u)->Real {return u.m_max_rad; },
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"NR",
	"Number of Radiuses",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_num_rad; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"SID",
	"Structure ID",
	option::need_search | option::contains_id,
	true,//def_order
	[](let&, let& u)->Integer {return u.topo_id(); },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"MID",
	"Metric ID",
	option::need_search | option::contains_id,
	true,//def_order
	[](let&, let& u)->Integer {return u.get_mid();},
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"NMID",
	"Same Metric IDs",
	option::need_search,
	false,//def_order
	[](let&, let& u)->Integer {return u.num_isomers(); },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"BISO",
	"Best isomer",
	option::need_search,
	false,//def_order
	[](let& b, let& u)->Integer {return u.get_best() == &b ? 1 : 0; },
	nullptr,
	[](let& b, let& u, auto&& str)
	{
		if (u.get_best() == &b)str = '+';
	},
	{ {0,0},{0,0} },
},

{
	"OVL",
	"Overlapped",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_ir.m_over_depth; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},

{
	"OVF",
	"Overflowed",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_ir.m_overflowed; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"ARTH",
	"Arithmetic",
	option::need_search,
	true,//def_order
	nullptr,
	nullptr,
	[](let&, let& u, auto&& str)
	{
		switch (u.m_ir.m_mode) {
		case intersect_mode::rational:		fmt::format_to(str, "int"); break;
		case intersect_mode::big_rational:	fmt::format_to(str, "big"); break;
		case intersect_mode::real:			fmt::format_to(str, "real"); break;
		};
	},
	{ {0,0},{0,0} },
},
{
	"BITS",
	"Used bits",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_ir.m_bits; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"PRC",
	"Precision",
	option::need_search,
	false,//def_order
	nullptr,
	[](let&, let& u)->Real {return u.m_prec; },
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"MUT",
	"Number of mutations",
	option::need_search | option::def_vis,
	false,//def_order
	[](let&, let& u)->Integer {return u.m_num_mut; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"SMUT",
	"Number of successful mutations",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_num_mut_success; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"CM",
	"Changed maps",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_changed_maps; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"PRT",
	"Prototype ID",
	option::need_search | option::contains_id,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_proto_id; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0}},
},
{
	"GEN",
	"Generation",
	option::need_search,
	true,//def_order
	[](let&, let& u)->Integer {return u.m_generation; },
	nullptr,
	nullptr,//tostring
	{ {0,0},{0,0} },
},

{
	"TIME",
	"When the element was found",
	option::none,
	true,//def_order
	[](let& b, let&)->Integer {return (Integer)b.m_timestamp; },
	nullptr,
	[](let& b, let&, auto&& str) 
	{
		if (b.m_timestamp == 0)return;

		ims_chrono::fmt_time_t_to_hmsdmY t(static_cast<time_t>(b.m_timestamp / 1000));

		if (t.buf[0]) {
			fmt::format_to(str, "{}", t.buf);//(b.m_timestamp % 1000);			
		}else {//strange number
			fmt::format_to(str, "{}", b.m_timestamp);
		}

	},
	{ {0,0},{0,0} },
},


#if defined(DEVELOPER_VERSION)
{
	"_LDIM2",
	"Performance",
	option::need_search,
	true,//def_order
	nullptr,
	[](let&, let& u)->Real {return u.m_calc_time.ldim2; },
	nullptr,//tostring
	{ {0,0},{0,0} },
},
{
	"_R2",
	"Performance",
	option::need_search,
	true,//def_order
	nullptr,
	[](let&, let& u)->Real {return u.m_calc_time.r2; },
	nullptr,//tostring
	{ {0,0},{0,0} },
},
#endif

} };

bool data_column::is_same(const oper_block* b1, const oper_block* b2) const
{
	let* u1 = b1->m_calc_data.get();
	let* u2 = b2->m_calc_data.get();

	if (is_need_search() && (!u1 || !u2)) {
		return false;
	}
	if (get_int) {
		return get_int(*b1, *u1) == get_int(*b2, *u2);
	} else if (get_float) {
		let d1 = get_float(*b1, *u1);
		let d2 = get_float(*b2, *u2);
		return std::abs(d1 - d2) < ims_num_traits<Real>::almost_zero();
	}
	assert(to_string);
	static std::string v1, v2;
	to_string(*b1, *u1, std::back_inserter(v1));
	to_string(*b2, *u2, std::back_inserter(v2));
	return v1 == v2;
}

template<typename Real>
void prn_num_ex2(const Real& v, std::string& s)
{
	let& e = ims_num_traits<Real>::almost_zero();
	let iv = std::round(v);
	if (std::abs(iv - v) < e) {
		fmt::format_to(std::back_inserter(s), "{}", (int64_t)iv);
	}else {
		fmt::format_to(std::back_inserter(s), "{}", v);
	}
};


void data_column::get_column_str(
	const oper_block& sr,
	std::string& s,
	bool raw) const
{
	s.clear();
	let* u = sr.m_calc_data.get();

	if (is_need_search() && !u) {
		s += "-";
		return;
	};

	if (raw || !to_string) {
		if (get_int) {
			fmt::format_to(std::back_inserter(s), "{}", get_int(sr, *u));
			return;
		}
		if (get_float) {
			prn_num_ex2(get_float(sr, *u), s);
			return;
		};
	};

	assert(to_string);
	to_string(sr, *u, std::back_inserter(s));
}
