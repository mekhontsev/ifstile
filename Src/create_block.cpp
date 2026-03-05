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
#include "creator_state.h"
#include "block_form.h"
#include "oper_block.h"
#include "ims_identifiers.h"
#include "matrix_funcs.h"

void creator_state::create_block2(
	ims_identifiers& idf,
	oper_block& b,
	const ims_graph_base& ig,
	std::span<const intptr_t> pows,
	const RationalPoly& poly,
	const size_t cyc,
	const subspace_info* si)
{

	assert(!b.get_parent());

	let cgi = creator_state::get_group_info();

	if (cyc > 0) {
		b.m_dim2 = cgi[cyc].poly.size() - 1;
	} else {
		b.m_dim2 = poly.p.degree();
	};

	b.m_flags.clear();
	b.m_flags.has_dim = true;

	//b.m_flags.hidden = true;


	/*
	g=1+s^2-s^3
	s=$companion([1,-1,1,-1])
	r=$exchange()
	$subspace=[s,0]
	O=[1,r,s,s^2,s^3,s^4,s^5,s^6,s^7,s^8,s^9,(s*r),(s^2*r),(s^3*r),(s^4*r),
		(s^5*r),(s^6*r),(s^7*r),(s^8*r),(s^9*r)]
	&Q=O[$number($integer(0,19))]*$vector()
	h0=Q
	h1=Q
	h2=Q
	h3=Q
	h4=Q
	A0=g^-1*h0*A0|g^-1*h1*A0|g^-1*h2*A1
	A1=g^-1*h3*A0|g^-1*h4*A1
	*/

	//s,r may not exist
	//s is always $companion
	//r is $diagonal or $exchange
	//g can be a polynomial in s or $companion
	//Q, hi, Ai are always present


	std::vector<const creator_state::subspace_info::group_info*> group_gens;


	//number of group generators
	size_t num_s = 0;
	size_t num_r = 0;//number of additional group generators

	//additional group generator
	DynMat<int64_t> mat_r;
	bool is_exchange = false;

	if (cyc > 0) {
		num_s = 1;
		num_r = 1;
		is_exchange = true;
		get_exchange(mat_r, b.m_dim2);

	} else {

		std::vector<double> cells = { double(si->cell) };

		if (block_form::get_simple_reflection(mat_r, poly.p.degree()) &&
			block_form::check_additional_group(mat_r, poly.p.data(), cells))
		{
			num_r = 1;
		}

		for (let& g : si->group) {
			if (!g.dup) {
				group_gens.emplace_back(&g);
			}
		}

		num_s = group_gens.size();
	}


	b.clear_ops();
	
	auto make_ref = [&](size_t ops_idx, size_t ref)
	{
		b.m_ops[ops_idx].hdr.set_reference(ref);
	};

	size_t next_var = 0;
	uint32_t pos = 0;
	auto make_var = [&](std::string_view name, bool is_subs = false)
	{
		++next_var;
		return b.add_var(pos, idf.get_unk_id(name), is_subs);
	};


	////////////////////////////////////////////////////////////////////////

	//base matrix g: space allocation
	let base_idx = next_var;
	let base_ds = make_var("g");
	
	////////////////////////////////////////////////////////////////////////
	let s_idx = next_var;

	for (size_t i = 0; i < num_s; ++i) {

		std::string str_id = "s";
		if (num_s > 1)str_id += std::to_string(i);

		let ds = make_var(str_id);

		//right side

		if (cyc > 0) {//via the companion matrix
			let& p = cgi[cyc].poly;
			b.set_one_vector_arg(p, ds, ETYPE::companion);

		} else {//polynomial of g: sum of products
			let* gi = group_gens[i];
			b.set_int_poly(gi->p.p.data(), gi->p.d, ds, base_idx);
		}
	}
	////////////////////////////////////////////////////////////////////////
	let r_idx = next_var;
	if (num_r > 0) {
		assert(num_r == 1);

		let ds = make_var("r");

		//right side
		if (is_exchange) {
			b.set_exchange(ds);
		} else if (is_diag(mat_r)) {
			let n = (size_t)mat_r.rows();
			std::vector<int64_t> diag(n + 1);
			for (size_t k = 0; k < n; ++k) {
				diag[k] = mat_r(k, k);
			}
			diag[n] = 1;
			b.set_one_vector_arg(diag, ds, ETYPE::diagonal);
		} else {
			let psz = mat_r.rows() * mat_r.cols();
			size_t di = b.set_vector_ex(ds, ETYPE::vector_imm, ESUBTYPE::integer, (size_t)psz);
			//row-major
			for (size_t r = 0; r < (size_t)mat_r.rows(); ++r) {
				for (size_t c = 0; c < (size_t)mat_r.cols(); ++c) {
					b.m_ops[di++].i64 = mat_r(r, c);
				}
			}
		}
	}
	////////////////////////////////////////////////////////////////////////
	//base matrix :g
	{
		let ds = base_ds;

		//right side

		if (cyc == 0) {//companion matrix
			assert(poly.d == 1);
			b.set_one_vector_arg(poly.p.data(), ds, ETYPE::companion);
		} else {//polynomial of s: sum of products
			assert(num_s > 0);
			b.set_int_poly(poly.p.data(), poly.d, ds, s_idx);
		}
	}


	////////////////////////////////////////////////////////////////////////////
	{
		let ds = b.add_var(pos, builtin2ref(builtin_ids::subspace), false);
		let ar = b.add_args(ds, ETYPE::vector, 2);

		make_ref(ar, cyc > 0 ? s_idx : base_idx);
		b.m_ops[ar + 1].hdr.set_small_int(si->cell);
	}


	////////////////////////////////////////////////////////////////////////////
	//semigroup elements: array O
	let oref = next_var;
	let oidx = make_var("O");
	size_t num_el = 0;
	{

		////////////////////////////////////////////////////////////////////////
		//list of generators

		bool has_minus_one = false;//element -1 must be generated by generators

		std::vector<size_t> offsets;
		auto add = [&] {
			let a = b.m_ops.size();
			offsets.emplace_back(a);
			b.m_ops.emplace_back();
			return a;
			};


		b.set_integer(add(), 1);//identity

		auto make_elems = [&](size_t deg, size_t s_index)
		{
			if (deg > 0) {
				//s, s^2, ..., s^(deg-1)
				for (size_t i = 1; i < deg; ++i) {
					make_ref(b.set_power(add(), i), s_index);
				}
				if (num_r > 0) {//s*r, .... s^(deg-1)*r
					for (size_t i = 1; i < deg; ++i) {
						let ofs = b.add_args(add(), ETYPE::mul, 2);
						make_ref(b.set_power(ofs, i), s_index);
						make_ref(ofs + 1, r_idx);
					}
				}
				has_minus_one = true;
			} else {

				for (size_t mul = 0; mul < 2; ++mul) {

					auto u = [&] {return mul == 0 ? add() : b.set_neg(add()); };

					{//s
						make_ref(u(), s_index);
					}

					{//s^-1
						
						make_ref(b.set_power(u(), -1), s_index);
					}

					if (num_r > 0) {
						{//r*s*r
							let ofs = b.add_args(u(), ETYPE::mul, 3);
							make_ref(ofs, r_idx);
							make_ref(ofs + 1, s_index);
							make_ref(ofs + 2, r_idx);
						}
						{//r*s^-1*r
							let ofs = b.add_args(u(), ETYPE::mul, 3);
							make_ref(ofs, r_idx);
							make_ref(b.set_power(ofs + 1, -1), s_index);
							make_ref(ofs + 2, r_idx);
						}

						{//s*r
							let ofs = b.add_args(u(), ETYPE::mul, 2);
							make_ref(ofs, s_index);
							make_ref(ofs + 1, r_idx);
						}
						{//s^-1*r
							let ofs = b.add_args(u(), ETYPE::mul, 2);
							make_ref(b.set_power(ofs, -1), s_index);
							make_ref(ofs + 1, r_idx);
						}

						{//r*s
							let ofs = b.add_args(u(), ETYPE::mul, 2);
							make_ref(ofs, r_idx);
							make_ref(ofs + 1, s_index);
						}
						{//r*s^-1
							let ofs = b.add_args(u(), ETYPE::mul, 2);
							make_ref(ofs, r_idx);
							make_ref(b.set_power(ofs + 1, -1), s_index);
						}
					}
				}
			}
		};

		if (cyc > 0) {
			let deg = cgi[cyc].degree;
			make_elems(deg, s_idx);
		} else {
			for (size_t j = 0; j < group_gens.size(); ++j) {
				let& g = group_gens[j];
				make_elems(g->deg, s_idx + j);
			}
		}

		if (!has_minus_one) {
			b.set_integer(add(), -1);//-1
			if (num_r > 0) {//-r
				make_ref(b.set_neg(add()), r_idx);
			}
		}

		if (num_r > 0) {
			make_ref(add(), r_idx);//r
		}

		num_el = offsets.size();

		let a = b.set_vector(oidx, num_el);

		//copy the headings of the group elements
		for (size_t i = 0; i < num_el; ++i) {
			b.m_ops[a + i] = b.m_ops[offsets[i]];
		}

	}

	////////////////////////////////////////////////////////////////////////////
	//map template - group * shift :Q
	let qref = next_var;
	let qidx = make_var("Q", true);
	{
		let arx = b.add_args(qidx, ETYPE::mul, 2);
		let ar = b.add_args(arx, ETYPE::index, 2);
		make_ref(ar, oref);
		let dax = b.set_vector_template(ar + 1, ETYPE::set_interval);
		b.set_distribution(dax, ETYPE::distribution_int,
			ESUBTYPE::dist_uniform, 0, double(num_el) - 1);

		let da = b.set_vector_template(arx + 1, ETYPE::set_vector);
		b.set_distribution_def(da);
	}

	////////////////////////////////////////////////////////////////////////////
	//maps :h
	let maps_start = next_var;

	for (size_t i = 0; i < ig.m_edges.size(); ++i) {
		let ds = make_var(std::string("h") + std::to_string(i));
		make_ref(ds, qref);
	}


	////////////////////////////////////////////////////////////////////////////
	//graph :A
	let sets_start = next_var;

	for (size_t i = 0; i < ig.num_ver(); ++i) {

		let ds = make_var(std::string("A") + std::to_string(i));

		let ne = ig.num_edges(i);

		//union elements
		let ofs = b.add_args(ds, ETYPE::uni, ne);

		for (size_t j = 0; j < ne; ++j) {
			let& e = ig.get_edge(i, j);

			let pw = pows[e.m];

			auto a = b.add_args(ofs + j, ETYPE::mul, pw == 0 ? 2 : 3);
			if (pw != 0) {
				let pa = b.set_power(a, -pw);
				make_ref(pa, base_idx);
				++a;
			}

			make_ref(a, e.m + maps_start);
			make_ref(a + 1, e.second + sets_start);
		}
	}

}
