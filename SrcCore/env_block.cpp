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
#include "env_block.h"
#include "columns.h"
#include "oper_block.h"
#include "data_column.h"
#include "block_class.h"
#include "finder.h"
#include "ims_keywords.h"
#include "render_params.h"
#include "ims_identifiers.h"
#include "ims_info.h"
#include "eval_context.h"
#include "variable.h"

static constexpr int  params_version_format = 2;

bool env_block_data::s_use_fparams = true;
bool env_block_data::s_use_rparams = false;

void set_env_block(ims_identifiers& idf, oper_block& b, const env_block_data& ebd) 
{

	b.m_flags.clear_but_attr();

	b.clear_ops();
	uint32_t pos = 0;


	auto make_var = [&](std::string_view name, bool is_subs = false)
	{
		let unk_idx = idf.get_unk_id(name);
		return b.add_var(pos, unk_idx, is_subs);
	};


	b.set_integer(make_var("version"), params_version_format);
	////////////////////////////////////////////////////////////////////////////

	if (ebd.fparams) {
		let& fp = *ebd.fparams;
		b.set_integer(make_var("sDomain"), (int)fp.m_search_domain);
		b.set_double(make_var("sVariance"), fp.m_var_par.m_search_rad);
		b.set_integer(make_var("sVarMaps"), (int)fp.m_var_par.m_kernel_defect);
		b.set_integer(make_var("sEmptyMaps"), (int)fp.m_var_par.m_max_disabled);
		b.set_integer(make_var("sAttempts"), (int)fp.m_search_attempts);
		b.set_integer(make_var("sCheckNew"), fp.m_check_new_found ? 1 : 0);
		b.set_integer(make_var("sStoreTime"), fp.m_store_time_to_file ? 1 : 0);
		b.set_integer(make_var("sBruteForce"), fp.m_use_full_search ? 1 : 0);
		b.set_integer(make_var("sHideFiltered"), fp.m_hide_filtered ? 1 : 0);
		b.set_integer(make_var("sSkipMaximized"), fp.m_skip_maximized ? 1 : 0);
		b.set_integer(make_var("sIsomers"), (int)fp.m_max_isomers);
		b.set_double(make_var("sVirtual"), fp.m_virtual_quota);
		b.set_double(make_var("sComplexityMul"), fp.m_proto_complexity_mul);
		b.set_integer(make_var("sMaxBits"), (int)fp.m_max_bits);
		b.set_double(make_var("sMaxErr"), fp.m_find_prec);
	}
	
	if (ebd.cols) {
		let& cols = *ebd.cols;

		std::string tmp;

		for (size_t i = 0; i < column_id::NUM_COLS; ++i) {
			if (cols.m_col_visible[i]) {
				tmp = data_column::g_cols[i].title; tmp += "_v";
				b.set_integer(make_var(tmp), 1);
			}
		}

		for (size_t idx = 0; idx < cols.m_rule.size(); ++idx) {

			let& ra = cols.m_rule[idx];

			for (size_t i = 0; i < column_id::NUM_COLS; ++i) {
				let& r = ra[i];
				if (!r.filter)continue;

				let& src = data_column::g_cols[i];

				tmp = src.title;
				tmp += "_";
				tmp += std::to_string(idx);
				tmp += "_?";

				if (src.get_int) {
					tmp.back() = '1';	b.set_integer(make_var(tmp), r.ilim[0]);
					tmp.back() = '2';	b.set_integer(make_var(tmp), r.ilim[1]);
				} else if (src.get_float) {
					tmp.back() = '1';	b.set_double(make_var(tmp), r.lim[0]);
					tmp.back() = '2';	b.set_double(make_var(tmp), r.lim[1]);
				}
			}

		}
	}

	if (ebd.rparams) {
		let& rp = *ebd.rparams;

		b.set_double(make_var("rBrightness"), rp.m_brightness);
		b.set_double(make_var("rContrast"), rp.m_contrast);
		b.set_integer(make_var("rOversampling"), rp.m_oversamp);
		b.set_double(make_var("rQuality"), rp.m_quality);
		b.set_double(make_var("rThickness"), rp.m_thickness);
		b.set_double(make_var("rBorder"), rp.m_border_pow);
		b.set_integer(make_var("rTransparent"), rp.m_inv_mode?1:0);
		b.set_double(make_var("rAO_Radius"), rp.m_ssao_rad_perc);
		b.set_double(make_var("rAO_Amount"), rp.m_ssao_density);
		b.set_integer(make_var("rAO_Samples"), rp.m_ssao_samples);

		if (!rp.m_use_window_res) {
			b.set_integer(make_var("rResolutionX"), rp.m_resolution[0]);
			b.set_integer(make_var("rResolutionY"), rp.m_resolution[1]);
		}
	}

}


bool load_env_block(const oper_block* xb, env_block_data& ebd)
{
	let* g = xb->get_class();
	if (!g)return false;

	let* ec = xb->ctx();
	

	double v = 0;

	auto gd = [xb, g, &v, ec](std::string_view key)->bool
	{
		let var = g->find_var_by_name(key);
		if (var == ims_max)return false;
		return xb->get_double(ec->m_refs5[var].c.h, v);
	};


	if (gd("version")) {
		if ((int)v != params_version_format) {
			ims_warning("Invalid @{} version", ims_keywords::search_params_block);
			//we'll still try to download what we can
		}
	};

	//the block looks like a real one
	if (ebd.fparams) {
		auto& fp = *ebd.fparams;
		fp.set_default();


		if (gd("sDomain")) {
			v = std::floor(v);
			if (v >= 0 && v<double(search_domain::NumDomains)) {
				fp.m_search_domain = search_domain(v);
			}
		};
		if (gd("sVariance")) {
			fp.m_var_par.m_search_rad = v;
		};

		if (gd("sVarMaps")) {
			if (v >= 0) fp.m_var_par.m_kernel_defect = (size_t)v;
		};

		if (gd("sEmptyMaps")) {
			if (v >= 0) fp.m_var_par.m_max_disabled = (size_t)v;
		};

		if (gd("sAttempts")) {
			if (v >= 0) fp.m_search_attempts = (size_t)v;
		};

		if (gd("sCheckNew")) {
			fp.m_check_new_found = (v != 0);
		};

		if (gd("sStoreTime")) {
			fp.m_store_time_to_file = (v != 0);
		};

		if (gd("sBruteForce")) {
			fp.m_use_full_search = (v != 0);
		};

		if (gd("sHideFiltered")) {
			fp.m_hide_filtered = (v != 0);
		};
		if (gd("sSkipMaximized")) {
			fp.m_skip_maximized = (v != 0);
		};

		if (gd("sIsomers")) {
			if (v >= 0) fp.m_max_isomers = (size_t)v;
		};
		if (gd("sVirtual")) {
			fp.m_virtual_quota = (float)v;
		};
		if (gd("sComplexityMul")) {
			if (v >= 0) fp.m_proto_complexity_mul = (float)v;
		};
		if (gd("sMaxBits")) {
			if (v >= 0) fp.m_max_bits = (size_t)v;
		};
		if (gd("sMaxErr")) {
			if (v >= 0) fp.m_find_prec = (float)v;
		};

	}

	////////////////////////////////////////////////////////////////

	if (ebd.cols) {
		auto& cols = *ebd.cols;

		cols.init_columns(ERULE::search_first + 1, true);
		
		std::string s;

		for (size_t i = 0; i < column_id::NUM_COLS; ++i) {

			auto& cv = cols.m_col_visible[i];

			s = data_column::g_cols[i].title;
			s += "_v";

			if (gd(s)) {
				cv = (v != 0);
			} else {
				cv = false;
			};
		}
		cols.adjust_vis();

		////////////////////////////////////////////////////////////////

		for (size_t ri = 0; ri < 16; ++ri) {

			for (size_t i = 0; i < column_id::NUM_COLS; ++i) {
				let& src = data_column::g_cols[i];

				s = src.title;

				s += "_";
				s += std::to_string(ri);
				s += "_?";

				for (int q = 0; q < 2; ++q) {
					s.back() = '1' + char(q);
					if (!gd(s))continue;

					cols.resize_rules(ri + 1);
					auto& r = cols.m_rule[ri][i];
					if (src.get_int) {
						r.filter = true;
						r.ilim[q] = (int64_t)v;
					} else if (src.get_float) {
						r.filter = true;
						r.lim[q] = v;
					}
				}
			}
		}

	}

	if (ebd.rparams) {
		auto& rp = *ebd.rparams;

		if (gd("rBrightness")) {
			rp.m_brightness = v > 0 ? (float)v : 0.0f;
		};
		if (gd("rContrast")) {
			rp.m_contrast = v > 0 ? (float)v : 0.0f;
		};

		if (gd("rOversampling")) {
			rp.m_oversamp = ims_clamp((int)floor(v), -4, 16);
		};

		if (gd("rQuality")) {
			rp.m_quality = (float)ims_clamp(v, 1, 16);
		};
		if (gd("rThickness")) {
			rp.m_thickness = (float)ims_clamp(v, 1, 1024);
		};
	
		if (gd("rBorder")) {
			rp.m_border_pow = (float)v;
		};

		if (gd("rTransparent")) {
			rp.m_inv_mode = (v != 0);
		};

		if (gd("rAO_Radius")) {
			rp.m_ssao_rad_perc = (float)ims_clamp(v, 0, 100);
		};

		if (gd("rAO_Amount")) {
			rp.m_ssao_density = (float)ims_clamp(v, 0, 4);
		};
		
		if (gd("rAO_Samples")) {
			rp.m_ssao_samples = ims_clamp((int)floor(v), 100, 10000);
		};

		if (gd("rResolutionX")) {
			rp.m_resolution[0] = ims_clamp((int)floor(v), 2, 10000000);
			rp.m_use_window_res = false;
		};

		if (gd("rResolutionY")) {
			rp.m_resolution[1] = ims_clamp((int)floor(v), 2, 10000000);
			rp.m_use_window_res = false;
		};
	
	}
	
	return true;
}

