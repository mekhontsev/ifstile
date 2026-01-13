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

#include "dist_solver.h"
#include "diam_solver.h"
#include "oper_block.h"
#include "graph_poly.h"
#include "block_info.h"
#include "aifs_printer.h"
#include "affine_subspace.h"
#include "data_column.h"
#include "block_converter.h"
#include "block_class.h"
#include "affine_point_calc.h"
#include "eval_helpers.h"
#include "ims_val.h"
#include "edge_map.h"
#include "block_graph.h"
#include "variable.h"
#include "matrix_helper.h"

void print_ims_val(const ims_val* v, const ifs_list* lst);

void print_operator(
	const ifs_list& lst,
	std::ostream& str,
	const operator_ptr& ptr,
	const ETYPE par_type,
	const char* fmt);


static void print_header(std::string_view h) 
{
	std::cout << h << std::endl << std::endl;
}

static void print_footer()
{
	std::cout << std::endl;
}

void print_balls(const oper_block& sr, const block_info* bi)
{
	print_header("Bounding balls:");IMS_SCOPE(print_footer);

	if (!bi->exists()) {
		return;
	}

	let* g = sr.get_class();

	for (size_t i = 0; i < g->m_refs.size(); ++i) {
		let vr = sr.get_graph()->ref2fg(i);
		if (vr == ims_max)continue;

		std::cout << g->get_var_name(i) << ": " << std::endl;

		let& b = bi->m_vb[vr];
		if (b.defined2()) {
			let* c = b.center_data();
			for (size_t j = 0; j < (size_t)b.dim(); ++j) {
				std::cout << c[j] << ", ";
			};
			std::cout << " r = " << b.radius() << std::endl;
		}

	}

}

void print_diams(const oper_block& sr, const block_info* bi)
{
	print_header("Geometry balls:"); IMS_SCOPE(print_footer);

	assert(!ims_worker::is_main_thread());

	if (!bi->exists()) {
		return;
	}

	using Real = double;
	
	let& fg = bi->get_fg();
	
	let* g = sr.get_class();

	let sz = sr.num_vars();

	dist_solver dist_s;
	diam_solver diam_s;

	let& im = bi->m_im;


	auto nice_print = [](Real x)->Real
	{
		if (std::abs(x) < 10 * ims_num_traits<Real>::epsilon())return 0;
		return x;
	};

	let w = 1.0 / sz;

	geom_input_data d;
	d.gm = &fg;
	d.ri = bi->m_em;
	d.vb = bi->m_vb;
	d.eps = ims_num_traits<Real>::epsilon();
	d.max_queue_size = 100'000'000;
	d.max_result_size = 1000;

	for (size_t i = 0; i < sz; ++i) {

		let vr = sr.get_graph()->ref2fg(i);
		if (vr == ims_max)continue;

		let& id = g->get_var_name(i);

		d.root = vr;

		////////////////////////////////////////////////////////////////////////
		//diameters
		diam_s.compute(d);

		//std::cout << "ds.m_heap.size()=" << ds.m_heap.size() << std::endl;

		if (ims_need_stop())return;
		ims_worker::get()->work_add(w);

		let& diams = diam_s.m_result;

		if (!diams.empty()) {
			let& fd = diams.front();

			std::cout << id << ": diam^2 = " <<
				(fd[0] - fd[1]).squaredNorm() << std::endl << std::endl;

			for (let& q : diams) {
				for (size_t k = 0; k < 2; ++k) {
					let& v = q[k];
					for (size_t j = 0; j < (size_t)v.size(); ++j) {
						std::cout << nice_print(v[j]) << ", ";
					}
					std::cout << std::endl;
				}
				std::cout << std::endl;
			}
		}

		////////////////////////////////////////////////////////////////////////
		//radii
		let& C = im.me[vr].C;
		dist_s.compute(d, C);

		//std::cout << "dhb.m_heap.size()=" << dhb.m_heap.size() << std::endl;

		if (ims_need_stop())return;
		ims_worker::get()->work_add(w);

		let& rads = dist_s.m_result;

		if (!rads.empty()) {
			std::cout << id << ": center of mass = ";
			for (size_t j = 0; j < (size_t)C.size(); ++j) {
				std::cout << C[j] << ", ";
			};
			std::cout << std::endl;

			let r2 = (rads.front()[0] - C).squaredNorm();
			std::cout << id << ": rad^2 = " << r2 << std::endl;
			
			for (let& v : rads) {
				let& p = v[0];
				for (size_t j = 0; j < (size_t)p.size(); ++j) {
					std::cout << nice_print(p[j]) << ", ";
				}
				std::cout << std::endl;
			}
			std::cout << std::endl;
		}

	}
}

static void print_measure_1(const oper_block& sr, const block_info* bi)
{
	print_header("Measure:"); IMS_SCOPE(print_footer);

	if (!bi->exists()) {
		return;
	}

	using Real = double;

	let& fg = bi->get_fg();

	let& im = bi->m_im;
	
	let* g = sr.get_class();
	
	for (size_t i = 0; i < g->m_refs.size(); ++i) {
		let vr = sr.get_graph()->ref2fg(i);
		if (vr == ims_max)continue;

		let& di = im.di[fg.m_ver2com[vr]];
		let& mes = im.measure[vr];

		std::cout <<
			g->get_var_name(i) << ": dim = " << di.H << " mes = ";

		if (di.DR == dim_relations::equ) {
			std::cout << "inf";
		} else {
			std::cout << mes;
		}

		let& m = im.me[vr].I;
		let d = m(m.size() - 1);
		let eps = ims_num_traits<Real>::almost_zero();
		if (d > eps) {
			std::cout << " inv = ";
			for (size_t k = 0; k + 1 < (size_t)m.size(); ++k) {
				auto v = m[k];
				if (v < eps)v = 0;
				std::cout << v / d << ",";
			}
		}

		std::cout << std::endl;
	}
}


static void print_moments(const oper_block& sr, const block_info* bi)
{
	print_header("Moments:"); IMS_SCOPE(print_footer);

	if (!bi->exists()) {
		return;
	}

	let& im = bi->m_im;

	let* g = sr.get_class();

	for (size_t i = 0; i < g->m_refs.size(); ++i) {
		let vr = sr.get_graph()->ref2fg(i);
		if (vr == ims_max)continue;

		let& mom = im.me[vr];

		std::cout << g->get_var_name(i) << ":" << std::endl;
		std::cout << "C = " << std::endl << mom.C << std::endl;
		std::cout << "I = " << std::endl << mom.I << std::endl;
		std::cout << "Q = " << std::endl << mom.Q << std::endl;
		std::cout << std::endl;
	}
}
void print_measure(const oper_block& sr, const block_info* bi)
{
	print_measure_1(sr, bi);
	print_moments(sr, bi);
}

void print_ifs_def(const oper_block& sr)
{
	std::vector < const oper_block*> arr = { &sr };

	aifs_printer de;
	de.ims_to_text(
		std::cout,
		sr.get_list(),
		arr,
		false,
		false,
		false
	);
}


void print_ifs_eval(const oper_block& sr, eval_context& ec)
{
	print_header("Evaluated maps:"); IMS_SCOPE(print_footer);

	let* g = sr.get_class();
	let nv = sr.num_vars();

	for (size_t i = 0; i < nv; ++i) {
		if (!ec.m_refs5[i].is_geom())continue;
		auto* v = ec.eval_ref(i);
		if (!v)continue;
		
		std::cout << g->get_var_name(i) << "=";
		print_ims_val(v, &sr.get_list());

		std::cout << std::endl;
	};
}


void print_ifs_proj(const oper_block& sr, const block_info& bi)
{
	print_header("Projected IFS:"); IMS_SCOPE(print_footer);
	
	using Real = double;

	
	let* g = sr.get_class();
	let* lst = &sr.get_list();
	
	let nv = sr.num_vars();

	for (let& p : bi.get_proj_data().m_projs) {
		if (!p.ready || p.m_projector.empty())continue;
		std::cout << "proj: ";
		print_operator(*lst, std::cout, p.m_ptr, ETYPE::min_priority, nullptr);
		std::cout << std::endl<<
			"#L =" << std::endl <<
			p.m_projector.L << std::endl <<
			"#R =" << std::endl <<
			p.m_projector.R << std::endl <<
			std::endl << std::endl;
	}

	if (!bi.exists())return;

	bi.get_fg().dump_graph(std::cout);	std::cout << std::endl;
	bi.get_fg().dump_comp(std::cout);	std::cout << std::endl;

	
	for (size_t i = 0; i < nv; ++i) {
		let ver = sr.get_graph()->ref2fg(i);
		if (ver == ims_max)continue;
		std::cout << g->get_var_name(i) << "=@v" << ver << std::endl;
	}
	std::cout << std::endl;

	for (size_t i = 0; i < bi.m_em.size(); ++i) {
		let& m = bi.m_em[i];
		if (!m.used || !m.mg)continue;
		std::cout << "@m" << i << "= ";
		print_ims_val(m.mg.get(), &sr.get_list());
		std::cout << std::endl;
	}


	for (size_t i = 0; i < nv; ++i) {
		if (!sr.ctx()->is_geom(i))continue;
#if 0
		pool_ptr v(bi.create_proj_map(i));


		if (!v) {
			continue;
		}

		std::cout << g->get_var_name(i) << "=";

		edge_map em;
		block_info::set_map(em, v.get(), 0);

		if (!em.mg) {
			std::cout << "invalid projection" << std::endl;
			continue;
		}

		if (!em.mg->is(ims_val_b::ETP::matrix, ims_val_b::EST::real)) {
			print_ims_val(em.mg.get(), lst);
		} else {
			let& a = em.mg->MatR();
			let& b = em.mg->TrR();

			constexpr auto eps = std::numeric_limits<Real>::epsilon() * 10;

			let dproj = em.mg->extent(0);

			bool t_zero = true;
			for (size_t r = 0; r < dproj; ++r) {
				if (std::abs(b(r)) > eps) {
					t_zero = false;
					break;
				}
			}
			if (!t_zero) {
				std::cout << "[";
				for (size_t r = 0; r < dproj; ++r) {
					std::cout << b(r);
					if (r + 1 < dproj)std::cout << ",";
				}
				std::cout << "]*";
			}


			std::cout << "[" << std::endl;
			for (size_t r = 0; r < dproj; ++r) {
				std::cout << "[";
				for (size_t c = 0; c < dproj; ++c) {
					let x = a(r, c);
					std::cout << (std::abs(x) > eps ? x : 0);
					if (c + 1 < dproj)std::cout << ",";
				}
				std::cout << "]";
				if (r + 1 < dproj) {
					std::cout << ",";
				} else {
					std::cout << "]";
				}
				std::cout << std::endl;
			}

			let det = matrix_helper::calc_det_ex(em.mg->p_r(), dproj);

			std::cout << " det = " << det << std::endl;
		}

		
#endif	


		std::cout << std::endl;
	};


}


void print_normal_maps(const oper_block& sr, eval_context& ec)
{
	print_header("Normal maps:"); IMS_SCOPE(print_footer);

	let* g = sr.get_class();
	let nv = sr.num_vars();
	let& ra = ec.m_refs5;
	
	val_recognizer rec;
	
	for (size_t i = 0; i < nv; ++i) {
	
		if (!ra[i].is_geom())continue;
		auto* v = ec.eval_ref(i);
		if (!v)continue;

		let dim = ra[i].c.a->get_dim();
		
		pool_ptr mi(dim>0 ? eval_helpers::to_affine3(v, dim): v);
		if (!mi || !mi->is(ims_val::EST::rational)) {
			continue;
		}
		rec.init(&sr);
		if (rec.recognize(mi.get())) {
			std::cout << g->get_var_name(i) << "=";
			print_operator(sr.get_list(), std::cout,
				rec.get_ptr(), ETYPE::min_priority, nullptr);
			std::cout << std::endl;
		}
	};
}

void print_ast(const oper_block& sr, const block_info& bi, const ast_maps& am)
{
	print_header("AST:"); IMS_SCOPE(print_footer);

	let* g = sr.get_class();

	for (size_t i = 0; i < bi.m_em.size(); ++i) {

		let& m = bi.m_em[i];
		if (!m.used || !m.mg)continue;
		std::cout << "@m" << i << "= ";

		let& im = am.get_map(i);

		if (im.num == 0) {
			std::cout << "1" << std::endl;
			continue;
		}
		
		for (size_t j = 0; j < im.num; ++j) {
			let aidx = am.m_ixm.get_atom(im.start + j);

			let& a = am.m_atoms[aidx];
			if (a.h.tt == ETYPE::reference && aidx < sr.num_vars()) {
				std::cout << g->get_var_name(aidx);
			} else {
				print_operator(
					sr.get_list(), std::cout, a, ETYPE::vector, nullptr);
			}

			if (j + 1 < im.num)std::cout << "*";
		}

		std::cout << std::endl;
	}
	std::cout << std::endl;

	////////////////////////////////////////////////////////////////////////////

	let* ec = sr.ctx();
	if (!ec)return;
	

	for (size_t i = 0; i < ec->m_refs5.size(); ++i) {//run through more than the graph
		let& rf = ec->get_ref4(i);

		for (size_t j = 0; j < 2; ++j) {
			if (!rf.ready[j]) continue;

			std::cout << (rf.dep_from_unions	? "u" : "_");
			std::cout << (rf.dep_from_cycles	? "c" : "_");
			std::cout << " ";

			if (j == 0)std::cout << "*";

			if (i < g->m_refs.size()) {
				std::cout << g->get_var_name(i) << "=";
			} else {
				std::cout << "ref(" << i << ")=";
			}

			let& v = rf.v[j];
			print_ims_val(v.get(), &sr.get_list());
			std::cout << std::endl;
		}
	};

	


}

void print_subspaces(const oper_block& sr, const block_info* bi)
{
	print_header("Subspaces:"); IMS_SCOPE(print_footer);
	
	assert(!ims_worker::is_main_thread());

	if (!bi->exists()) {
		return;
	}

	let dim = bi->common_dim_proj();
	if (dim == 0) {
		return;
	}

	using Real = double;

	let& fg = bi->get_fg();
	
	affine_builder ab;

	affine_point_calc pcalc;
	std::vector<edge_ball> vb;

	//find one point from each set
	pcalc.process(vb, bi->m_em, fg);

	

	ab.compute(
		fg,
		dim,
		bi->m_em, 
		vb, 
		ims_num_traits<Real>::almost_zero(), 
		1000);

	
	let* g = sr.get_class();
	let& rf = g->m_refs;
	

	for (size_t i = 0; i < rf.size(); ++i) {

		let vr = sr.get_graph()->ref2fg(i);
		if (vr == ims_max)continue;

		let& vd = ab.m_data[vr];
		size_t num_pt = 0;
		size_t num_sb = 0;
		for (let& q : vd.m_sbs) {
			num_pt = std::max(num_pt, q.num);
			if (q.num > 0)++num_sb;
		}

		std::cout << g->get_var_name(i) << ": " << num_pt << " " << num_sb << std::endl;

	}
	std::cout << std::endl;

	////////////////////////////////////////////////////////////////////////////



	for (size_t i = 0; i < rf.size(); ++i) {

		let vr = sr.get_graph()->ref2fg(i);
		if (vr == ims_max)continue;

		let& vd = ab.m_data[vr];

		bool id_printed = false;

		for (let& q : vd.m_sbs) {
			if (q.num == 0)continue;
			if (q.num == dim + 1)continue;//all space

			if (!id_printed) {
				id_printed = true;
				std::cout << g->get_var_name(i) << std::endl;
			}

			Eigen::Map<DynMat<double>,0, Eigen::OuterStride<>> 
				m(&ab.m_points[q.idx], dim, q.num, Eigen::OuterStride<>(dim+1));

			std::cout << m.transpose() << std::endl << std::endl;
		}
	}

}

void print_ifs_data(const oper_block& sr)
{
	print_header("Data:"); IMS_SCOPE(print_footer);

	std::string buf;
	for (let& c : data_column::g_cols) {
		std::cout << c.title << "=";
		c.get_column_str(sr, buf, true);
		std::cout << buf << std::endl;
	}
}
