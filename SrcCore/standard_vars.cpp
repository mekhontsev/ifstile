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
#include "standard_vars.h"
#include "oper_block.h"
#include "render_params.h"
#include "eval_helpers.h"
#include "ims_val.h"
#include "eval_context.h"
#include "block_graph.h"
#include "pool_ptr.h"
#include "variable.h"

void standard_vars::add_palette(oper_block& b, const palette& pal)
{
	auto ds = b.add_builtin(builtin_ids::palette);
	let& p = pal.data;
	ds = b.set_vector(ds, p.size());
	for (size_t i = 0; i < p.size(); ++i) {
		let a = b.set_vector_ex(ds + i, ETYPE::vector_imm, ESUBTYPE::real, 4);
		let& q = p[i].c;
		for (size_t j = 0; j < 4; ++j) {
			b.m_ops[a + j].f64 = q[j];
		}
	}
}

void standard_vars::add_background(oper_block& b, const background& bac)
{
	auto ds = b.add_builtin(builtin_ids::background);
	let& v = bac.data;
	let a = b.set_vector_ex(ds, ETYPE::vector_imm, ESUBTYPE::real, v.size());
	for (size_t i = 0; i < v.size(); ++i) {
		b.m_ops[a + i].f64 = v[i];
	}
}

void standard_vars::add_root(oper_block& b, size_t root_ref)
{
	auto ds = b.add_builtin(builtin_ids::root);
	b.m_ops[ds].hdr.set_reference(root_ref);
}

void standard_vars::add_colorize(oper_block& b, const colorize_params& crz)
{
	assert(!crz.empty());
	auto ds = b.add_builtin(builtin_ids::colorize);

	let& p = crz.params;

	let a = b.set_vector_ex(ds, ETYPE::vector_imm, ESUBTYPE::real, 2 + p.size());

	b.m_ops[a + 0].f64 = double(crz.type);
	b.m_ops[a + 1].f64 = crz.shift;
	for (size_t i = 0; i < p.size(); ++i) {
		b.m_ops[a + 2 + i].f64 = p[i];
	}
}


void standard_vars::add_screen_disk(oper_block& b, const screen_disk<Real>& sd)
{
	auto ds = b.add_builtin(builtin_ids::camera);
	let a = b.set_vector_ex(ds, ETYPE::vector_imm, ESUBTYPE::real, 4);
	b.m_ops[a + 0].f64 = sd.c[0];
	b.m_ops[a + 1].f64 = sd.c[1];
	b.m_ops[a + 2].f64 = sd.r;
	b.m_ops[a + 3].f64 = sd.a;
}

void standard_vars::add_lights(oper_block& b, const light_params<Real>& c)
{
	auto ds = b.add_builtin(builtin_ids::light);


	ds = b.set_vector(ds, 2 + c.m_lsource.size());
	{
		let a = b.set_vector_ex(ds + 0, ETYPE::number, ESUBTYPE::real, 1);
		b.m_ops[a + 0].f64 = c.m_ambient;
	}
	{
		let a = b.set_vector_ex(ds + 1, ETYPE::number, ESUBTYPE::real, 1);
		b.m_ops[a + 0].f64 = c.m_camera;
	}

	for (size_t i = 0; i < c.m_lsource.size(); ++i) {
		let& q = c.m_lsource[i];

		size_t num = q.is_direct ? 2 : 3;
		let a = b.set_vector_ex(ds + 2 + i, ETYPE::vector_imm, ESUBTYPE::real, num + 1);

		b.m_ops[a + 0].f64 = q.brightness;

		for (size_t j = 0; j < num; ++j) {
			b.m_ops[a + j + 1].f64 = q.p(j);
		}
	}
}

void standard_vars::add_camera(oper_block& b, const camera<Real>& c)
{
	auto ds = b.add_builtin(builtin_ids::camera);
	ds = b.set_vector(ds, 4);
	{
		let a = b.set_vector_ex(ds + 0, ETYPE::vector_imm, ESUBTYPE::real, 3);
		b.m_ops[a + 0].f64 = c.m_loc[0];
		b.m_ops[a + 1].f64 = c.m_loc[1];
		b.m_ops[a + 2].f64 = c.m_loc[2];
	}
	{
		let a = b.set_vector_ex(ds + 1, ETYPE::vector_imm, ESUBTYPE::real, 3);
		b.m_ops[a + 0].f64 = c.m_ref[0];
		b.m_ops[a + 1].f64 = c.m_ref[1];
		b.m_ops[a + 2].f64 = c.m_ref[2];
	}
	{
		let a = b.set_vector_ex(ds + 2, ETYPE::vector_imm, ESUBTYPE::real, 3);
		b.m_ops[a + 0].f64 = c.m_ver[0];
		b.m_ops[a + 1].f64 = c.m_ver[1];
		b.m_ops[a + 2].f64 = c.m_ver[2];
	}
	{
		let a = b.set_vector_ex(ds + 3, ETYPE::number, ESUBTYPE::real, 1);
		b.m_ops[a + 0].f64 = c.m_fov;
	}
}

void standard_vars::add_section(oper_block& b, const subspace_info<Real>& si)
{
	let dim = (size_t)si.origin.size();
	let rd = si.get_section_dim();

	auto ds = b.add_builtin(builtin_ids::section);
	ds = b.set_vector(ds, rd + 1);
	{
		let a = b.set_vector_ex(ds, ETYPE::vector_imm, ESUBTYPE::real, dim);
		for (size_t r = 0; r < dim; ++r) {
			b.m_ops[a + r].f64 = si.origin(r);
		}
	}
	for (size_t c = 0; c < rd; ++c) {
		let a = b.set_vector_ex(ds + c + 1, ETYPE::vector_imm, ESUBTYPE::real, dim);
		for (size_t r = 0; r < dim; ++r) {
			b.m_ops[a + r].f64 = si.basis_user(r, c);
		}
	}
}

void standard_vars::clear8()
{
	for (auto& h : m_has_builtin) {
		h = false;
	}
	m_xcam2.clear();
	m_si_empty = true;
#if 0 //TODO: understand whether it is necessary
	m_pal.clear();
	m_light.clear();
	m_colorize.clear();
	m_bac.reset();
	m_root = ims_max;
#endif
}

bool standard_vars::chas_builtin(builtin_ids bid) const
{
	return m_has_builtin[(size_t)bid];
}

void standard_vars::sync_builtins(bool add_all, oper_block& b)
{

	for (size_t i = 0; i < m_has_builtin.size(); ++i) {
		let bid = builtin_ids(i);

		if (bid == builtin_ids::subspace)continue;

		if (!m_has_builtin[i]) {
			b.remove_builtin(bid);
			if (!add_all)continue;
		}

		let rd = m_si2.get_section_dim();

		switch (bid) {
		case builtin_ids::palette:
			add_palette(b, m_pal);
			break;
		case builtin_ids::background:
			add_background(b, m_bac);
			break;
		case builtin_ids::colorize:
			add_colorize(b, m_colorize);
			break;
		case builtin_ids::section:
			add_section(b, m_si2);
			break;
		case builtin_ids::camera:
			if (rd == 1 || rd == 2) {
				add_screen_disk(b, m_xcam2.m_sd);
			} else if (rd == 3) {
				add_camera(b, m_xcam2.m_camera);
			}
			break;
		case builtin_ids::light:
			if (rd == 3) {
				//add_lights(b, m_light);
			}
			break;
		case builtin_ids::root:
		{
			let root = b.get_active_ref();
			if (root != ims_max) {
				add_root(b, root);
			}
			break;
		}
		case builtin_ids::subspace:
			break;
		default:
			assert(false);
			break;
		}
	}
}

void standard_vars::add_all_builtins(oper_block& dst, const render_params& rp, bool def_back)
{
	if (!chas_builtin(builtin_ids::palette)) {
		m_pal = rp.m_palette;
	}
	if (def_back && !chas_builtin(builtin_ids::background)) {
		m_bac = rp.m_background;
	};
	if (!chas_builtin(builtin_ids::colorize)) {
		m_colorize = rp.m_colorize;
	};


	sync_builtins(true, dst);
}



size_t standard_vars::eval_root(const oper_block& b)
{
	auto& arr = this->m_has_builtin;

	constexpr auto id_root = (size_t)builtin_ids::root;
	arr[id_root] = false;

	operator_ptr ptr;
	if (!b.get_builtin(ptr, builtin_ids::root)) {
		return ims_max;
	}

	let& h = ptr.h;
	if (h.tt == ETYPE::reference) {
		let root = h.get_offset();
		if (b.get_graph()->closed2(root)) {
			arr[id_root] = true;
			return root;
		}
	}

	ims_error("invalid root");
	return ims_max;
}

void standard_vars::eval_builtins(const oper_block& b, eval_context& ec)
{
	auto& cur = *this;
	
	constexpr auto id_sec = (size_t)builtin_ids::section;
	constexpr auto id_crz = (size_t)builtin_ids::colorize;
	constexpr auto id_pal = (size_t)builtin_ids::palette;
	constexpr auto id_bac = (size_t)builtin_ids::background;
	constexpr auto id_light = (size_t)builtin_ids::light;
	constexpr auto id_cam = (size_t)builtin_ids::camera;

	builtin_arr ptrs;
	b.get_builtins(ptrs, true);
	auto def = [&ptrs](size_t idx) {return !ptrs[idx].h.is_xundef(); };


#define eval_vec(id) pool_ptr vec(ec.eval7(ast_context{ ptrs[id], 0 }, true));


	cur.m_has_builtin[id_cam] = false;
	if (def(id_cam))
	{
		
		eval_vec(id_cam)

		if (vec) {
			let vec_len = vec->vec_length();

			pool_ptr v2(eval_helpers::to_real3(vec.get()));

			if (v2) {//2d camera
				if (vec_len == 3 || vec_len == 4) {
					let* arr = v2->p_r();
					auto& d = cur.m_xcam2.m_sd;
					d.c[0] = arr[0];
					d.c[1] = arr[1];
					d.r = arr[2];
					d.a = (vec_len == 3) ? 0 : arr[3];
					cur.m_xcam2.m_2d_empty = false;
					cur.m_has_builtin[id_cam] = true;
				} else {
					ims_error("invalid $camera size: {}", vec_len);
				}
			} else if (vec_len == 4) {//3d camera
				assert(vec->is(ims_val::EST::other));

				auto& cam = cur.m_xcam2.m_camera;

				std::array arr_cam{ &cam.m_loc, &cam.m_ref, &cam.m_ver };

				size_t j;
				auto* sa = vec->p_v();
				for (j = 0; j < 4; ++j) {

					pool_ptr v(eval_helpers::to_real3(sa[j]));

					if (!v) {
						ims_error("invalid camera component {}", j);
						break;
					}


					let* arr = v->p_r();

					if (j < 3) {
						if (!v->is(ims_val::ETP::vector, ims_val::EST::real) ||
							v->get_size() != 3)
						{
							ims_error("invalid camera component {}", j);
							break;
						}
						for (size_t k = 0; k < 3; ++k) {
							(*arr_cam[j])(k) = arr[k];
						}
					} else {
						if (!v->to_real(cam.m_fov)) {
							ims_error("invalid camera fov type {}", (int)v->gt());
							break;
						}
					}
				}

				if (j == 4) {
					cur.m_has_builtin[id_cam] = true;
					cur.m_xcam2.m_3d_empty = false;
					cam.init();
				}
			} else {
				ims_error("invalid $camera size: {}", vec_len);
			}
		} else {
			ims_error("invalid $camera");
		}

	}

	cur.m_has_builtin[id_light] = false;
#if 0
	if (def(id_light))
	{
		let& vec = ec.resolve_to_vector(ptrs[id_light], es, 0);
		if (vec->p.h.tt != ETYPE::vector) {
			ims_error("invalid light_source");
		} else {
			let sz = vec->p.h.get_u24();

			auto& ls = cur.m_light;

			size_t j;

			for (j = 0; j < sz; ++j) {

				let& ref = vec->p.a->get_ptr_to_elem(vec->p.h, j);
				let& e = vec->c->eval_real(ref, es, 0);
				let& arr = e.m_pr;

				if (j == 0) {
					if (!e.get_small_float(ls.m_ambient)) {
						break;
					}
				} else if (j == 1) {
					if (!e.get_small_float(ls.m_camera)) {
						break;
					}
				} else {
					if (!e.is(ims_val::ETP::vector, ims_val::EST::Real)) {
						break;
					}

					auto& v= ls.m_lsource.emplace_back();
					
					if (e.m_dim == 2) {
						v.is_direct = true;
						v.p(0) = arr[0];
						v.p(1) = arr[1];
					} else if (e.m_dim == 3) {
						v.is_direct = false;
						v.p(0) = arr[0];
						v.p(1) = arr[1];
						v.p(2) = arr[2];
					} else {
						break;
					}
				}
			}
			if (j == sz) {
				cur.m_has_builtin[id_light] = true;
			} else {
				ims_error("invalid light_sources");
			}
		}
	}
#endif

	cur.m_has_builtin[id_sec] = false;
	if (def(id_sec))
	{
		//origin, basis1, basis2,... 

		eval_vec(id_sec)

		if (vec) {
			let vec_len = vec->vec_length();
			if (vec_len == 0 || vec_len > 3 || !vec->is(ims_val::EST::other)) {
				ims_error("invalid $section {}", vec_len);
			} else {
				auto& si = cur.m_si2;
				size_t num_inited = 0;


				size_t dim = 0;
				auto* sa = vec->p_v();
				for (size_t j = 0; j < vec_len; ++j) {
					pool_ptr v(eval_helpers::to_real3(sa[j]));

					if (!v) {
						break;
					}

					let* arr = v->p_r();
					if (!v->is(ims_val::ETP::vector, ims_val::EST::real)){
						break;
					}

					if (v->get_size() == 0) {
						break;
					}

					if (dim == 0) {
						dim = v->get_size();
						si.resize2(dim, vec_len - 1);
						si.reset();
					} else if (dim != v->get_size()){
						break;
					}

					for (size_t k = 0; k < dim; ++k) {
						if (j == 0) {
							si.origin(k) = arr[k];
						} else {
							si.basis_user(k, j - 1) = arr[k];
						}

					}
					++num_inited;
				}


				if (num_inited > 0 && num_inited == vec_len) {
					cur.m_has_builtin[id_sec] = true;
					si.init_si();
					cur.m_si_empty = false;
				}
			}
		} else {
			ims_error("invalid $section");
		}
	}

	cur.m_has_builtin[id_pal] = false;
	if (def(id_pal))
	{

		eval_vec(id_pal)

		if (vec) {
			let vec_len = vec->vec_length();
			if (vec_len == 0 || !vec->is(ims_val::EST::other)) {
				ims_error("invalid $palette {}", vec_len);
			} else {
				auto& pal = cur.m_pal;

				let n = vec_len;

				pal.data.resize(n);

				size_t j;
				auto* sa = vec->p_v();
				for (j = 0; j < n; ++j) {
					pool_ptr v(eval_helpers::to_real3(sa[j]));
					if (!v) {
						break;
					}

					let* arr = v->p_r();

					if (!v->is(ims_val::ETP::vector, ims_val::EST::real) ||
						v->get_size() < 3 || v->get_size() > 4)
					{
						break;
					}

					auto& dst = pal.data[j];
					dst.c[3] = 1;
					dst.checked_p = true;
					for (size_t k = 0; k < v->get_size(); ++k) {
						dst.c[k] = (float)arr[k];
					}
				}

				if (j == n) {
					cur.m_has_builtin[id_pal] = true;
				}
			}
		} else {
			ims_error("invalid $palette");
		}
		
	}

	cur.m_has_builtin[id_bac] = false;
	if (def(id_bac))
	{
		auto& bac = cur.m_bac;


		eval_vec(id_bac)

		if (vec) {
			let vec_len = vec->num_vec_length();

			if (vec_len < 3 || vec_len > 4) {
				ims_error("invalid $background {}", vec_len);
			} else {
				pool_ptr v(eval_helpers::to_real3(vec.get()));
				if (v) {
					let* arr = v->p_r();
					auto& dst = bac.data;
					for (size_t k = 0; k < vec_len; ++k) {
						dst[k] = (float)arr[k];
					}
					if (vec_len == 3) {
						bac.get_fog() = 0;
					}

					cur.m_has_builtin[id_bac] = true;
				}
			}
		} else {
			ims_error("invalid $background");
		}
	}

	cur.m_has_builtin[id_crz] = false;
	if (def(id_crz))
	{
		auto& crz = cur.m_colorize;
		crz.clear();

		eval_vec(id_crz)

		if (vec) {
			let vec_len = vec->num_vec_length();

			pool_ptr v(eval_helpers::to_real3(vec.get()));

			if (!v || vec_len < 1 || size_t(v->p_r()[0]) >= colorize_params::e_numpar)
			{
				ims_error("invalid $colorize {}", vec_len);
			} else {
				let* arr = v->p_r();

				crz.type = (colorize_params::EPAR)size_t(arr[0]);
				colorize_params::set_default(crz.type, crz.params);

				if (vec_len > 1) {
					crz.shift = arr[1];
				}

				if (vec_len > 2) {
					let num = std::min(crz.params.size(), vec_len - 2);
					for (size_t j = 0; j < num; ++j) {
						crz.params[j] = arr[j + 2];
					}
				}

				cur.m_has_builtin[id_crz] = true;
			}
		} else {
			ims_error("invalid $colorize");
		}
	}
}
