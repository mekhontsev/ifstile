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
#include "ifs_renderer.h"
#include "ifs_data_text.h"
#include "block_class.h"
#include "info_printer.h"
#include "report_params.h"
#include "custom_block.h"
#include "ast_stack.h"
#include "inter_type.h"
#include "derived_ifs.h"

bool ifs_renderer::init(const std::string& aifs)
{
	m_nfo = std::make_unique<ims_info>();
	m_bb = nullptr;

	std::istringstream istr(aifs);
	auto nbeg = std::istreambuf_iterator<char>(istr);
	let nend = std::istreambuf_iterator<char>();

	
	/////////////////////////////////////////////////
	
	size_t cur_line = 1;

	read_state rs;

	std::string js_src;
	js_from_stream(js_src, cur_line, nbeg, nend);

	m_nfo->m_js_src = std::move(js_src);
	

	if (!m_nfo->m_js_src.empty()) {
		m_nfo->m_js_filename = "";
		if (!m_nfo->process_js(rs)) {
			return false;
		}
	}

	for (;;) {//loop through blocks
		if (!aifs_from_stream_ex(m_nfo->m_list, cur_line, rs, nbeg, nend)) {
			return false;
		}

		if (rs.m_source_num_lines == 0) {
			break;//completed
		};
	};

	if (m_nfo->m_list.empty() && m_nfo->m_js_src.empty()) {
		std::cerr << "No blocks or JS code found in the input." << std::endl;
		return false;
	}

	if (!m_nfo->link_refs(0)) {
		return false;
	}

	return true;
}

bool ifs_renderer::select(std::string_view block_id, std::string_view root_id)
{
	if (!m_nfo) {
		std::cerr << "No AIFS data loaded. Please call init() with valid AIFS data before selecting a block." << std::endl;
		return false;
	}

	oper_block* b = nullptr;
	
	if (!block_id.empty()) {
		b = m_nfo->m_list.find_block2(block_id, block_id);
		if (!b) {
			block_id_t bid = 0;
			if (boost::conversion::try_lexical_convert(block_id, bid)) {
				if (bid < m_nfo->m_list.m_id2data.size()) {
					b = m_nfo->m_list.get_block(bid);
				}
			}
		}
	} else {//find first visible
		for (let bid : m_nfo->m_list.m_blocks) {
			auto* pb = m_nfo->m_list.m_id2data[bid].b.get();
			if (!pb->m_flags.hidden) {
				b = pb;
				break;
			}
		}
	}

	if (!b) {
		std::cerr << "No blocks found." << std::endl;
		return false;
	}

	if (m_bb != b) {
		m_bi.set_to_recalc_graph();
		if (!m_bi.init4(*b)) {
			std::cerr << "Failed to initialize block info for block: " << block_id << std::endl;
			return false;
		}
		m_sv.eval_builtins(*b, m_bi.m_ctx);

		//even if the moment or dimension could not be calculated correctly
		//important values are filled with acceptable values
		if (!m_bi.compute_metrics()) {
			std::cerr << "Failed to compute block metrics for block: " << block_id << std::endl;
			return false;
		}
	}

	m_bb = b;

	///////////////////////////////////////////////////////////////////////////

	size_t root_ref = ims_max;
	if (!root_id.empty()) {
		root_ref = b->get_class()->find_var_by_name(root_id);
	} else {
		root_ref = m_sv.eval_root(*b);
		if (root_ref == ims_max) {
			root_ref = b->find_default_ref();
		}
	}

	if (root_ref == ims_max) {
		std::cerr << "No valid root reference found in the block." << std::endl;
		return false;
	}
	b->set_active_ref(root_ref);

	return true;
}


bool ifs_renderer::information(const char* what)
{
	if (!m_bb) {
		std::cerr << "No block selected. Please call select() with a valid block before requesting information." << std::endl;
		return false;
	}

	std::string_view w(what);

	if (w == "Evaluation") {
		print_ifs_eval(*m_bb, m_bi.m_ctx);
		return true;
	}
	if (w == "NormalMaps") {
		if (m_bb->get_dim() > 0) {
			print_normal_maps(*m_bb, m_bi.m_ctx);
		}
		return true;
	}
	if (w == "Projection") {
		print_ifs_proj(*m_bb, m_bi);
		return true;
	}
	if (w == "Dimension") {
		print_dimensions(std::cout, m_bi);
		return true;
	}
	if (w == "Balls") {
		print_balls(*m_bb, &m_bi);
		return true;
	}
	if (w == "Diameters") {
		print_diams(*m_bb, &m_bi);
		return true;
	}
	if (w == "Measure") {
		print_measure(*m_bb, &m_bi);
		return true;
	}
	if (w == "Subspaces") {
		print_subspaces(*m_bb, &m_bi);
		return true;
	}
	if (w == "AST") {
		print_ast(*m_bb, m_bi);
		return true;
	}
	if (w == "Components") {
		print_components(*m_bb, m_bi);
		return true;
	}
	return false;
}


bool ifs_renderer::custom_ifs(const report_params& rp, bool boundary_mode)
{
	if (0 == m_nb.num_ver()) {
		std::cerr << "#Neighbor graph is empty. Please calculate the neighbor graph before creating a custom block." << std::endl;
		return false;
	}

	let num_neighbours = m_nb.set_idx_graph(!boundary_mode);
	if (!num_neighbours) {
		std::cerr << "#Neighbor graph is empty." << std::endl;
		return false;
	}

	std::unique_ptr<oper_block> block_custom;

	let res = create_neghbours(
		m_nfo->m_list.m_idf,
		*block_custom,
		*m_bb,
		m_nb,
		m_bi.get_fg(),
		boundary_mode ? nullptr : &rp,
		nullptr);

	ASSUME(res);
	
	ast_stack ai;
	if (!ims_info::link_refs_for_block(*m_nfo, block_custom.get(), ai)) {
		std::cerr << "#Failed to link references for the custom block." << std::endl;
		return false;
	}

	print_ifs_def(*block_custom);
	return true;

#if 0
	
	inter_result ires;

	let ret = create_custom_block(
		block_custom,
		ires,
		m_bi,
		*m_bb,
		m_nfo->m_list.m_idf,
		boundary_mode ? ifs_object_type::boundary : ifs_object_type::custom,
		rp);

	std::cout << "#how many intersections were checked = " << ires.m_gcx << std::endl;
	std::cout << "#depth reached = " << ires.m_depth << std::endl;
	std::cout << "#how many bits were used = " << ires.m_bits << std::endl;
	std::cout << "#minimum depth where an exact overlap was found (0: OSC) = " << ires.m_over_depth << std::endl;
	std::cout << "#intersections are fully created = " << ires.m_completed << std::endl;
	std::cout << "#there was a rational overflow = " << ires.m_overflowed << std::endl;
	std::cout << "#the mode in which the calculations were performed = " << 
		(ires.m_mode == intersect_mode::rational ? "rational" : 
			ires.m_mode == intersect_mode::big_rational ? 
			"big rational" : "floating point") << std::endl;

	if (!ret)
	{
		return false;
	};

	ast_stack ai;
	ims_info::link_refs_for_block(*m_nfo, block_custom.get(), ai);

	print_ifs_def(*block_custom);
	return true;
#endif
}



bool ifs_renderer::calc_neighbor_graph(inter_result& ires, const integer_ims::settings& settings)
{
	if (!m_bb) {
		std::cerr << "No block selected for neighbor graph calculation. Please call select() with a valid block before calculating intersections." << std::endl;
		return false;
	}
	ires = m_cs.calc_inter(m_nb, m_bi, settings);
	return true;
}

void ifs_renderer::fit1d2d(
	standard_vars& sv,
	block_info& bi,
	camera_ex& cc,
	size_t root,
	size_t tw,
	size_t th,
	float iter_thk,
	bool is2d)
{
	using real_number = standard_vars::Real;
	auto& si = sv.m_si2;

	box<real_number> dst;
	auto& sd = cc.m_sd;

	if (cc.empty(2)) {
		sd.a = 0;
	}


	Eigen::Matrix2<real_number> rot;


	auto si_a = si;//copy
	if (is2d) {
		let a = sd.a * boost::math::constants::pi<double>() / 180;
		let c = cos(a);
		let s = sin(a);
		rot << c, s, -s, c;
		si_a.basis = si_a.basis * rot;
	}

	builder::adjust_box(
		dst,
		0.01,
		si_a,
		bi.m_em,
		bi.m_vb,
		bi.get_fg(),
		root);

	if (ims_need_stop()) {
		return;
	}

	assert(!dst.empty());

	dst.adjust();


	auto vc = dst.get_center();

	if (is2d) {
		vc = rot * vc;
	}

	sd.c[0] = vc(0);
	real_number bw = dst.size(0);
	real_number bh;
	if (vc.size() > 1) {
		sd.c[1] = vc(1);
		bh = dst.size(1);
	} else {
		sd.c[1] = 0;
		bh = 0;
	}

	let twh = std::min(tw, th);
	let ps = std::max(bw / tw, bh / th);
	sd.r = ps * twh / 2;
	//expand by about 3 pixels
	sd.r *= 1 + (1 + 2 * iter_thk) / twh;

	cc.m_2d_empty = false;
}

bool ifs_renderer::set_camera(const double* camera_params, size_t num_params)
{
	if (!m_bb) {
		std::cerr << "No block selected. Please call select() with a valid block before setting the camera." << std::endl;
		return false;
	}

	if (num_params == 4) {
		m_sv.m_xcam2.m_sd.c[0] = camera_params[0];
		m_sv.m_xcam2.m_sd.c[1] = camera_params[1];
		m_sv.m_xcam2.m_sd.r = camera_params[2];
		m_sv.m_xcam2.m_sd.a = camera_params[3];
		m_sv.m_xcam2.m_2d_empty = false;
		m_fit = false;
		return true;
	}

	if (num_params == 10) {
		m_sv.m_xcam2.m_camera.m_loc << camera_params[0], camera_params[1], camera_params[2];
		m_sv.m_xcam2.m_camera.m_ref << camera_params[3], camera_params[4], camera_params[5];
		m_sv.m_xcam2.m_camera.m_ver << camera_params[6], camera_params[7], camera_params[8];
		m_sv.m_xcam2.m_camera.m_fov = camera_params[9];
		m_sv.m_xcam2.m_3d_empty = false;
		m_fit = false;
		return true;
	}

	if (num_params == 0) {
		m_fit = true;
		return true;
	}

	return false;
}

bool ifs_renderer::render(ims_bitmap& dst, float quality, float thickness)
{
	if (!m_bb) {
		std::cerr << "No block selected for rendering." << std::endl;
		return false;
	}

	clear_color(dst);

	let root2 = m_bb->get_froot();

	let dim_set = m_bi.m_ver_dim[root2];
	if (dim_set == ims_max || dim_set == 0) {
		return false;
	};
	
	//initialize the subspace
	auto& sv = m_sv;
	auto& si = sv.m_si2;

	let tx = dst.w();
	let ty = dst.h();

	bool has2dstd = false;
	bool has2dext = false;
	bool has3d = false;
	size_t max_dim_set_ext = 0;


	///////////////////////////////////////////////

	si.resize2(dim_set,  dim_set);
	si.reset();

	if (dim_set > si.get_section_dim()) {
		//we select the most elongated directions, example:
		//k1 <= k2 <= k3 are singular values, so if k1 != k2, we take (k2, k3)
		//otherwise, we take (k1, k2)
		let& me = m_bi.m_im.me[root2];
		si.origin = me.C;

		auto& b = si.basis_user;

		size_t start_idx = me.Q.cols() - b.cols();

		while (start_idx > 0 &&
			std::abs(me.I(start_idx) - me.I(start_idx - 1)) <
			ims_num_traits<double>::almost_zero())
		{
			--start_idx;
		}

		for (int c = 0; c < b.cols(); ++c) {
			b.col(b.cols() - 1 - c) = me.Q.col(start_idx + c);
		}
	}

	si.init_si();

	let sds = si.get_section_dim();

	if (sds < 1 || sds > 3) {
		return false;
	}

	if (m_bi.get_fg().is_ver_empty(root2)) {
		return false;
	}

	m_rp.reset_render_params();
	m_rp.m_palette.reset();

	let& rend = m_rp;

	
	//builder type
	let& crz = sv.chas_builtin(builtin_ids::colorize) ?
		sv.m_colorize : m_rp.m_colorize;

	// automatic position detection
	auto& cc = m_sv.m_xcam2;
	if (sds == 2 || sds == 1) {

		if (m_fit) {
			ifs_renderer::fit1d2d(
				m_sv,
				m_bi,
				cc,
				root2,
				tx,
				ty,
				thickness,
				sds == 2);
		}
		
		if (cc.empty(2)) {
			return false;
		}
		////////////////////////////////////////////////////////////////


		if (crz.is_field()) {
			has2dext = true;
			max_dim_set_ext = std::max(max_dim_set_ext, dim_set);
		} else {
			has2dstd = true;
		}
	} else {//3D

		if (cc.empty(3)) {
			//use the projection of the center of mass
			ball3d<real_number> bound;

			projector proj;
			proj.R = si.basis;
			proj.calc_L_ortho();
			bound.c = proj.L * (m_bi.m_im.me[root2].C - si.origin);
			bound.r = 1;//then adjust3d will straighten it out

			cc.m_camera.randomize(bound);
			cc.m_camera.init();
			cc.m_3d_empty = false;
		}

		builder::adjust3d(
			cc.m_camera,
			tx,
			ty,
			thickness,
			si,
			m_bi.m_em,
			m_bi.m_vb,
			m_bi.get_fg(),
			root2);

		
		if (cc.empty(3)) {
			return false;
		}

		has3d = true;
	}

	double cdp = 0;//coloring depth for the edge method

	if (crz.is_tiling()) {
		cdp = std::pow(2.0, -crz.get_depth());
		cdp += ims_num_traits<real_number>::almost_zero();
	}


	//maximum number of pixels on the last iteration
	let max_pix = (tx) * (ty);


	if (has2dstd)m_buf2d_di.m_img.reserve(max_pix);
	if (has3d)m_buf3d_di.m_img.reserve(max_pix);
	if (has2dext)m_buf_ext_di.m_img.reserve(max_pix);


	if (has2dstd) {
		m_builder2d.reserve_memory(max_pix);
	}
	if (has3d) {
		m_builder3d.reserve_memory(max_pix);
	}
	if (has2dext) {
		m_builder_ext.reserve_memory(max_pix, max_dim_set_ext);
	}


	////////////////////////////////////////////////////////////////

	//size of one thumbnail
	let wc = tx;
	let hc = ty;

	//size of the entire image
	auto& rgba = dst;

	clear_color(rgba);


	const size_t oc = 1;
	const float  qa = quality;

	//resolution of the enlarged thumbnail taking into account oversampling
	let sx = wc * oc;
	let sy = hc * oc;

	let root = root2;
	let& pal = sv.chas_builtin(builtin_ids::palette) ?
		sv.m_pal : rend.m_palette;

	let& fog = sv.chas_builtin(builtin_ids::background) ?
		sv.m_bac.get_fog() : rend.m_background.get_fog();

	////////////////////////////////////////////////////////////////


	//resulting thumbnail position
	let hx = 0;
	let hy = 0;

	////////////////////////////////////////////////////////////////
	//building

	if (has2dstd) {

		screen_params<real_number> sp;
		cc.m_sd.to_params(sp, sx, sy);

		m_builder2d.m_img2.recreate(sx, sy);
		m_builder2d.m_img2.for_each([](auto& q) {q.clear_color(); });

		m_cm.init_cmaps(
			m_bi.m_style,
			m_bi.get_fg(),
			pal,
			crz.shift,
			crz.type
		);

		m_ss.gm = &m_bi.get_fg();
		m_ss.m_psi = &si;
		m_ss.ri = m_bi.m_em;
		m_ss.vb = m_bi.m_vb;
		m_ss.mes_mul = m_bi.m_im.mes_mul;
		m_ss.icm = &m_cm;
		m_ss.cdpx = cdp;

		m_builder2d.calc_buffer(
			m_ss,
			qa,
			thickness,
			root,
			sp);

		m_builder2d.init_draw(m_buf2d_di);

		m_buf2d_di.init(
			rend.m_brightness,
			rend.m_contrast,
			rend.m_border_pow,
			rend.m_inv_mode);

		to_bitmap(rgba, m_buf2d_di, hx, hy, wc, hc, 1);

	} else if (has2dext) {

		screen_params<real_number> sp;
		cc.m_sd.to_params(sp, sx, sy);

		m_builder_ext.prepare(sx, sy, si, sp);

		let power = crz.params.front();

		m_ss.gm = &m_bi.get_fg();
		m_ss.m_psi = &si;
		m_ss.ri = m_bi.m_em;
		m_ss.vb = m_bi.m_vb;
		m_ss.mes_mul = m_bi.m_im.mes_mul;
		m_ss.icm = nullptr;
		m_ss.cdpx = cdp;//not used

		m_builder_ext.calc_buffer(
			m_bi.m_ver_dim[root],
			m_ss,
			root,
			sp.ps / 2,
			quality * 4,
			power);


		projector proj;
		proj.R = si.basis;
		proj.calc_L_ortho();

		m_builder_ext.init_draw(m_buf_ext_di, proj.L);

		m_buf_ext_di.init(pal, crz);

		to_bitmap(rgba, m_buf_ext_di, hx, hy, wc, hc, 1);

	} else if (has3d) {//3D

		m_cm.init_cmaps(
			m_bi.m_style,
			m_bi.get_fg(),
			pal,
			crz.shift,
			crz.type
		);

		m_ss.gm = &m_bi.get_fg();
		m_ss.m_psi = &si;
		m_ss.ri = m_bi.m_em;
		m_ss.vb = m_bi.m_vb;
		m_ss.mes_mul = m_bi.m_im.mes_mul;
		m_ss.icm = &m_cm;
		m_ss.cdpx = cdp;


		m_builder3d.calc_buffer(
			m_ss,
			sx,
			sy,
			cc.m_camera,
			qa,
			thickness,
			root,
			sv.m_light
		);

		////////////////////////////////////////////////////////

		//cannot be interrupted
		m_builder3d.init_draw(
			m_buf3d_di,
			si,
			cc.m_camera);

		m_buf3d_di.calc_ssao(
			rend.m_ssao_rad_perc,
			rend.m_ssao_samples);

		m_buf3d_di.init(
			rend.m_brightness,
			rend.m_border_pow,
			rend.m_ssao_density,
			fog);

		to_bitmap(rgba, m_buf3d_di, hx, hy, wc, hc, 1);

	}//3D

	return true;
}

