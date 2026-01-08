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
#include "animator.h"
#include "eval_context.h"

static bool do_copy(builtin_arr& dst, const builtin_arr& src, builtin_ids id) 
{
	let idx = (size_t)id;

	if (src[idx].is_def() && !dst[idx].is_def()) {
		dst[idx] = src[idx];
		return true;
	}
	return false;
};

static void adjust_builtins(
	animator::keyframe& dst,
	const animator::keyframe& src)
{
	auto& dsv = dst.sv;
	auto& ssv = src.sv;

	if (do_copy(dst.ptrs, src.ptrs, builtin_ids::section)){
		dsv.m_si2 = ssv.m_si2;
		dsv.m_si_empty = ssv.m_si_empty;
	}		
	
	if (do_copy(dst.ptrs, src.ptrs, builtin_ids::camera)) {
		if (!ssv.m_xcam2.m_2d_empty && dsv.m_xcam2.m_2d_empty) {
			dsv.m_xcam2.m_sd = ssv.m_xcam2.m_sd;
			dsv.m_xcam2.m_2d_empty = false;
		}
		if (!ssv.m_xcam2.m_3d_empty && dsv.m_xcam2.m_3d_empty) {
			dsv.m_xcam2.m_camera = ssv.m_xcam2.m_camera;
			dsv.m_xcam2.m_3d_empty = false;
		}
	}

	if (do_copy(dst.ptrs, src.ptrs, builtin_ids::palette)) {
		dsv.m_pal = ssv.m_pal;
	}

	if (do_copy(dst.ptrs, src.ptrs, builtin_ids::background)) {
		dsv.m_bac = ssv.m_bac;
	}
};

bool animator::anim_init(std::vector<keyframe>& kframes, size_t ref_time)
{
	let n = kframes.size();
	for (auto& kf : kframes) {
		kf.b.get_builtins(kf.ptrs, false);//only for the block itself
	}

	//for the zero frame we take the missing ones from the first one which has builtin
	for (size_t i = 1; i < n; ++i) {
		adjust_builtins(kframes[0], kframes[i]);
	}
	//required in 2 passes. For the rest - from the nearest previous
	for (size_t i = 1; i < n; ++i) {
		for (size_t j = 0; j < i; ++j) {
			adjust_builtins(kframes[i], kframes[j]);
		}
	}

	//total dimension of the section from the hierarchy (0 - not specified)
	size_t common_sec_dim = 0;

	//single palette size from the hierarchy (0 - not specified)
	size_t common_pal_size = 0;

	//camera size 2, 3 or 0 if not found
	size_t common_cam_dim = 0;

	//one dimension for all set
	size_t common_dim = 0;


	for (size_t i = 0; i < n; ++i) {

		auto& kf = kframes[i];
		let id = kf.b.m_block_id;

		let dim = kf.b.get_dim();

		auto& sp = kf.sv;
		

		////////////////////////////////////////////////////////////////////
		size_t d = 0;
		if (!sp.m_si_empty) {
			d = sp.m_si2.get_section_dim();
		}

		size_t cd = 0;
		if (!sp.m_xcam2.m_2d_empty)cd = 2;
		else if (!sp.m_xcam2.m_3d_empty)cd = 3;

		let pal_sz = sp.m_pal.data.size();

		if (i == 0) {
			common_dim = dim;
			if (common_dim == 0) {
				ims_error("Block {}: invalid dimension", id);
				return false;
			}

			common_cam_dim = cd;
			if (common_cam_dim == 0) {
				ims_error("Block {}: camera is not defined", id);
				return false;
			}
			//////////////////////////////////

			common_sec_dim = d;
			if (common_sec_dim > 0 && common_sec_dim != common_cam_dim) {
				ims_error("Block {}: rendering dimension mismatch", id);
				return false;
			}

			common_pal_size = pal_sz;
		} else {
			if (common_dim != dim) {
				ims_error("Block {}: dimension mismatch", id);
				return false;
			}

			if (common_cam_dim != cd) {
				ims_error("Block {}: incompatible camera", id);
				return false;
			}

			if (common_sec_dim != d) {
				ims_error("Block {}: rendering dimension mismatch", id);
				return false;
			}

			if (common_pal_size != pal_sz) {
				ims_error("Block {}: incompatible palette", id);
				return false;
			}
		}

	}

	let& kf0 = kframes.front();

	
	let* e = kf0.b.ctx();

	let sz = kf0.b.num_vars();

	for (size_t j = 0; j < n;++j) {
		auto& kf = kframes[j];

		auto& ctls = kf.ctls;

		ctls.resize(sz);
		for (auto& q : ctls) {
			q.data.clear();
		}

		auto& b = kf.b;

		kf.time = (double)j;//Auto

		for(let& q: b){
			if (q.is_builtin())continue;
			let ca = q.gr();
			let ptr = b.get_ptr(q.pos5);

			if (!e->m_refs5[ca].is_var())continue;

			auto ctl_ptr = e->m_refs5[ca].c;

			while (ctl_ptr.h.tt == ETYPE::reference) {
				ctl_ptr = e->m_refs5[ctl_ptr.h.get_offset()].c;
			}

			distrib_info di;
			if (!b.get_distrib(di, ctl_ptr)) {
				continue;
			}

			if (di.t != ETYPE::distribution_real) {
				continue;
			}


			size_t num_el;

			let& h = ptr.h;

			if (ctl_ptr.h.tt == ETYPE::set_interval &&
				(h.tt == ETYPE::number || h.tt == ETYPE::number_imm)) {
				num_el = 1;
			}
			else if (ctl_ptr.h.tt == ETYPE::set_vector &&
				h.tt == ETYPE::vector_imm) {
				num_el = ctl_ptr.h.get_u24();
				if (num_el == 0)num_el = common_dim;
				if (h.get_u24() != num_el) {
					continue;
				}
			}
			else {
				continue;
			}
			///////////////////////////////////////////////
			ctls[ca].data.resize(num_el);

			for (size_t i = 0; i < num_el; ++i) {
				///////////////////////////////////////////////
				//read the current value
				double dv;
				int64_t iv = 0, denominator = 0;
				bool is_int = false;

				ims_operator op = h;
				if (op.tt == ETYPE::vector_imm) {
					op.tt = ETYPE::number;
					op.set_offset(op.get_offset() + i);
				};

				bool res = b.get_val(op, is_int, dv, iv, denominator);

				if (!res) {
					dv = 0;
				}
				else if (is_int) {
					dv = double(iv) / denominator;
				}

				ctls[ca].data[i] = dv;

				if (i==0 && ca == ref_time) {
					kf.time = dv;
				}
			};
		};
	}

	////////////////////////////////////////////////////////////////////////
	std::sort(kframes.begin(), kframes.end(), [](let& e1, let& e2) {
		return e1.time < e2.time;
	});

	for (size_t i = 0; i + 1 < n; ++i) {
		if (kframes[i].time == kframes[i + 1].time) {
			ims_error("Block {}: duplicate time", kframes[i + 1].b.m_block_id);
			return false;
		}
	}

	m_time_arr.resize(n);
	for (size_t i = 0; i < m_time_arr.size(); ++i) {
		m_time_arr[i] = kframes[i].time;
	}
	////////////////////////////////////////////////////////////////////////
	//find splines of variables
	m_spv.resize(sz);
	for (size_t ca = 0; ca < sz; ++ca) {
		let num_el = kf0.ctls[ca].data.size();
		auto& sp = m_spv[ca];
		auto* d = sp.init(num_el, n);
		if (!d)continue;

		for (size_t f = 0; f < n; ++f) {
			let& q = kframes[f].ctls[ca].data;
			for (size_t i = 0; i < num_el; ++i) {
				d[i] = q[i];
			}
			d += sp.stride();
		}
		sp.create(m_time_arr.data(), m_temp);
	}

	////////////////////////////////////////////////////////////////////////
	//find the remaining splines
	constexpr auto id_sec = (size_t)builtin_ids::section;
	constexpr auto id_pal = (size_t)builtin_ids::palette;
	constexpr auto id_cam = (size_t)builtin_ids::camera;
	constexpr auto id_bac = (size_t)builtin_ids::background;

	m_imm_sec = common_sec_dim > 0 && kf0.ptrs[id_sec].is_def();
	m_imm_pal = common_pal_size > 0 && kf0.ptrs[id_pal].is_def();
	m_imm_cam2 = common_cam_dim == 2 && kf0.ptrs[id_cam].is_def();
	m_imm_cam3 = common_cam_dim == 3 && kf0.ptrs[id_cam].is_def();

	m_imm_bac = kf0.ptrs[id_bac].is_def();
	
	if (m_imm_bac) {
		auto& bc = m_bac;
		auto& fg = m_fog;


		auto* dbc = bc.init(3, n);
		auto* dfg = fg.init(1, n);

		for (size_t f = 0; f < n; ++f) {
			let& b = kframes[f].sv.m_bac.data;
			dbc[0] = b[0];
			dbc[1] = b[1];
			dbc[2] = b[2];
			dfg[0] = b[3];
			dbc += bc.stride();
			dfg += fg.stride();
		}
		
		bc.create(m_time_arr.data(), m_temp);
		fg.create(m_time_arr.data(), m_temp);
	}

	if (m_imm_pal) {
		m_sp_pal.resize(common_pal_size);

		for (size_t i = 0; i < common_pal_size; ++i) {
			auto& sp = m_sp_pal[i];
			auto* d = sp.init(4, n);
			
			for (size_t f = 0; f < n; ++f) {
				let& pal = kframes[f].sv.m_pal.data[i].c;
				d[0] = pal[0];
				d[1] = pal[1];
				d[2] = pal[2];
				d[3] = pal[3];
				d += sp.stride();
			}
			sp.create(m_time_arr.data(), m_temp);
		}
	}

	////////////////////////////////////////////////////////////////////////

	if (m_imm_cam2) {
		 			
		auto& sp = m_sp_cam2d.pos;
		auto* dp = sp.init(2, n);
	
		auto& spa = m_sp_cam2d.a;
		auto* da = spa.init(1, n);
		
		auto& spz = m_sp_zoom;
		auto* dz = spz.init(1, n);
	
		for (size_t f = 0; f < n; ++f) {
			let& sd = kframes[f].sv.m_xcam2.m_sd;
			dp[0] = sd.c[0];
			dp[1] = sd.c[1];
			dp += sp.stride();

			dz[0] = log(sd.r);
			dz += spz.stride();

			da[0] = sd.a;
			da += spa.stride();
		}
		sp.create(m_time_arr.data(), m_temp);
		spz.create(m_time_arr.data(), m_temp);
		spa.create(m_time_arr.data(), m_temp);
	
	} else if (m_imm_cam3) {

		auto& loc = m_sp_cam3d.loc_ref;
		auto* loc_d = loc.init(3, n);
	
		auto& ref = m_sp_cam3d.ref;
		auto* ref_d = ref.init(3, n);

		auto& ver = m_sp_cam3d.ver;
		auto* ver_d = ver.init(3, n);
	
		auto& fov = m_sp_cam3d.fov;
		auto* fov_d = fov.init(1, n);
	
		auto& spz = m_sp_zoom;
		auto* dz = spz.init(1, n);
	
		for (size_t f = 0; f < n; ++f) {
			let& c = kframes[f].sv.m_xcam2.m_camera;

			//unit vector from target to camera, interpolate it
			Eigen::Vector3d loc_ref = c.m_loc - c.m_ref;
			loc_ref.normalize();

			loc_d[0] = loc_ref[0];
			loc_d[1] = loc_ref[1];
			loc_d[2] = loc_ref[2];
			loc_d += loc.stride();

			ref_d[0] = c.m_ref[0];
			ref_d[1] = c.m_ref[1];
			ref_d[2] = c.m_ref[2];
			ref_d += ref.stride();

			let v = c.adjusted_ver();
			ver_d[0] = v[0];
			ver_d[1] = v[1];
			ver_d[2] = v[2];
			ver_d += ver.stride();

			fov_d[0] = c.m_fov;
			fov_d += fov.stride();

			dz[0] = log((c.m_loc - c.m_ref).norm());
			dz += spz.stride();
		}

		loc.create(m_time_arr.data(), m_temp);
		ref.create(m_time_arr.data(), m_temp);
		ver.create(m_time_arr.data(), m_temp);
		fov.create(m_time_arr.data(), m_temp);
		spz.create(m_time_arr.data(), m_temp);
	}

	if (m_imm_sec) {

		let dim_set = (size_t)kf0.sv.m_si2.origin.size();

		{
			auto& sp = m_sp_sect.origin;
			auto* d = sp.init(dim_set, n);

			for (size_t f = 0; f < n; ++f) {
				auto& org = kframes[f].sv.m_si2.origin;
				for (size_t i = 0; i < dim_set; ++i) {
					d[i] = org(i);
				}
				d += sp.stride();
			}
			sp.create(m_time_arr.data(), m_temp);
		}

		m_sp_sect.basis_user.resize(common_sec_dim);

		for (size_t j = 0; j < common_sec_dim; ++j) {

			auto& sp = m_sp_sect.basis_user[j];
			auto* d = sp.init(dim_set, n);
		
			for (size_t f = 0; f < n; ++f) {
				auto& ba = kframes[f].sv.m_si2.basis_user;
				for (size_t i = 0; i < dim_set; ++i) {
					d[i] = ba(i, j);
				}
				d += sp.stride();
			}
			sp.create(m_time_arr.data(), m_temp);
		}
	}

	////////////////////////////////////////////////////////////////////////
	//prepare the block for interpolation

	m_offsets.resize(sz);

	auto& b = m_bl_dummy;
	kf0.b.simple_copy(b);

	auto& ctls = kf0.ctls;

	for(let& q: b){
		if (q.is_builtin())continue;

		auto& qb = b.m_ops[q.pos5].hdr;

		let num_el = ctls[q.gr()].data.size();

		m_offsets[q.gr()].data.resize(num_el);

		for (size_t i = 0; i < num_el; ++i) {
			size_t offs;

			if (qb.tt == ETYPE::number_imm) {//->number
				qb.tt = ETYPE::number;
				offs = b.m_ops.size();
				qb.set_offset(offs);
				qb.ts = ESUBTYPE::real;
				b.add(1);//b gets spoiled here
			} else {
				b.convert_type_inplace(qb, false);
				offs = qb.u32 + i;
			}

			m_offsets[q.gr()].data[i] = offs;
		}
	};

	return true;
}

void animator::interpolate(oper_block& dst, double ft)
{
	assert(ft >= 0 && ft <= 1);

	m_bl_dummy.simple_copy(dst);
	
	let tmin = m_time_arr.front();
	let tmax = m_time_arr.back();
	assert(tmin < tmax);

	double time = ft * (tmax - tmin) + tmin;

	let jdx = aspline_t::find_idx(m_time_arr.data(), m_time_arr.size(), time);

	aspline_t::internal_point time_pt, zoom_pt;
	time_pt.init(time, m_time_arr.data(), jdx);

	//adjust the time within the interval to support zoom
	double zoom_time = time;
	double lr = 0;
	if (m_imm_cam2 || m_imm_cam3) {
		m_sp_zoom.get(&lr, time_pt);

		let t0 = m_time_arr[jdx];
		let t1 = m_time_arr[jdx + 1];

		double lr0, lr1;//logarithm of the current scale	
		m_sp_zoom.get2(&lr0, t0, m_time_arr.data(), jdx);
		m_sp_zoom.get2(&lr1, t1, m_time_arr.data(), jdx);

		if (std::abs(lr0 - lr1) > ims_num_traits<double>::almost_zero()) {
			let t = (exp(lr - lr0) - 1) / (exp(lr1 - lr0) - 1);
			zoom_time = t0 + (t1 - t0) * t;//return to original range
		}
	}

	zoom_pt.init(zoom_time, m_time_arr.data(), jdx);

	////////////////////////////////////////////////////////////////////
	//interpolation of variables - use zoom time
	for (size_t ca = 0; ca < m_spv.size(); ++ca) {
		let& sp = m_spv[ca];
		if (sp.m_dim == 0)continue;
		m_temp.resize(sp.m_dim);
		sp.get(m_temp.data(), zoom_pt);

		for (size_t i = 0; i < sp.m_dim; ++i) {
			let offs = m_offsets[ca].data[i];
			dst.m_ops[offs].f64 = m_temp[i];
		}
	}
	////////////////////////////////////////////////////////////////////
	//palette interpolation - use normal time
	if (m_imm_pal) {
		auto& p = m_sv.m_pal;
		p.data.resize(m_sp_pal.size());
		for (size_t m = 0; m < m_sp_pal.size(); ++m) {
			double c[4];
			
			m_sp_pal[m].get(c, time_pt);
			for (size_t u = 0; u < 4; ++u) {
				p.data[m].c[u] = (float)c[u];
			}
		}
		standard_vars::add_palette(dst, p);
	} else {
		//calculated from the hierarchy or the global one will be used
	}
	////////////////////////////////////////////////////////////////////
	//camera interpolation - we mostly use regular time
	if (m_imm_cam2) {

		auto& c = m_sv.m_xcam2.m_sd;

		m_sp_cam2d.pos.get(c.c, zoom_pt);//use zoom time
		m_sp_cam2d.a.get(&c.a, time_pt);
		c.r = exp(lr);
		
		standard_vars::add_screen_disk(dst, c);
	} else if (m_imm_cam3) {
		auto& c = m_sv.m_xcam2.m_camera;

		Eigen::Vector3d loc_ref;
		m_sp_cam3d.loc_ref.get(loc_ref.data(), time_pt);
		loc_ref.normalize();

		m_sp_cam3d.ref.get(c.m_ref.data(), zoom_pt);//use zoom time
		m_sp_cam3d.ver.get(c.m_ver.data(), time_pt);

		//TODO: fov needs to be cleverly interpolated
		m_sp_cam3d.fov.get(&c.m_fov, time_pt);

		c.m_loc = c.m_ref + loc_ref * exp(lr);

		c.m_ver = c.adjusted_ver();

		standard_vars::add_camera(dst, c);
	} else {
		//calculated from the hierarchy
	}

	if (m_imm_bac) {

		double backd[3];
		double fogd;

		m_bac.get(backd, time_pt);
		m_fog.get(&fogd, time_pt);

		auto& b = m_sv.m_bac;
		b.data[0] = (float)backd[0];
		b.data[1] = (float)backd[1];
		b.data[2] = (float)backd[2];
		b.data[3] = (float)fogd;

		standard_vars::add_background(dst, b);
	} else {
		//calculated from the hierarchy
	}

	////////////////////////////////////////////////////////////////////
	//interpolation of the section - using zoom time
	if (m_imm_sec) {
		auto& p = m_sv.m_si2;

		let dim_set = m_sp_sect.origin.m_dim;

		let csd = m_sp_sect.basis_user.size();

		p.resize2(dim_set, csd);
		p.reset();

		m_sp_sect.origin.get(p.origin.data(), zoom_pt);

		for (size_t i = 0; i < csd; ++i) {
			m_sp_sect.basis_user[i].get(
				p.basis_user.col(i).data(), zoom_pt);
			
		}
		standard_vars::add_section(dst, p);
	} else {
		//calculated from the hierarchy
	}
}
