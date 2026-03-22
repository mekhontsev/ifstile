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
#include "program_state.h"
#include "palette.h"
#include "visible_blocks.h"
#include "render_params.h"
#include "oper_block.h"
#include "aifs_printer.h"
#include "ims_keywords.h"
#include "png_support.h"
#include "block_graph.h"
#include "ims_info.h"
#include "edge_map.h"
#include "variable.h"

thumb_elem* program_state::get_first_thumb_elem()
{
	if (m_thumb_arr.empty()) {
		return nullptr;
	}
	auto* ret = m_thumb_arr.front().get();
	if (ret->empty()) {
		return nullptr;
	}
	return ret;
}

build_data* program_state::get_first_build_data2()
{
	auto* bp = get_first_thumb_elem();
	if (!bp)return nullptr;
	return bp->m_data3;
}


ims_bitmap& program_state::get_img_draw()
{
	assert(m_draw_img_idx <= 1);
	return m_img_buf[m_draw_img_idx];
}

ims_bitmap& program_state::get_img_rend()
{
	assert(m_draw_img_idx <= 1);
	return m_img_buf[m_draw_img_idx ^ 1];
}

void program_state::clear_draw_image()
{
	assert(ims_worker::is_main_thread());
	std::scoped_lock lock(m_lock_draw);
	get_img_draw().recreate(0, 0);
	m_img_draw_uploaded = false;
}


background* program_state::get_first_background()
{
	auto* bd = get_first_build_data2();
	if (!bd)return nullptr;
	return bd->get_background();
}


bool program_state::can_upload_img(size_t ms) const
{
	if (m_img_draw_uploaded)return false;

	return m_img_draw_start_time.to_now_ms() > (int64_t)ms;
}

void program_state::init7()
{
	m_build_data.resize(1);
	m_build_data.front().clear();

	thumb_resize(1);
	m_thumb_arr.front()->clear();
	m_thumb_arr.front()->m_data3 = &m_build_data.front();

}

void program_state::swap_buffer()
{
	assert(!ims_worker::is_main_thread());
	std::scoped_lock lock(m_lock_draw);
	m_draw_img_idx ^= 1;
	m_img_draw_uploaded = false;
	m_img_draw_start_time = m_img_rend_start_time;
}

void program_state::set_block3(const oper_block* src, const variator_params& vp)
{
	m_build_data.front().pre_init(src, vp);
}

void program_state::thumb_resize(size_t sz)
{
	let was_thumb = m_thumb_arr.size();
	m_thumb_arr.resize(sz);

	for (size_t i = was_thumb; i < sz; ++i) {
		m_thumb_arr[i] = std::make_unique<thumb_elem>();
		m_thumb_arr[i]->clear();
	}
}


bool program_state::save_png(
	const ims_info& nfo,
	const std::function<void(const void*, size_t)>& of,
	const render_params& rpars,
	bool crop
)
{
	//always save with the coloring and zoom parameters, so we create copies
	std::vector<std::unique_ptr<oper_block>> bsa;
	std::vector<const oper_block*> arr;

	for (auto& dt : m_build_data) {
		if (dt.empty())continue;
		let* b = dt.get_direct(ifs_object_type::normal);
		auto& dst = bsa.emplace_back(std::make_unique<oper_block>());
		b->simple_copy(*dst);
		dt.m_special.add_all_builtins(*dst, rpars, false);
		arr.emplace_back(dst.get());
	}

	////////////////////////////////////////////////////////////////////////////
	std::ostringstream ss;

	ss << ims_png::png_chunk_aifs << '\0';

	ss << "\r\n";

	nfo.print_js(ss);
	aifs_printer de;
	de.ims_to_text(ss, nfo.m_list, arr, false, true, true);

	//to find the end correctly later
	ss << ims_keywords::block << ims_keywords::block <<
		 ims_keywords::end << ims_keywords::nlc;

	////////////////////////////////////////////////////////////////////////
	let* bg = get_first_background();
	uint8_t bg8[3];
	if (bg) {
		for (size_t i = 0; i < 3; ++i) {
			bg8[i] = uint8_t(ims_clamp(bg->data[i] * 255 + 0.5f, 0.0f, 255.0f));
		}
	}

	let aifs = ss.str();

	return ims_png::save(
		of, 
		aifs,
		get_img_draw(), 
		crop, 
		bg ? bg8 : nullptr);
}


bool program_state::on_start_build(
	screen_area& scr, 
	bool thumb_list,
	size_t max_thumb, 
	const visible_blocks& vb,
	const variator_params& vp)
{
	assert(ims_worker::is_main_thread());

	if (max_thumb == 1 || !thumb_list) {
		scr.initX(1, max_thumb);
		m_build_data.resize(1);
		//m_build_data[0].pre_init is not needed because it was executed before on_click
		return true;
	}

	//here max_thumb>1 && thumb_list
	
	if (vb.m_cur_block_pos >= vb.m_vis_blocks.size()) {
		return false;//for example, an empty list
	}

	let rem_vis = vb.m_vis_blocks.size() - vb.m_cur_block_pos;

	scr.initX(rem_vis, max_thumb);
	let sz = std::min(scr.get_num(), rem_vis);
	m_build_data.resize(sz);

	//don't touch the zero one
	for (size_t i = 1; i < sz; ++i) {
		let* bb = vb.get_vis(vb.m_cur_block_pos + i);
		assert(bb);

		//initialize thumbnail
		m_build_data[i].pre_init(bb, vp);
	}
	
	return true;
}


void program_state::fit1d2d(
	thumb_elem& cur,
	size_t tw,
	size_t th,
	float iter_thk,
	bool is2d)
{
	auto& bd = *cur.m_data3;
	auto& cc = *cur.m_pcam;
	auto& sv = bd.m_special;
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
		bd.m_bi.m_em,
		bd.m_bi.m_vb,
		bd.m_bi.get_fg(),
		cur.m_root2);

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

void program_state::build_image(
	ims_identifiers& idf,
	std::function<void()> on_frame_complete, 	
	screen_area& scr,
	draw_task task, 
	report_params rp, 
	bool thumb_list,
	size_t max_thumb,
	bool force2d, 
	const render_params& rend,
	const ifs_object_type mode,
	ims_worker& rth)
{
	assert(on_frame_complete);

	assert(!ims_worker::is_main_thread());

	size_t num_thumb = 0;

	if (max_thumb > 1 && !thumb_list) {//components + thumbnails

		auto& dt = m_build_data[0];

		if (!dt.init_normal_block()) {
			return;
		}

		if (mode != ifs_object_type::normal) {
			if (!dt.init_custom_block(idf, mode, rp)){
				return;
			};
		}

		
		if (!dt.m_bi.exists()) {
			return;
		}

		let* b = dt.get_direct(mode);
		let root_ref = b->get_active_ref();
	
		auto& sbs = m_subsets_arr;
		sbs.clear();

		let& r = b->ctx()->m_refs5;

		let* g = b->get_graph();

		for (size_t i = root_ref; i < r.size(); ++i) {
			let& q = r[i];
			if (!g->closed2(i) || q.is_subs)continue;
			sbs.emplace_back(i);
		}

		scr.initX(sbs.size(), max_thumb);
	
		num_thumb = scr.get_num();
		thumb_resize(num_thumb);

		num_thumb = std::min(num_thumb, m_subsets_arr.size());

		for (size_t i = 0; i < num_thumb; ++i) {
			auto& bp = *m_thumb_arr[i];

			bp.m_data3 = &dt;//refer to one
			bp.m_root2 = g->ref2fg(m_subsets_arr[i]);//redefine vertices

			//the camera is unknown - because there is only one per block
			bp.m_pcam = &bp.m_cam;
			bp.m_pcam->clear();
		}

	} else {//max_thumb==1 || thumb_list
		num_thumb = scr.get_num();
		thumb_resize(num_thumb);

		num_thumb = std::min(num_thumb, m_build_data.size());

		for (size_t i = 0; i < num_thumb; ++i) {
			auto& bp = *m_thumb_arr[i];

			auto& dt = m_build_data[i];

			bp.m_data3 = &dt;//each one refers to his own
			bp.m_root2 = ims_max;//don't use overriding

			//use the set camera for its boundary, etc.
			bp.m_pcam = &dt.m_special.m_xcam2;
		}
	}

	if (num_thumb == 0) {
		return;
	}

	let tx = scr.get_tx();
	let ty = scr.get_ty();

	if (task.fit) {
		task.rebuild();
	}

	//resolution downscaling multiplier
	const size_t lim_ts = rend.m_oversamp >= 0 ? 1 : (1 << (-rend.m_oversamp));
	let iter_ovs = std::max(rend.m_oversamp, 0) + 1;
	let iter_qua = std::max(rend.m_quality, 1.0f);
	let iter_thk = rend.m_thickness;

	////////////////////////////////////////////////////////////////////////
	//finding the boundary, initializing subspaces and cameras

	rth.work_reset();
	rth.m_work_mul = 1.0 / num_thumb;
	m_build_scale = 0;

	bool has2dstd = false;
	bool has2dext = false;
	bool has3d = false;
	size_t max_dim_set_ext = 0;

	for (size_t i = 0; i < num_thumb; ++i) {
		IMS_SCOPE([&] {rth.work_add(1); });

		auto& cur = *m_thumb_arr[i];
		auto& cdt = *cur.m_data3;

		if (rth.is_need_stop2()) {
			return;
		}

		if (!cdt.init_normal_block()) {
			continue;
		}

		if (rth.is_need_stop2()) {
			return;
		}
		if (mode != ifs_object_type::normal) {
			if (!cdt.init_custom_block(idf, mode, rp)){
				continue;
			};
		}

		if (rth.is_need_stop2()) {
			return;
		}

		let& bd = cdt;

		if (cur.m_root2 == ims_max) {
			cur.m_root2 = bd.get_froot();
			if (cur.m_root2 >= bd.m_bi.get_fg().num_ver()) {
				continue;
			}
		}

		let dim_set = bd.m_bi.m_ver_dim[cur.m_root2];
		if (dim_set == ims_max || dim_set == 0)continue;

		////////////////////////////////////////////////////////////////

		//initialize the subspace
		auto& sv = cdt.m_special;
		auto& si = sv.m_si2;

		//try to reuse the subspace
		//even if the set has changed
		if (dim_set != si.get_dim_space()) {
			sv.m_si_empty = true;
		};

		if (sv.m_si_empty) {
			sv.m_si_empty = false;

			si.resize2(dim_set, force2d ? 2 : dim_set);
			si.reset();

			if (dim_set > si.get_section_dim()) {
				//select the most elongated directions, example:
				//k1 <= k2 <= k3 are singular values, so if k1 != k2, we take (k2, k3)
				//otherwise, we take (k1, k2)
				let& me = bd.m_bi.m_im.me[cur.m_root2];
				si.origin = me.C;

				auto& b = si.basis_user;

				size_t start_idx = me.Q.cols() - b.cols();

				let eps = ims_num_traits<real_number>::almost_zero();

				while (start_idx > 0 && 
					std::abs(me.I(start_idx) - me.I(start_idx - 1)) < eps)
				{
					--start_idx;
				}
				

				for (int c = 0; c < b.cols(); ++c) {
					b.col(b.cols()-1-c) = me.Q.col(start_idx+c);
				}
			}

			
			si.init_si();
		}


		if (rth.is_need_stop2()) {
			return;
		}


		let sds = si.get_section_dim();

		if (sds < 1 || sds > 3) {
			continue;
		}

		if (bd.m_bi.get_fg().is_ver_empty(cur.m_root2)) {
			continue;
		}
		

		//builder type
		let& crz = sv.chas_builtin(builtin_ids::colorize) ?
			sv.m_colorize : rend.m_colorize;

		// automatic position detection
		auto& cc = *cur.m_pcam;
		if (sds == 2 || sds == 1) {
			if (task.fit || cc.empty(2)) {

				fit1d2d(cur,
					tx* iter_ovs,
					ty* iter_ovs,
					iter_thk,
					sds == 2);
			}

			if (cc.empty(2))continue;
			////////////////////////////////////////////////////////////////


			if (crz.is_field()) {
				cur.set_btype(builder_type::E2dField);
				has2dext = true;

				max_dim_set_ext = std::max(max_dim_set_ext, dim_set);
			} else {
				cur.set_btype(builder_type::E2d);
				has2dstd = true;
			}
		} else {//3D

			if (task.fit || cc.empty(3)) {

				if (cc.empty(3)) {
					//use the projection of the center of mass
					ball3d<real_number> bound;

					projector proj;
					proj.R = si.basis;
					proj.calc_L_ortho();
					bound.c = proj.L * (bd.m_bi.m_im.me[cur.m_root2].C - si.origin);
					bound.r = 1;//then adjust3d will straighten it out

					cc.m_camera.randomize(bound);
					cc.m_camera.init();
					cc.m_3d_empty = false;
				}

				builder::adjust3d(
					cc.m_camera,
					tx * iter_ovs,
					ty * iter_ovs,
					iter_thk,
					si,
					bd.m_bi.m_em,
					bd.m_bi.m_vb,
					bd.m_bi.get_fg(),
					cur.m_root2);

			}
			if (cc.empty(3))continue;

			cur.set_btype(builder_type::E3d);
			has3d = true;
		}

		if (crz.is_tiling()) {

			auto cdp = std::pow(2.0, -crz.get_depth());
			cdp += ims_num_traits<real_number>::almost_zero();
			
			cur.m_cdp = cdp;
		} else {
			cur.m_cdp = 0;
		}

	}

	////////////////////////////////////////////////////////////////////////



	//maximum number of pixels on the last iteration
	let max_pix = (tx / lim_ts * iter_ovs) * (ty / lim_ts * iter_ovs);

	size_t ts = 1;
	if (task.use_low_res) {
		let wh = std::max(tx, ty);
		constexpr size_t min_res = 32;
		while (wh > ts * min_res) { ts *= 2; }
	}

	if ((has3d && task.build_3d) ||
		(has2dstd && task.build_2d_std) ||
		(has2dext && task.build_2d_ext))
	{
		m_ts2 = 0;//rebuild everything
	}

	//reserve a place
	if (m_ts2 == 0) {

		//you need to take this, not the current one, otherwise there will be freezes
		m_img_rend_start_time = rth.m_time_start;

		for (size_t i = 0; i < num_thumb; ++i) {
			auto& cur = *m_thumb_arr[i];

			cur.m_ts3 = 0;//reset each

			if (cur.m_buf2d_di)cur.m_buf2d_di->m_img.reserve(max_pix);
			if (cur.m_buf3d_di)cur.m_buf3d_di->m_img.reserve(max_pix);
			if (cur.m_buf_ext_di)cur.m_buf_ext_di->m_img.reserve(max_pix);
		}

		if (has2dstd) {
			m_builder2d.reserve_memory(max_pix);
		}
		if (has3d) {
			m_builder3d.reserve_memory(max_pix);
		}
		if (has2dext) {
			m_builder_ext.reserve_memory(max_pix, max_dim_set_ext);
		}
	}

	if (m_ts2 > 0) {
		ts = std::min(ts, m_ts2);
	}

	
	

	//loop through resolution levels
	for (; ts >= lim_ts; ts /= 2)
	{

		bool call_complete = false;

		m_build_scale = ts;
		let wm = 1.0 / num_thumb;
		rth.work_reset();


		////////////////////////////////////////////////////////////////

		//size of one thumbnail
		let wc = tx / ts;
		let hc = ty / ts;

		//size of the entire image
		auto& rgba = get_img_rend();
		rgba.reserve((tx / lim_ts)* (ty / lim_ts));
		rgba.recreate(wc * scr.get_nx(), hc * scr.get_ny());
		clear_color(rgba);


		const size_t oc = ts > 1 ? 1 : iter_ovs;
		const float  qa = ts > 1 ? 1.0f : iter_qua;

		//resolution of the enlarged thumbnail taking into account oversampling
		let sx = wc * oc;
		let sy = hc * oc;

		//loop through thumbnails
		for (size_t i = 0; i < num_thumb; ++i) {

			if (ims_need_stop()) {
				return;
			}

			auto& cur = *m_thumb_arr[i];
			if (cur.m_root2 == ims_max)continue;

			let& bd = *cur.m_data3;
			let& sv = bd.m_special;
			let& si = sv.m_si2;
			let root = cur.m_root2;
			let& pal = sv.chas_builtin(builtin_ids::palette) ?
				sv.m_pal : rend.m_palette;
			let& crz = sv.chas_builtin(builtin_ids::colorize) ?
				sv.m_colorize : rend.m_colorize;

			let& fog = sv.chas_builtin(builtin_ids::background) ?
				sv.m_bac.get_fog() : rend.m_background.get_fog();

			////////////////////////////////////////////////////////////////

			size_t qx, qy;
			scr.get_xy(i, qx, qy);

			//flip vertical
			let vx = qx;
			let vy = scr.get_ny() - qy - 1;


			//resulting thumbnail position
			let hx = wc * vx;
			let hy = hc * vy;

			////////////////////////////////////////////////////////////////
			//building

			if (cur.m_buf2d_di) {

				screen_params<real_number> sp;
				cur.m_pcam->m_sd.to_params(sp, sx, sy);

				rth.m_work_mul = wm;

				bool force_repaint = task.paint_2d_std;

				if (cur.m_ts3 == 0 || cur.m_ts3 > ts)
				{
					m_build_mode = build_mode::buffer;
					

					m_builder2d.m_img2.recreate(sx, sy);
					m_builder2d.m_img2.for_each([](auto& q) {q.clear_color(); });

					m_cm.init_cmaps(
						bd.m_bi.m_style,
						bd.m_bi.get_fg(),
						pal, 
						crz.shift,
						crz.type
					);
					
					m_ss.gm = &bd.m_bi.get_fg();
					m_ss.m_psi = &si;
					m_ss.ri = bd.m_bi.m_em;
					m_ss.vb = bd.m_bi.m_vb;
					m_ss.mes_mul = bd.m_bi.m_im.mes_mul;
					m_ss.icm = &m_cm;
					m_ss.cdpx = cur.m_cdp;
					

					//SDL_Log("calc_buffer1 %d, %d, %d", 	(int)m_builder2d.m_img2.w(), (int)m_builder2d.m_img2.h(),(int)ts);

					m_builder2d.calc_buffer(
						m_ss,
						qa,
						iter_thk,
						root,
						sp);

					if (ims_need_stop()) {
						return;
					}


					//SDL_Log("calc_buffer2 %d, %d",(int)m_builder2d.m_img2.w(),(int)m_builder2d.m_img2.h());

					m_builder2d.init_draw(*cur.m_buf2d_di);

					//SDL_Log("init_draw %d, %d",	(int)cur.m_buf2d_di.m_img.w(),(int)cur.m_buf2d_di.m_img.h());


					if (si.get_section_dim() == 1) {
						cur.m_square = sp.ps * cur.m_buf2d_di->num_pix / m_builder2d.m_img2.h();
					} else {
						cur.m_square = sp.ps * sp.ps * cur.m_buf2d_di->num_pix;
					}

					cur.m_ts3 = ts;
					size_t ocx = m_ts2 > 0 ? ts / cur.m_ts3 : 1;
					cur.m_ovs = oc * ocx;

					force_repaint = true;


				}//build 2d


				if (force_repaint && !cur.m_buf2d_di->m_img.empty()) {
			
					m_build_mode = build_mode::paint;
					rth.m_work_mul = 0;

					cur.m_buf2d_di->init(
						rend.m_brightness,
						rend.m_contrast,
						rend.m_border_pow,
						rend.m_inv_mode);

					to_bitmap(rgba, *cur.m_buf2d_di, hx, hy, wc, hc, cur.m_ovs);

					call_complete = true;
#if 0								
					let& pa = bd.m_bi.get_proj_data().m_projs;
					if (pa.size() == 1) {
						builder2d::draw_lattice(
							rgba,
							1,
							4,
							pa.front().m_projector,
							si,
							sp);
					}
#endif


				}//force_repaint

			} else if (cur.m_buf_ext_di) {

				screen_params<real_number> sp;
				cur.m_pcam->m_sd.to_params(sp, sx, sy);
				rth.m_work_mul = wm;

				bool force_repaint = task.paint_2d_ext;

				if (cur.m_ts3 == 0 || cur.m_ts3 > ts) {

					m_build_mode = build_mode::buffer;


					m_builder_ext.prepare(sx, sy, si, sp);


					let power = crz.params.front();

					m_ss.gm = &bd.m_bi.get_fg();
					m_ss.m_psi = &si;
					m_ss.ri = bd.m_bi.m_em;
					m_ss.vb = bd.m_bi.m_vb;
					m_ss.mes_mul = bd.m_bi.m_im.mes_mul;
					m_ss.icm = nullptr;
					m_ss.cdpx = cur.m_cdp;//not used
					

					m_builder_ext.calc_buffer(
						bd.m_bi.m_ver_dim[root],
						m_ss,
						root,
						sp.ps / 2,
						iter_qua * 4,
						power);

					if (ims_need_stop()) {
						return;
					}

					projector proj;
					proj.R = si.basis;
					proj.calc_L_ortho();

					m_builder_ext.init_draw(*cur.m_buf_ext_di, proj.L);
					cur.m_ts3 = ts;

					size_t ocx = m_ts2 > 0 ? ts / cur.m_ts3 : 1;
					cur.m_ovs = oc * ocx;

					force_repaint = true;
				}

				if (force_repaint) {
					m_build_mode = build_mode::paint;

					rth.m_work_mul = 0;

					cur.m_buf_ext_di->init(pal, crz);


					to_bitmap(rgba, *cur.m_buf_ext_di, hx, hy, wc, hc, cur.m_ovs);
					call_complete = true;
				}

			} else if(cur.m_buf3d_di) {//3D

				bool force_repaint = task.paint_3d;
				bool force_ssao = task.ssao_3d;

				if (cur.m_ts3 == 0 || cur.m_ts3 > ts) {

					m_build_mode = build_mode::buffer;

					rth.m_work_mul = wm;

					////////////////////////////////////////////////////////

					m_cm.init_cmaps(
						bd.m_bi.m_style,
						bd.m_bi.get_fg(),
						pal,
						crz.shift,
						crz.type
					);

					m_ss.gm = &bd.m_bi.get_fg();
					m_ss.m_psi = &si;
					m_ss.ri = bd.m_bi.m_em;
					m_ss.vb = bd.m_bi.m_vb;
					m_ss.mes_mul = bd.m_bi.m_im.mes_mul;
					m_ss.icm = &m_cm;
					m_ss.cdpx = cur.m_cdp;
				

					m_builder3d.calc_buffer(
						m_ss,
						sx,
						sy,
						cur.m_pcam->m_camera,
						qa,
						iter_thk,
						root,
						sv.m_light
					);

					////////////////////////////////////////////////////////

					if (ims_need_stop()) {
						return;
					}

#ifdef DEVELOPER_VERSION
					if (ts == lim_ts && num_thumb == 1) {
						m_builder3d.print_statistics();
					}
#endif
					//cannot be interrupted
					m_builder3d.init_draw(
						*cur.m_buf3d_di,
						si,
						cur.m_pcam->m_camera);

					assert(!cur.m_buf3d_di->m_ssao_ready);

					cur.m_ts3 = ts;

					size_t ocx = m_ts2 > 0 ? ts / cur.m_ts3 : 1;
					cur.m_ovs = oc * ocx;
				}

				if (force_ssao || !cur.m_buf3d_di->m_ssao_ready) {
					rth.work_reset();
					m_build_mode = build_mode::ssao;

					cur.m_buf3d_di->calc_ssao(
						rend.m_ssao_rad_perc,
						rend.m_ssao_samples);

					if (ims_need_stop()) {
						return;
					}

					force_repaint = true;
				}

				if (force_repaint) {

					m_build_mode = build_mode::paint;

					rth.m_work_mul = 0;//assume that it is instantaneous

					cur.m_buf3d_di->init(
						rend.m_brightness,
						rend.m_border_pow,
						rend.m_ssao_density,
						fog);

					to_bitmap(rgba, *cur.m_buf3d_di, hx, hy, wc, hc, cur.m_ovs);

					call_complete = true;
				}

			}//3D


		}//loop through thumbnails



		if (ims_need_stop()) {
			return;
		}

		//draw at each iteration
		m_ts2 = ts;

#ifndef NDEBUG
		//	draw_frame(rgba);
#endif//NDEBUG

		if (call_complete) {
			swap_buffer();
			on_frame_complete();
		}

	}//loop through resolution levels
}
