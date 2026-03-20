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
#include "ims_random.h"
#include "block_form.h"
#include "graph_poly.h"

ims_static const std::initializer_list<creator_state::group_info> 
g_creator_group_info
{	
	{
		"D2",
		{1,1},//x+1
		{ 0, },
		2,
	},

	{
		"D4",
		{1,0,1},//x^2+1
		{ 0 },
		4,
	},

	
	{
		"D6",
		{ 1,-1,1 },//x^2-x+1
		{ 0 },
		6,
	},

	
	{
		"D8",
		{ 1, 0, 0, 0, 1},//x^4+1
		{ 0 },
		8,
	},

	
	{
		"D10",
		{ 1,-1, 1,-1, 1 },//x^4-x^3+x^2-x+1
		{ 0 },
		10,
	},

	
	{
		"D12",
		{ 1, 0,-1, 0, 1 },//x^4-x^2+1	
		{ 0 },
		12,
	},

	{
		"D14",
		{ 1,-1, 1,-1, 1,-1, 1 },//x^6-x^5+x^4-x^3+x^2-x+1
		{ 0 },
		14,
	},

	
	{
		"D16",
		{ 1, 0, 0, 0, 0, 0 ,0, 0, 1 },//x^8+1
		{ 0 },
		16,
	},

	
	{
		"D18",
		{ 1, 0, 0,-1, 0, 0, 1 },//x^6-x^3+1
		{ 0 },
		18,
	},

	
	{
		"D20",
		{ 1, 0, -1, 0 , 1, 0, -1,0,1 },//x^8-x^6+x^4-x^2+1
		{ 0 },
		20,
	},

	
	{
		"D24",
		{ 1, 0, 0, 0 , -1, 0, 0,0,1 },//x^8-x^4+1
		{ 0 },
		24,
	},
	
	{
		"D30",
		{ 1, 1, 0, -1 , -1, -1, 0,1,1 },//x^8+x^7-x^5-x^4-x^3+x+1
		{ 0 },
		30,
	},

	
	{
		"78.98",
		{1,-3,3,-3,1},//x^4-3x^3+3x^2-3x+1 [0.190983005625053 + 0.981593343275320*i]
		{ 1 },
		0
	},

	
	{
		"111.47",
		{ 1,-2,0,-2,1 },//x^4-2x^3-2x+1  [-0.366025403784439 + 0.930604859102100*i]
		{ 1 },
		0
	},

	
	{
		"101.95",
		{ 1,-2,1,-2,1 },//x^4-2x^3+x^2-2x+1   [-0.207106781186548 + 0.978318343478516*i]
		{ 1 },
		0
	},

	
	{
		"130.64",
		{ 1,-1,-1,-1,1 },//x^4-x^3-x^2-x+1=0 [-0.651387818865997 + 0.758744956775990*i]
		{ 1 },
		0
	},


	{
		"32.93",
		{1,4,3,-8,3,4,1},//x^6+4x^5+3x^4-8x^3+3x^2+4x+1=0 [0.83928675521416113255 + 0.54368901269207636157*i]
		{ 1 },
		0
	},

	{
		"Tame",
		{ 2,3,2},//2x^2+3x+2
		{ 0 },
		0
	},
	{
		"Viper",
		{ 2,1,2 },//2x^2+x+2
		{ 0 },
		0
	},

};


ims_static const size_t s_max_group_order = 100;




void creator_state::clear_hash() 
{

	m_hash_base.clear();
	for (auto& v : m_hash_cyc) {
		v.clear();
	}


}

void creator_state::clear()
{
	m_cg.clear();	

	{
		std::scoped_lock lock(m_lock);
		m_found_poly.clear();

	}
	clear_hash();
}




bool creator_state::graph::parse(std::string_view line)
{
	
	clear();

	//remove spaces
	std::string lnsp(line);

	ims_erase(lnsp, [](char c) {return c <= ' ';});
	
	std::vector<std::string> str_ver;

	boost::split(str_ver, lnsp, boost::is_any_of("-"));

	std::vector<std::string> str_edg;


	for (size_t i = 0; i < str_ver.size(); ++i) {
		let& sv = str_ver[i];
		boost::split(str_edg, sv, boost::is_any_of("."));
		for (let& se : str_edg) {
			//[number]letter[number]
			let sz = se.length();
			size_t pos = sz;
			char c = 0;
			for (size_t j = 0; j < sz; ++j) {
				c = se[j];
				if (c >= 'a' && c <= 'z') {
					pos = j;
					break;
				}
			}
			if (pos >= sz) {
				return false;
			}
			c -= 'a';


			size_t num = 1;
			size_t pw = 1;

			if (pos > 0) {
				if (!boost::conversion::try_lexical_convert(se.substr(0, pos), num)) {
					return false;
				}
			}
			if (pos + 1 < sz) {
				if (!boost::conversion::try_lexical_convert(se.substr(pos + 1, sz - pos - 1), pw)) {
					return false;
				}
			};


			for (size_t j = 0; j < num; ++j) {
				m_ig2.create_edge(i, c, m_pows.size());
				m_pows.emplace_back(pw);
			}
			
		}
	}
	
	///////////////////////////////////////////////////////////////////////////

	m_ig2.init();

	size_t comp = ims_max;
	for (size_t i = 0; i < m_ig2.m_comp.size(); ++i) {
		let& c = m_ig2.m_comp[i];

		if (c.has_other() || c.countable) {
			continue;
		}
		
		comp = i;
		break;
	}

	if (comp == ims_max) {
		return false;
	}

	
	BigPoly poly;

	compute_graph_poly(poly.data(),m_ig2, comp, m_pows);
	if (poly.is_zero()) {
		return false;
	}
	m_graph_sp = poly_roots::max_positive_root<double>(poly);

	std::ostringstream oss;
	poly_func::print(poly.data().data(), poly.size(), false, oss);	
	m_str_poly = oss.str();


	return true;
}


void creator_state::found_poly::get_group_code(std::string& code) const
{
	if (cyc > 0) {
		code = creator_state::get_group_info()[cyc].name;
		return;
	}
	code=has_r ? "R" : "C";
	
	for (let& q : si) {
		for (let& g : q.group) {
			if (!g.dup) {
				code.append(std::to_string(g.deg));
				return;
			}
		}
	}
	code.append("2");
};



void creator_state::init_units()
{
	if (!m_units.empty())return;

	let cgi = creator_state::get_group_info();

	let sz = cgi.size();
	m_units.resize(sz);

	

	RootFinder rf;
	ims_polynomial<int64_t> poly;
	for (size_t i = 0; i < sz; ++i) {
		let& u = cgi[i];
		poly.data() = u.poly;
		rf.find(poly);
		m_units[i] = rf.m_relem;
	}
}



std::span<const creator_state::group_info> creator_state::get_group_info()
{
	return g_creator_group_info;
}

creator_state::PolyHash::value_type* creator_state::get_found(size_t idx) const
{
	std::scoped_lock lock(m_lock);
	if (idx >= m_found_poly.size())return nullptr;
	return m_found_poly[idx];
}


void creator_state::find_poly()
{
	init_units();

	{
		std::scoped_lock lock(m_lock);
		m_found_poly.clear();
	}

	m_num_checked=0;

	clear_hash();


	let cgi = creator_state::get_group_info();

	m_hash_cyc.resize(cgi.size());

	
	auto& irn = ims_random::getR();
	irn.seed();
	
	RationalPoly poly;
	poly.d = 1;
	
	RationalPoly t1, t2;

	//use local RootFinder
	RootFinder rf;

	//additional elements to insert (only for the base)
	std::vector<RationalPoly> stack;
	DynMat<int64_t> mat_r;


	let gsp = m_cg.m_graph_sp;
	

	let ar1 = std::pow(gsp, 1 / m_search_hdim_max);
	let ar2 = std::pow(gsp, 1 / m_search_hdim_min);

	//list of base groups for which an additional group search is required
	std::vector<PolyHash::value_type*> need_group;

	RootFinder::ElemType re;

	std::vector<size_t> intervals;

	enum class mode
	{
		//indicates that we are looking for a base polynomial (for a space of any dimension)
		base = 0,
		//indicates that we are looking for a base polynomial from a group (only for 2D space)
		base_from_pal = 1,
		//indicates that we are looking for a rotation polynomial from the base (only for 2D space)
		rot_from_base = 2,
	};

	mode m;

	
	
	//search for the root with the given module
	let eps = ims_num_traits<double>::almost_zero();

	while (!ims_need_stop()) {

		//base_from_pal
		size_t cyc = 0;

		//rot_from_base
		PolyHash::value_type* ng_el=nullptr;
		found_poly* ng = nullptr;

		m = static_cast<mode>(irn.rng() % 3);

		if (m == mode::rot_from_base) {
			if (need_group.empty())continue;
			ng_el = need_group[irn.rng() % need_group.size()];
			ng = ng_el->second.get();
		} else if (m == mode::base_from_pal) {
			
			cyc = irn.rng() % (cgi.size()-1) + 1;
		}

		if (m == mode::base && !stack.empty()) {
			poly = stack.back();
			stack.pop_back();
		}else{

			size_t n1=1, n2;

			if (m == mode::rot_from_base) {
				n1 = 1;
				n2 = 0;
				for (let& q : ng->r) {
					n2 += q.degree();
				}
				assert(n2 > 0);
				n2--;//the maximum degree is expressed through smaller ones
	
			}else if (m == mode::base_from_pal){
				n1 = 0;
				let deg = cgi[cyc].poly.size()-1;
				if (deg > m_search_poly_degree) {
					continue;
				}
				n2=deg-1;//the maximum degree is expressed through smaller ones
			} else {
				n1 = 1;
				n2 = m_search_poly_degree;
			}

			assert(n1 <= n2);
			
		

			size_t n = (irn.rng() % (n2 - n1 + 1)) + n1;
			poly.p.data().resize(n + 1);

		

			let is_monic = (m == mode::base && m_search_integer);

			for (size_t i = 0; i <= n; ++i) {
				if (is_monic && i == n) {
					poly.p[n] = 1;//monic
					break;
				}
				
				auto v = static_cast<int>(round(m_variance *irn.get_normal()));

				if (v == 0) {
					//the leading coefficient cannot be zero
					//0 must not be the root of the base coefficient
					if (i == n || (m == mode::base && i == 0)) {
						v = irn.rng() % 2 == 0 ? 1 : -1;
					}
				}
				poly.p[i] = v;
			}

			//x ^ 6 - x ^ 4 + x ^ 2 + 1
			//poly.p.data().resize(7);
			//poly.p.data() = {1,0,1,0,-1,0,1};



			if (m == mode::base || m_search_integer) {
				poly.d = 1;
			} else {
				auto v= static_cast<int>(round(m_variance *irn.get_normal()));
				poly.d = (v==0)?1: std::abs(v);
			}
	
		}

		poly.adjust();
		

	
		m_num_checked++;


		if (m == mode::rot_from_base) {
			
			for (auto& si : ng->si) {
				
				let idx = si.cell;
				re = ng->r[idx];
				re.action(poly.p, poly.d);

				//must be in the upper half-plane and with a modulus of 1
				if (re.r.imag() < eps || std::abs(re.ar - 1) > eps) {
					continue;
				}

				
				bool dup = false;
				for (let& q : si.group) {

					if (q.p.p == poly.p && q.p.d == poly.d) {
						dup = true;
						break;
					};
				}

				if (dup) {
					continue;
				}


				//from now on we insert in any case
				for (auto& q : si.group) {
				
					let n1 = poly_roots::is_pow_group(q.v, re.r, s_max_group_order);
					let n2 = poly_roots::is_pow_group(re.r, q.v, s_max_group_order);
					if (n1 > 0 && n2 > 0) {
						if (re.r.real() < q.v.real()) {
							dup = true;
							break;
						} else {
							q.dup = true;
						}
					} else if (n1 > 0) {//q.v=re.r^n1
						q.dup = true;
					} else if (n2 > 0) {//re.r=q.v^n2
						dup = true;
						break;
					}
				}

				si.group.emplace_back();

				auto& gi=si.group.back();

				gi.dup = dup;
				gi.deg = poly_roots::is_pow(std::complex<RealType>(1),
					re.r, s_max_group_order);
				gi.v = re.r;
				gi.p = poly;
			
				
			}
		
			
			continue;//branch completed
		}
		////////////////////////////////////////////////////////////////////////
		//from now on m != mode::rot_from_base

		if (m == mode::base) {
			if (poly.p.size() <= 1) {
				continue;//may be obtained after divisions
			}
			poly_func::adjust_signs(poly.p, false);
		}

		auto& h = m == mode::base ? m_hash_base : m_hash_cyc[cyc];

		if (h.find(poly) != h.end()) {
			continue;//already checked
		};

		//if there is space, insert
		if (h.size() < s_max_hash) {
			h.emplace(poly, nullptr);
		};

		std::unique_ptr<found_poly> fp;

		if (m == mode::base_from_pal) {
			let& nfo = cgi[cyc];
			let idx = nfo.subspace.begin()[0];
			re = m_units[cyc][idx];

			let s = re.r;//copy! root of unity
			re.action(poly.p, poly.d);

			if (re.r.imag() + eps < 0 || 
				re.ar < ar1 - eps || re.ar > ar2 + eps)
			{
				continue;
			}

		
			//insert all variants of polynomials taking into account multiplication by a primitive root
			RationalPoly b = poly;

			auto z = re.r;
			auto best_z = z;

			h.emplace(b, nullptr);//insert in any case

			let deg = cgi[cyc].degree;

			if (deg > 0) {
				for (size_t iter = 1; iter < deg; ++iter) {
					z *= s;

					std::span<const int64_t> sp = nfo.poly;
					b.d *= poly_func::mul_var(
						b.p.data(), 
						std::span<const int64_t>(nfo.poly));
					b.adjust();

#ifndef NDEBUG
					{
						//integrity check
						auto tr = m_units[cyc][idx];//copy?
						tr.action(b.p, b.d);
						assert(std::abs(tr.r - z) < eps);
					}
#endif // !NDEBUG

					h.emplace(b, nullptr);//insert all variants of polynomials

					if (z.imag() + eps > 0 &&
						(best_z.imag() + eps < 0 || best_z.real() < z.real())) {
						best_z = z;
						poly = b;
					}
				}
			} else {
				//infinite order

			}

		

			fp = std::make_unique<found_poly>();
			fp->si.resize(1);
			fp->si[0].cell = idx;
			fp->si[0].ifs_dim = log(gsp) / log(re.ar);
		} else {//base

			rf.find(poly.p);

			//fill the subspace
			rf.divide(ar1, ar2, intervals);

			if (intervals.empty()) {
				continue;
			}

		
			//The GCD with all previously found ones must be equal to 1
			bool ignore = false;
			for (size_t j = 0;; ++j) {
				auto* cb = get_found(j);
				if (!cb)break;
				if (cb->second->cyc > 0) {
					continue;
				}

				//check for polynomials x and -x
				for (size_t k = 0; k < 2; ++k) {
					let& p = cb->first;
					t1 = p;
					t2 = poly;

					poly_func::adjust_signs(t1.p, k == 0);

					let& pg = poly_func::pseudo_gcd(t1.p, t2.p);
					if (pg.size() < 2) {
						continue;//suitable
					}

					if (pg.size() == p.p.size()) {
						//pg==cb, poly is divisible by cb => not needed
						ignore = true;
						break;
					}

					//delete cb anyway - it's been shared with someone
					//insert its factorization instead

					stack.emplace_back(RationalPoly{ p.p / pg, 1 });
					stack.emplace_back(RationalPoly{ pg,1 });

					if (pg.size() != poly.p.size()) {
						stack.emplace_back(RationalPoly{ poly.p / pg,1 });
						ignore = true;
					}


					{
						std::scoped_lock lock(m_lock);
						m_found_poly.erase(m_found_poly.begin() + j);
					}
					
					break;
				}
				if (ignore) {
					break;
				}
			}
			if (ignore)continue;

			fp = std::make_unique<found_poly>();

		
			for (let q : intervals) {
				fp->si.emplace_back();
				auto& si = fp->si.back();
				si.cell = (uint16_t)q;
	
				let& k = rf.m_relem[si.cell].ar;
				si.ifs_dim = log(gsp) / log(k);
			}

			if (fp->si.empty()) {
				continue;//couldn't dial subspace
			}

			//stretching coefficient
			fp->r = rf.m_relem;

			std::vector<double> cells = { double(fp->si[0].cell) };

			fp->has_r = 
				block_form::get_simple_reflection(mat_r, poly.p.degree()) &&
				block_form::check_additional_group(mat_r, poly.p.data(), cells);

		}

		fp->cyc = cyc;
		
		

		//fill in the name
		std::ostringstream oss;
		if (poly.d != 1) oss << "(";
		poly_func::print(poly.p.data().data(), poly.p.size(), false, oss);
		if (poly.d != 1) oss << ")/"<<poly.d;
		fp->title = oss.str();
		
		//couldn't be inserted above due to a hash overflow,
		//so we'll force it
		auto res = h.emplace(poly, nullptr);
		res.first->second = std::move(fp);

		if (m == mode::base) {
			need_group.emplace_back(&*res.first);
		}
		
		{
			std::scoped_lock lock(m_lock);
			m_found_poly.emplace_back(&*res.first);
		}
	}

}
