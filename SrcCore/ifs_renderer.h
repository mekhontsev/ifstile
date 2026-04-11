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

#pragma once
#include "ims_bitmap.h"
#include "ims_info.h"
#include "block_info.h"
#include "standard_vars.h"
#include "builder.h"
#include "builder2d.h"
#include "builder3d.h"
#include "builder_ext.h"
#include "gbuffer3d.h"
#include "render_params.h"
#include "integer_ims.h"
#include "neighbors_data.h"

struct report_params;

struct ifs_renderer 
{
	using real_number = double;

	static void fit1d2d(
		standard_vars& sv,
		block_info& bi,
		camera_ex& cc,
		size_t root,
		size_t tw,
		size_t th,
		float iter_thk,
		bool is2d);

	bool init(const std::string& aifs);

	bool select(std::string_view block_id, std::string_view root_id);

	bool render(ims_bitmap& dst, float quality, float thickness);

	bool information(const char* what);

	size_t max_complexity = 1000;
	size_t max_bits = 63;
	float find_prec2 = 0;//0.3 is a reasonable value

	bool calc_neighbor_graph(inter_result& ires, const integer_ims::settings& settings);
	bool custom_ifs(const report_params& rp, bool boundary_mode);

	bool set_camera(const double* camera_params, size_t num_params);

	render_params m_rp;

	builder2d m_builder2d;
	builder3d m_builder3d;
	builder_ext m_builder_ext;

	draw_info2d m_buf2d_di;
	draw_info_ext m_buf_ext_di;
	gbuffer3d m_buf3d_di;

	state_stack m_ss;
	ims_cmap<real_number> m_cm;
	std::unique_ptr<ims_info> m_nfo;

	block_info m_bi;
	standard_vars m_sv;
	const oper_block* m_bb = nullptr;

	integer_ims m_cs;
	neighbors_data m_nb;

	bool m_fit = true;
};
