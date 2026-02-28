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
#include "parser_state.h"
#include "oper_block.h"
#include "ims_keywords.h"
#include "ims_identifiers.h"
#include "built_in_func.h"

#ifdef _MSC_VER

#pragma warning( disable : 4244)
#pragma warning( disable : 4459)
#pragma warning( disable : 4458)
#pragma warning( disable : 4702)
#pragma warning( disable : 4100)

#endif // _MSC_VER

#ifndef NDEBUG
//#define BOOST_SPIRIT_DEBUG 1
#endif


#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/support_utree.hpp>

namespace client
{
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace spirit = boost::spirit;


struct expr
{
	template <typename T1, typename T2 = void>
	struct result { using type = void; };

	expr(char q): op(q) {};

	void operator()(spirit::utree& t, spirit::utree const& rhs) const
	{
		let sym = spirit::utf8_symbol_type(&op, 1);
		let w = t.which();


		if (w != spirit::utree_type::list_type || t.front() != sym) {
			spirit::utree lhs;
			lhs.swap(t);
			t.push_back(sym);
			t.push_back(lhs);
		};

		t.push_back(rhs);
	}
	const char op;
};

struct negate_expr
{
	template <typename T1, typename T2 = void>
	struct result { using type = void; };

	void operator()(spirit::utree& expr, spirit::utree const& rhs) const
	{
		expr.clear();
		expr.push_back(spirit::utf8_symbol_type("n", 1));
		expr.push_back(rhs);
	}
};

struct vec_expr
{
	template <typename T1, typename T2 = void>
	struct result { using type = void; };

	vec_expr(char q): op(q) {};

	void operator()(spirit::utree& t, spirit::utree const& rhs) const
	{
		let w = t.which();

		if (w == spirit::utree_type::invalid_type) {
			t.push_back(spirit::utree(spirit::utf8_symbol_type(&op, 1)));
		}
		t.push_back(rhs);

	}

	void operator()(spirit::utree& t) const
	{
		let w = t.which();

		if (w == spirit::utree_type::invalid_type) {
			t.push_back(spirit::utree(spirit::utf8_symbol_type(&op, 1)));
		}
	}
	const char op;
};


struct arith_expr
{
	template <typename T1, typename T2 = void>
	struct result { using type = void; };

	arith_expr(char p1, char p2, bool i): op1(p1), op2(p2), inv(i) {};

	void operator()(spirit::utree& t, const spirit::utree& rhs) const
	{

		let w = t.which();

		using S = spirit::utf8_symbol_type;

		if (w != spirit::utree_type::list_type || t.front() != S(&op1, 1)) {
			spirit::utree lhs;
			lhs.swap(t);
			t.push_back(S(&op1, 1));
			t.push_back(lhs);
		}
		t.push_back(S(inv ? &op2 : &op1, 1));
		t.push_back(rhs);
	}

	const char op1, op2;
	bool inv;

};

#if 0
struct i64_expr
{
	template <typename T1, typename T2 = void>
	struct result { using type = int64_t; };



	void operator()(spirit::utree& t, const int64_t& rhs) const
	{
		t = spirit::binary_string_type(&rhs, 1);
	}
};
#endif

using fexpr = boost::phoenix::function<expr>;
static const boost::phoenix::function<negate_expr> neg;
static const fexpr sunion = expr('|');
static const fexpr powop = expr('^');
static const fexpr modop = expr('%');
static constexpr char s_sym_vector = 'v';
static constexpr char s_sym_index = 'i';
static constexpr char s_sym_funct = 'f';
static constexpr char s_sym_field = 'g';
static const boost::phoenix::function<vec_expr> vect(s_sym_vector);
static const boost::phoenix::function<vec_expr> idxe(s_sym_index);
static const boost::phoenix::function<vec_expr> fune(s_sym_funct);
static const boost::phoenix::function<vec_expr> field(s_sym_field);
static const boost::phoenix::function<arith_expr> times = arith_expr('*', '/', false);
static const boost::phoenix::function<arith_expr> divide = arith_expr('*', '/', true);
static const boost::phoenix::function<arith_expr> plus = arith_expr('+', '-', false);
static const boost::phoenix::function<arith_expr> minus = arith_expr('+', '-', true);
//static const boost::phoenix::function<i64_expr> s_i64_expr;
///////////////////////////////////////////////////////////////////////////////
//  grammar
///////////////////////////////////////////////////////////////////////////////
template <typename Iterator>
struct oper_grammar: qi::grammar<Iterator, ascii::space_type, spirit::utree()>
{
	oper_grammar(): oper_grammar::base_type(expression)
	{

		using qi::char_;
		using qi::short_;
		using qi::int_;
		using qi::double_;
		using qi::alpha;
		using qi::digit;
		using qi::_val;
		using qi::_1;


		expression = hsum[_val = _1] >> *('|' >> hsum[sunion(_val, _1)]);

		hsum = hmul[_val = _1] >>
			*(('+' >> hmul[plus(_val, _1)]) |('-' >> hmul[minus(_val, _1)]));

		hmul = factor[_val = _1] >>
			*(	('*'>>factor[times(_val, _1)])	|
				('/'>>factor[divide(_val, _1)]) |
				('%' >> factor[modop(_val, _1)])
				);

		factor = xfactor | simple;

		xfactor = simple[_val = _1] >> +('^' >> simple[powop(_val, _1)]);

		//order is important!
		simple =
			//p_int64_t[s_i64_expr(_val, _1)] |
			('-' >> xfactor[neg(_val, _1)]) |
			strict_double[_val = _1] |
			int_[_val = _1] |
			index[_val = _1] |
			field_call[_val = _1] |
			function_call[_val = _1] |
			identifier[_val = _1] |
			vector[_val = _1] |
			('(' >> expression[_val = _1] >> ')') |
			('-' >> simple[neg(_val, _1)]) |
			('+' >> simple[_val = _1])
			;

		////////////////////////////////////////////////////////////////////////

		identifier = qi::as_string[(alpha | char_('_') | char_('$')) >>
			*(alpha | digit | char_('_') | char_('$'))];

		vector = '[' >>  ( (expression[vect(_val, _1)] % ',') |
			qi::eps[vect(_val)]) >> *char_(',') >> ']';

		callable_or_indexable =
			identifier
			| vector
			//| ('(' >> expression >> ')') //VERY slow on PipeTree
			

			//| (identifier >> '(' >> -(expression % ',') >> ')')
			//| (identifier >> +('[' >> expression % ',' >> ']'))
			;

		//expr_suffix_call = '(' >> -(expression[fune(_val, _1)] % ',') >> ')';
		//expr_suffix_index = +('[' >> expression[idxe(_val, _1)] % ',' >> ']');

		function_call = callable_or_indexable[fune(_val, _1)]
			>> '(' >> -(expression[fune(_val, _1)] % ',') >> ')';

		index = (function_call | callable_or_indexable)[idxe(_val, _1)]
			>> +('[' >> expression[idxe(_val, _1)] % ',' >> ']');

		field_call = callable_or_indexable[field(_val, _1)] >>
			'.' >> -(identifier[field(_val, _1)] % '.');

#ifndef NDEBUG
		BOOST_SPIRIT_DEBUG_NODE(expression);
		BOOST_SPIRIT_DEBUG_NODE(hmul);
		BOOST_SPIRIT_DEBUG_NODE(simple);
		BOOST_SPIRIT_DEBUG_NODE(hsum);
		BOOST_SPIRIT_DEBUG_NODE(vector);
		BOOST_SPIRIT_DEBUG_NODE(identifier);
		BOOST_SPIRIT_DEBUG_NODE(function_call);
		BOOST_SPIRIT_DEBUG_NODE(field_call);
		BOOST_SPIRIT_DEBUG_NODE(index);
		BOOST_SPIRIT_DEBUG_NODE(factor);
		BOOST_SPIRIT_DEBUG_NODE(xfactor);
		BOOST_SPIRIT_DEBUG_NODE(callable_or_indexable);
		//BOOST_SPIRIT_DEBUG_NODE(expr_suffix_call);
		//BOOST_SPIRIT_DEBUG_NODE(expr_suffix_index);
		
#endif


	}

	qi::int_parser< int64_t, 10, 1, 19> p_int64_t;
	qi::real_parser<double, qi::strict_real_policies<double>> strict_double;

	qi::rule<Iterator, ascii::space_type, spirit::utree()>
		expression, hmul, hsum, simple, vector, identifier,
		factor, xfactor, function_call, field_call, index, tindex, tindex2,
		//, expr_suffix_call, expr_suffix_index,
		callable_or_indexable;

};
}


static bool is_int32(const boost::spirit::utree& ut, int32_t& v)
{

	using T = boost::spirit::utree_type;

	let w = ut.which();

	if (w == T::double_type) {

		let d = ut.get<double>();
		v = static_cast<int32_t>(d);

		if (v == d) {
			return true;
		}
	} else if (w == T::int_type) {
		let d = ut.get<int>();
		v = static_cast<int32_t>(d);

		if (v == d) {
			return true;
		}
	}
	return false;
};

static bool reter()
{
	return false;
}

static constexpr std::string_view INV_NARG = "invalid number of arguments";

//parse the operator with index x in array arr
static bool parse_operator(
	const boost::spirit::utree& ut, 
	parser_state& pfo, 
	oper_block& block, 
	size_t x)
{
	
	namespace spirit = boost::spirit;
	

	auto& a = block.m_ops;

	a[x].hdr.clear();

	let tp = ut.which();
	switch (tp) {
	case spirit::utree_type::list_type:
	{
		let& f = ut.front();
		let w = f.which();
		if (w != spirit::utree_type::symbol_type) {
			pfo.err << "invalid list";
			return reter() ;
		}

		let sz = ut.size();

		let str = f.get<spirit::utf8_symbol_range_type>();
		let c = str.front();


		ETYPE t;

		switch (c) {
		case '|':
		{
			t = ETYPE::uni;
			auto ar = block.add_args(x, t, sz - 1);
			for (auto it = std::next(ut.begin()); it != ut.end(); ++it) {
				if (!parse_operator(*it, pfo, block, ar++)) {
					return reter() ;
				}
			}
			return true;
		}
		case '*':
		{


			//rational: *int32/int32

			//we need to find the number of factors
			auto beg = ut.begin();

			size_t na = 0;



			bool prev_numer = false;//the previous one was "*int32"
			for (auto it = beg; it != ut.end();) {

				let op = it->get<spirit::utf8_symbol_range_type>().front();
				let vit = std::next(it);

				int32_t v32;
				bool b32 = is_int32(*vit, v32);

				if (op == '/' &&  b32 && prev_numer) {//found a rational
					prev_numer = false;
				} else {
					na += 1;
					prev_numer = (op == '*' &&  b32);
				}

				it = std::next(vit);
			}

			////////////////////////////////////////////////////////////////////
			size_t ar;
			if (na == 1) {//product of one factor
				ar = x;
			} else {
				ar = block.add_args(x, ETYPE::mul, na);
			}


			size_t idx = 0;

			prev_numer = false;//the previous one was "*int32"
			int32_t prev_32 = 0;
			for (auto it = beg; it != ut.end();) {

				let op = it->get<spirit::utf8_symbol_range_type>().front();
				let vit = std::next(it);

				int32_t v32;
				bool b32 = is_int32(*vit, v32);

				if (op == '/' &&  b32 && prev_numer) {
					prev_numer = false;
					// insert rational prev_32/v32
					block.set_rational(ar + idx, prev_32, v32);
					++idx;
				} else {

					if (prev_numer) {
						//insert integer prev_32
						block.set_integer(ar + idx, prev_32);
						++idx;
					}

					if (op == '/') {

						//insert inv(cur)

						let apos = block.add_args(ar + idx, ETYPE::inv, 1);

						if (b32) {
							block.set_integer(apos, v32);
						} else {
							if (!parse_operator(*vit, pfo, block, apos)) {
								return reter() ;
							}
						}
						++idx;
						prev_numer = false;
					} else {
						if (b32) {
							//don't insert, delay
							prev_32 = v32;
							prev_numer = true;
						} else {
							//insert cur
							if (!parse_operator(*vit, pfo, block, ar + idx)) {
								return reter() ;
							};

							++idx;
							prev_numer = false;
						}

					}
				}

				it = std::next(vit);
			}

			if (prev_numer) {//insert integer prev_32
				
				block.set_integer(ar + idx, prev_32);
				++idx;
			}
			assert(idx == na);


			return true;
		}
		case '+':
		{
			assert(sz % 2 == 0);
			auto ar = block.add_args(x, ETYPE::sum, sz / 2);

			for (auto it = ut.begin(); it != ut.end();) {

				let op = it->get<spirit::utf8_symbol_range_type>().front();
				let vit = std::next(it);

				if (op == '-') {
					let apos = block.add_args(ar, ETYPE::neg, 1);
					if (!parse_operator(*vit, pfo, block, apos)) {
						return reter() ;
					}
				} else {
					if (!parse_operator(*vit, pfo, block, ar)) {
						return reter() ;
					}
				}

				ar++;

				it = std::next(vit);
			}
			return true;
		}

		case 'n':
		{
			let apos = block.add_args(x, ETYPE::neg, 1);
			if (!parse_operator(*std::next(ut.begin()), pfo, block, apos)) {
				return reter() ;
			}
			return true;
		}
		case client::s_sym_vector:
		{

			let vbeg = std::next(ut.begin());
			size_t vsz = ut.size() - 1;

			auto ar = block.set_vector(x, vsz);

			for (auto it = vbeg; it != ut.end(); ++it) {
				if (!parse_operator(*it, pfo, block, ar++)) {
					return reter() ;
				}
			}

			if (vsz > 0) {
				block.adjust_vector(x);
			}

			return true;
		}
		case client::s_sym_field:
		case client::s_sym_funct:
		{
			//function call
			auto ts = c == client::s_sym_funct ?
				ESUBTYPE::call_normal : ESUBTYPE::call_fields;

			let fn = std::next(ut.begin());
			let r = fn->get<spirit::utf8_symbol_range_type>();
			std::string func_name(r.begin(), r.end());
			let nit = std::next(fn);

			let na = sz - 2;

			let bt = built_in_func::from_string(func_name);
			if (bt != BUILTIN_FUNC::invalid) {
				//found a built-in function
				if (na != built_in_func::num_args(bt)) {
					pfo.err << INV_NARG;
					return reter() ;
				}
				auto ar = block.add(na);
				a[x].hdr.tt = ETYPE::call_built_in;
				a[x].hdr.set_builtin_func((size_t)bt);
				a[x].hdr.set_u32(ar);

				for (auto it = nit; it != ut.end(); ++it) {
					if (!parse_operator(*it, pfo, block, ar++)) {
						return reter() ;
					}
				}

			} else if (func_name == ims_keywords::condition) {
				if (na % 2 == 0) {
					pfo.err << INV_NARG;
					return reter();
				}
				auto ar = block.add_args(x, ETYPE::condition, na);
				for (auto it = nit; it != ut.end(); ++it) {
					if (!parse_operator(*it, pfo, block, ar++)) {
						return reter();
					}
				}
			} else if (func_name[0] == ims_keywords::builtin) {

				func_name.erase(0, 1);


				let th = ims_operator::from_string(func_name);
				switch (th) {

				case ETYPE::call:
				{
					if ((na & 1) == 0) {//odd number of arguments
						pfo.err << INV_NARG;
						return reter();
					};

					auto ar = block.add_args(x, th, na);
					a[x].hdr.ts = ESUBTYPE::call_new;

					size_t arg_pos = 0;
					for (auto it = nit; it != ut.end(); ++it, ++arg_pos) {
						//each odd argument should be an identifier
						if ((arg_pos & 1) && it->which() == spirit::utree_type::string_type) {
							let sr = it->get<spirit::utf8_symbol_range_type>();
							let unk_id = pfo.unk->get_unk_id({ sr.begin(), sr.end() });
							auto& h = a[ar + arg_pos].hdr;
							h.tt = ETYPE::unk_reference;
							h.set_u32(unk_id);
						} else {
							if (!parse_operator(*it, pfo, block, ar + arg_pos)) {
								return reter();
							}
						}
					}
					break;
				}

				case ETYPE::thickness:
				{
					if (na != 1) {
						pfo.err << INV_NARG;
						return reter();
					};

					auto ar = block.add_args(x, th, 1);

					for (auto it = nit; it != ut.end(); ++it) {
						if (!parse_operator(*it, pfo, block, ar++)) {
							return reter();
						}
					}
					break;
				}

				case ETYPE::color_style:
				{
					if (na > 2 || na == 0) {
						pfo.err << INV_NARG;
						return reter() ;
					};

					if (na == 1) {
						auto ar = block.add_args(x, ETYPE::color_style, 1);

						for (auto it = nit; it != ut.end(); ++it) {
							if (!parse_operator(*it, pfo, block, ar++)) {
								return reter();
							}
						}
						break;
					}
				

					ims_warning("Deprecated $style found");

					//composition of color and thickness
					auto ar_mul = block.add_args(x, ETYPE::mul, 2);
					auto it = nit;
					{
						auto ar = block.add_args(ar_mul, ETYPE::color_style, 1);
						if (!parse_operator(*it++, pfo, block, ar)) {
							return reter();
						}
					}
					{
						auto ar = block.add_args(ar_mul+1, ETYPE::thickness, 1);
						if (!parse_operator(*it++, pfo, block, ar)) {
							return reter();
						}
					}
				
					break;
				}
				case ETYPE::vector_func:
				case ETYPE::charpoly:
				case ETYPE::companion:
				{
					
					size_t ar = block.add_args(x, th, na);
					a[x].hdr.ts = ts;
					for (auto it = nit; it != ut.end(); ++it) {
						if (!parse_operator(*it, pfo, block, ar++)) {
							return reter() ;
						}						
					}
					break;
				}
				case ETYPE::diagonal:
				{
					if (na != 1) {
						pfo.err << INV_NARG;
						return reter() ;
					}

					size_t ar = block.add_args(x, th, na);

					for (auto it = nit; it != ut.end(); ++it) {
						if (!parse_operator(*it, pfo, block, ar++)) {
							return reter() ;
						}
					}
					break;
				}
				case ETYPE::csg:
				{
					if (na != 4) {
						pfo.err << INV_NARG;
						return reter();
					}

					size_t ar = block.add_args(x, th, na);

					for (auto it = nit; it != ut.end(); ++it) {
						if (!parse_operator(*it, pfo, block, ar++)) {
							return reter();
						}
					}
					break;
				}
				case ETYPE::exchange:
				{
					if (na != 0) {
						pfo.err << INV_NARG;
						return reter() ;
					}
					block.set_exchange(x);
					break;
				}
				case ETYPE::inversion:
				{
					if (na != 0) {
						pfo.err << INV_NARG;
						return reter();
					}
					block.set_mobius(x);
					break;
				}
				case ETYPE::set_interval:
				{
					let ar = block.add_args(x, th, 1);

					if (na == 0) {
						block.set_distribution_def(ar);
					} else if (na == 1) {
						if (!parse_operator(*nit, pfo, block, ar)) {
							return reter() ;
						}
					} else {
						pfo.err << INV_NARG;
						return reter() ;
					}
					break;
				}
				case ETYPE::set_semigroup:
				{
					if (na == 0 || na > 2) {
						pfo.err << INV_NARG;
						return reter() ;
					}

					//partially process semigroups - create new variables
					//	&R = $semigroup([s,r])
					//		==========>
					//	&R = $semigroup(O)
					//	O = [s, r]

					let ar = block.add_args(x, th, 1);//semigroup

					let unk_id = pfo.unk->create_unique_identifier("O");
					a[ar].hdr.set_reference(unk_id, ESUBTYPE::ref_unknown);

					let offset = block.add_var(pfo.m_pos2, unk_id, false);

					if (!parse_operator(*nit, pfo, block, offset)) {
						return reter() ;
					}

					break;

				}
				case ETYPE::set_vector:
				case ETYPE::set_binary:
				{
					if (na > 2) {
						pfo.err << INV_NARG;
						return reter() ;
					};

					let ar = block.add_args(x, th, 1);

					//size first
					if (na == 0) {
						a[x].hdr.set_i24(0);
					}else if (nit->which() == spirit::utree_type::int_type) {
						let v = nit->get<int>();
						a[x].hdr.set_i24(v);
					} else {
						pfo.err << "invalid argument";
						return reter() ;
					}


					if (na <2) {
						block.set_distribution_def(ar);
					} else {
						if (!parse_operator(*std::next(nit), pfo, block, ar)) {
							return reter() ;
						}
					} 
					break;

				}
				case ETYPE::distribution_real:
				case ETYPE::distribution_int:
				{
					if (na > 2) {
						pfo.err << INV_NARG;
						return reter() ;
					}

					ESUBTYPE st;
					if (na == 0) {
						st = ESUBTYPE::dist_normal_def;
					} else if (na == 1) {
						st = ESUBTYPE::dist_normal;
					} else {
						st = ESUBTYPE::dist_uniform;
					}

					double v[2] = { 0,0 };

					auto it = nit;
					for (size_t i = 0; i < na; ++i) {
						let wit = it->which();
						if (wit == spirit::utree_type::int_type) {
							v[i] = it->get<int>();
						} else if (wit == spirit::utree_type::double_type) {
							v[i] = it->get<double>();
						} else {
							pfo.err << "invalid argument";
							return reter() ;
						}
						++it;
					}

					block.set_distribution(x, th, st, v[0], v[1]);

					break;
				}
				default:
					
					pfo.err << "Invalid identifier: "<< func_name;
					return reter() ;
				}
			} else {

				size_t ar = block.add_args(x, ETYPE::call, na + 1);
				a[x].hdr.ts = ts;

				let unk_id = pfo.unk->get_unk_id(func_name);
				a[ar++].hdr.set_reference(unk_id, ESUBTYPE::ref_unknown);

				if (c == client::s_sym_funct) {
					for (auto it = nit; it != ut.end(); ++it) {
						if (!parse_operator(*it, pfo, block, ar++)) {
							return reter();
						}
					}
				} else {
					//list of fields
					for (auto it = nit; it != ut.end(); ++it) {
						if (it->which() != spirit::utree_type::string_type) {
							pfo.err << "Invalid field in " << func_name;
							return reter();
						}
						let rit = it->get<spirit::utf8_symbol_range_type>();
						std::string field_name(rit.begin(), rit.end());

						let ref_field = pfo.unk->get_unk_id(field_name);
						a[ar++].hdr.set_small_int((int32_t)ref_field);
					}
				}
				


				
			}

			return true;
		}

		case '^':
		{
			let it_first = std::next(ut.begin());
			auto it = std::prev(ut.end());
			size_t ar = x;
			for (; it != it_first; --it) {
				if (it->which() == spirit::utree_type::int_type) {
					ar = block.set_power(ar, it->get<int>());
				} else {
					ar = block.set_power_ref(ar);
					if (!parse_operator(*it, pfo, block, ar + 1)) {
						return reter() ;
					}
				}
			}
			if (!parse_operator(*it, pfo, block, ar)) {
				return reter() ;
			}

			return true;
		}
		case '%':
		{
			auto ar = block.add_args(x, ETYPE::mod, sz - 1);
			for (auto it = std::next(ut.begin()); it != ut.end(); ++it) {
				if (!parse_operator(*it, pfo, block, ar++)) {
					return reter();
				}
			}
			return true;
		}
		case client::s_sym_index:
		{
			let it_x = std::next(ut.begin());
			auto it_y = std::next(it_x);

			if (sz==3 && it_y->which() == spirit::utree_type::int_type &&
				ims_operator::is_i24(it_y->get<int>())) 
			{
				let ar = block.add_args(x, ETYPE::index_imm, 1);
				a[x].hdr.set_i24(it_y->get<int>());
				if (!parse_operator(*it_x, pfo, block, ar)) {
					return reter() ;
				};
			} else {
				let ar = block.add_args(x, ETYPE::index, sz-1);
				if (!parse_operator(*it_x, pfo, block, ar)) {
					return reter() ;
				}
				
				for (size_t i = 1; i < sz-1; ++i) {
					if (!parse_operator(*it_y, pfo, block, ar + i)) {
						return reter();
					};
					it_y = std::next(it_y);
				}
				
			}

			return true;
		}

		default:
			pfo.err << "invalid op type";
			return reter() ;
		}

		assert(false);
		return reter() ;
	}
	case spirit::utree_type::double_type:
	{
		block.set_double(x, ut.get<double>());
		break;
	};
	case spirit::utree_type::int_type:
	{
		let v = ut.get<int>();
		block.set_integer(x, v);
		break;
	};
	case spirit::utree_type::string_type:
	{
		let r = ut.get<spirit::utf8_string_range_type>();
		std::string str(r.begin(), r.end());


		//TODO: rework?
		if (str[0] == ims_keywords::builtin) {

			str.erase(0, 1);
			if (str.empty()) {
				a[x].hdr.tt = ETYPE::this_vector;
				break;
			}

			let th = ims_operator::from_string(str);
			switch (th) {
			case ETYPE::distribution_int:
			case ETYPE::distribution_real:
				block.set_distribution(x, th, ESUBTYPE::dist_normal_def, 0, 0);
				break;
			case ETYPE::empty:
				a[x].hdr.set_xempty();
				break;
			case ETYPE::id:
				a[x].hdr.set_id();
				break;
			case ETYPE::color_style:
				a[x].hdr.set_color();
				break;
			case ETYPE::thickness:
				a[x].hdr.set_thickness();
				break;
			default:
				pfo.err << "Invalid builtin: "<< str;
				return reter() ;
			}
			break;
		}

		//parse the reference to the variable str
		a[x].hdr.set_reference(
			pfo.unk->get_unk_id(str), ESUBTYPE::ref_unknown);

		break;
	}
	case spirit::utree_type::symbol_type:
	{
		//let r = ut.get<spirit::utf8_symbol_range_type>();
		//std::string str(r.begin(), r.end());
		assert(false);
		return reter() ;
	}
	default:
		assert(false);
		return reter() ;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////



bool parser_state::parse7(
	std::string_view str, 
	oper_block& block, 
	size_t x)
{
	using boost::spirit::ascii::space;

	client::oper_grammar<std::string_view::const_iterator>  gram;

	auto iter = str.begin();
	auto end = str.end();

	//TODO: It might be better to reuse
	boost::spirit::utree ut;

	bool r = phrase_parse(iter, end, gram, space, ut);

	if (!r || iter != end) {
		err << "Error at: " << std::string(iter, end);
		return false;
	}
#if 0
	std::cout << "-------------------------\n";
	std::cout << "Parsing succeeded: " << ut << "\n";
	std::cout << "-------------------------\n";
	std::cout << ut << std::endl;
#endif

	return parse_operator(ut, *this, block, x);

}
