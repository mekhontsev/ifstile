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
#include "mprec.h"
#include "ims_num_traits.h"
#include "def_number_types.h"
#include "build_data.h"
#include "builder.h"
#include "builder2d.h"
#include "builder3d.h"
#include "builder_ext.h"
#include "gbuffer3d.h"
#include "draw_task.h"
#include "report_params.h"
#include "ims_chrono.h"

struct standard_vars;
struct oper_block;
struct visible_blocks;
struct ims_identifiers;
struct ims_info;
////////////////////////////////////////////////////////////////////////////////
enum class builder_type
{
	E2d = 0,
	E2dField = 1,
	E3d = 2,
	ENone = 3,
};

struct thumb_elem
{
	//multiple thumbnails can reference one build_data
	build_data* m_data3 = nullptr;
	
	//camera for the current fragment
	camera_ex m_cam;
	camera_ex* m_pcam=nullptr;
	
	//overrides the construction vertex (for thumbnail mode)
	size_t m_root2 = ims_max;

	
	std::unique_ptr<draw_info2d> m_buf2d_di;
	std::unique_ptr<draw_info_ext> m_buf_ext_di;
	std::unique_ptr<gbuffer3d> m_buf3d_di;

	
	builder_type get_btype() const
	{
		if (m_buf2d_di)return builder_type::E2d;
		if (m_buf_ext_di)return builder_type::E2dField;
		if (m_buf3d_di)return builder_type::E3d;
		return builder_type::ENone;
	}

	void set_btype(builder_type t) 
	{
		switch (t)
		{
		case builder_type::E2d:
			if (!m_buf2d_di)m_buf2d_di.reset(new draw_info2d);
			break;
		case builder_type::E2dField:
			if (!m_buf_ext_di)m_buf_ext_di.reset(new draw_info_ext);
			break;
		case builder_type::E3d:
			if (!m_buf3d_di)m_buf3d_di.reset(new gbuffer3d);
			break;
		case builder_type::ENone:
			break;
		}

		if (t != builder_type::E2d)m_buf2d_di.reset();
		if (t != builder_type::E3d)m_buf3d_di.reset();
		if (t != builder_type::E2dField)m_buf_ext_di.reset();
	}

	
	//at what resolution level the buffer was built
	size_t m_ts3 = 0;

	//what was the oversampling level when building
	size_t m_ovs = 1;

	//area of what was built
	double m_square = 0;

	double m_cdp = 0;//coloring depth for the edge method

	void clear()
	{
		m_data3 = nullptr;
		m_pcam = nullptr;
		m_root2 = ims_max;
		m_square = 0;
		
		m_ts3 = 0;
		m_ovs = 1;
		m_cdp = 0;
	}

	bool empty() const
	{
		return !m_data3;
	}
};




///////////////////////////////////////////////////////////////////////////////////////////

struct program_state
{
	using real_number = DefNumTypes::Real;	

	//////////////////////////////////////////////////////////////////	
	
	//data for each thumbnail
	
	std::vector<std::unique_ptr<thumb_elem>> m_thumb_arr;
	std::vector<build_data> m_build_data;

	//////////////////////////////////////////////////////////////////////////

	std::vector<size_t>	m_subsets_arr;
	////////////////////////////////////////////////////////////////////////////

	builder2d m_builder2d;
	builder3d m_builder3d;
	builder_ext m_builder_ext;

	//resolution level at which the image was built (>=1)
	//0 - means there is no resolution at all
	size_t m_ts2 = 0;

	size_t m_build_scale = 0;

	enum class build_mode
	{
		buffer,
		paint, 
		ssao
	};
	build_mode  m_build_mode = build_mode::buffer;
	
	thumb_elem* get_first_thumb_elem();
	build_data* get_first_build_data2();
	background* get_first_background();

	////////////////////////////////////////////////////////////////////////////

	std::mutex m_lock_draw;//for drawing

	std::array<ims_bitmap, 2> m_img_buf;
	uint8_t m_draw_img_idx = 0;
	ims_bitmap& get_img_draw();
	ims_bitmap& get_img_rend();

	
	//the time when the image reset its detail
	ims_chrono m_img_rend_start_time;
	ims_chrono m_img_draw_start_time;
	bool m_img_draw_uploaded = false;

	////////////////////////////////////////////////////////////////////////////

	state_stack m_ss;
	ims_cmap<real_number> m_cm;

public:

	//executes quickly
	void clear_draw_image();

	bool can_upload_img(size_t ms) const;

	//executes quickly
	void swap_buffer();

	////////////////////////////////////////////////////////////////////////////

	void init7();

	//initialization of the main block
	void set_block3(const oper_block* src, const variator_params& vp);

	//before starting construction on the main thread
	bool on_start_build(
		screen_area& scr, 
		bool thumb_list,
		size_t max_thumb, 
		const visible_blocks& vb,
		const variator_params& vp);



	//finds derived ifs, the dimensions of each set, the measure, and the moments
	//then constructs (2D, 3D, ext)
	void build_image(
		ims_identifiers& idf,
		std::function<void()> on_frame_complete,
		screen_area& scr,//by value
		draw_task task,
		report_params rp,
		bool thumb_list,//false if components are needed
		size_t max_thumb,//1 if normal mode
		bool force2d,
		const render_params& rend,
		const ifs_object_type mode,
		ims_stage& rth);

	bool save_png(
		const ims_info& nfo,
		const std::function<void(const void*, size_t)>& of,
		const render_params& rend,
		bool crop);

private:

	void thumb_resize(size_t sz);

};
