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
#include "gui.h"


#include "ui_creator.h"
#include "ui_animation.h"
#include "ui_examples.h"
#include "ui_finder.h"
#include "ui_file_dialog.h"
#include "ui_about.h"
#include "ui_palette.h"
#include "ui_location.h"
#include "ui_components.h"
#include "ui_filter_columns.h"
#include "ui_settings.h"
#include "ui_modal_msg.h"
#include "ui_help.h"
#include "ui_fonts.h"
#include "ui_ifs_list.h"
#include "ui_sel_visible.h"
#include "ui_render_par.h"
#include "ui_console.h"
#include "ui_editor.h"
#include "ui_search_session.h"
#include "ui_clipboard.h"

#include "IconsForkAwesome.h"

#include "program_state.h"
#include "block_class.h"
#include "file_dialog_state.h"
#include "data_column.h"
#include "ims_settings.h"
#include "finder.h"
#include "base64.h"
#include "gl_helper.h"
#include "call_thread.h"

#include "exr.h"
#include "animator.h"
#include "builder_mesh.h"
#include "mesh3d.h"
#include "voxel_volume.h"
#include "ims_file.h"
#include "flame_calc.h"
#include "info_printer.h"
#include "samples.h"
#include "error_helper.h"
#include "png_support.h"
#include "aifs_printer.h"
#include "ims_to_string_ex.h"
#include "ims_info.h"
#include "platform.h"
#include "clock_print.h"
#include "visible_blocks.h"
#include "render_params.h"
#include "ims_keywords.h"
#include "columns.h"
#include "block_converter.h"
#include "ast_stack.h"
#include "env_block.h"
#include "edge_map.h"
#include "variable.h"
#include "ovr_data.h"
#include "aifs_load.h"

#if 0
#include "imgui_virtual_keyboard.h"
#endif
////////////////////////////////////////////////////////////////////////////////

struct ws_ui
{
	ws_animation m_animation;
	ws_creator m_creator;
	ws_examples m_examples;
	ws_finder m_finder;
	ws_search_session m_graph_list;
	ws_file_dialog m_filedialog;
	ws_about m_about;
	ws_palette m_palette;
	ws_location m_location;
	ws_components m_set_selector;
	ws_filter_columns m_filter_columns;
	ws_settings m_settings;
	ws_modal_msg m_modal_msg;
	ws_help m_help;
	ws_ifs_list m_ifs_list;
	ws_sel_visible m_sel_visible;
	ws_render_par m_render_par;
	ws_console m_console;
	ws_editor m_editor;
	ws_clipboard m_clipboard;

	//returns true if a window actually had to be closed
	bool close_modals()
	{
		bool b = false;
		b = m_modal_msg.close() || b;
		b = m_about.close() || b;
		b = m_filedialog.close() || b;
		b = m_clipboard.close() || b;
		return b;
	};
};

ims_static ws_ui s_ui;



////////////////////////////////////////////////////////////////////////////////


ims_static ims_setting g_st;


////////////////////////////////////////////////////////////////////////////////
ims_static report_params g_rp;

ims_static visible_blocks g_vis;

ims_static render_params g_rpars;

ims_static samples g_samples;

ims_static file_dialog_state g_file_dialog_state;


//a procedure that should be executed in the main thread after construction is complete
//changes only in the main thread
ims_static std::function<void()> g_build_complete_proc;


ims_static program_state g_ps;


//lock while the list grows
//as well as block replacement during the search
ims_static std::mutex g_lock7;



ims_static std::unique_ptr<ims_info> g_ims_info;


ims_info& ims_info_get()
{
	if (!g_ims_info)g_ims_info.reset(new ims_info);
	return *g_ims_info;
}


ifs_list& ifs_list_get()
{
	return ims_info_get().m_list;
}

std::mutex& get_list_lock() 
{
	return g_lock7;
};


static std::string get_ini_filename()
{
	return std::string(platform::getPathPref()) + "ifstile.ini";
}
static std::string get_palette_filename()
{
	return std::string(platform::getPathPref()) + "ifstile.gpl";
}

///////////////////////////////////////////////////////////////////////
ims_static bool g_thum_enabled = false;	//thumbnail mode
ims_static bool g_thum_is_list = true;	//blocks or sets
ims_static bool g_right_select = false;	//the right button is select
///////////////////////////////////////////////////////////////////////


//file name for the entire current file
ims_static std::string g_last_file_all;


ims_static ifs_object_type g_boundary_mode = ifs_object_type::normal;


ims_static screen_area g_thumbnail;

ims_static const char* base_window_name = "Image";


//zoom/selection frame in screen coordinates
ims_static Eigen::Vector2f g_zoom_box[2] = { { 0,0 },{ 0,0 } };
ims_static bool g_zoom_box_visible = false;


ims_static bool g_show_pane = true;


ims_static uint32_t g_rate_checked = 0;

////////////////////////////////////////////////////////////////////////////////


ims_static bool g_is_check_navi = false;
////////////////////////////////////////////////////////////////////////////////


ims_static constexpr std::string_view g_url_prefix_import{ "?aifs=" };
ims_static constexpr std::string_view g_url_prefix_export{ "app.ifstile.com?aifs=" };

ims_static bool g_file_open_in_progress = false;


////////////////////////////////////////////////////////////////////////////////

SDL_Window* MainWindow_get();

static bool use_zoom_box();

void set_window_title(const char* t);
void switch_fullscreen();

static bool reset_resolution();

	

static void try_autosave();


ims_setting& get_settings() { return g_st; };



void timer_callback(size_t ms_interval)
{
	g_rate_checked = uint32_t(finder::get().get_rate() * 1000 / ms_interval);
}


void stop_build()
{
	get_thread(e_ims_threads::build).stop();
};



float& get_ui_scale()
{
	return g_st.m_ui_scale;
}

void set_ui_scale(float f)
{
	g_st.m_ui_scale = f;
	

	ImGuiStyle s;

	s.FontScaleMain = f;

	s.ChildRounding = 0;
	s.ChildBorderSize = 0;

	s.ScaleAllSizes(f);
	for (size_t i = 0; i < std::size(s.Colors); ++i) {
		s.Colors[i] = ImGui::GetStyle().Colors[i];
	}
	s.FrameBorderSize = 1;
	s.FrameRounding = 4;

	s.Colors[ImGuiCol_WindowBg].w = 1.0f;

	ImGui::GetStyle() = s;
}

samples& get_samples()
{
	return g_samples;
}

render_params& get_rpars()
{
	return g_rpars;
};


static void ims_write_sync() 
{
#ifdef __EMSCRIPTEN__
	auto em_syncfs = []()
	{
		EM_ASM_({
			FS.syncfs(false, function() {})
		});
	};
	if(ims_worker::is_main_thread()){
		em_syncfs();
	} else{
		call_main_thread(em_syncfs);
	}
#endif
};


file_dialog_state& get_fds()
{
	return g_file_dialog_state;
}

visible_blocks& get_vb()
{
	return g_vis;
};

oper_block* get_cur_block()
{
	return get_vb().get_cur_block();
};


colorize_params& get_colorize_params()
{
	auto* bd = get_global_bd();
	if (bd && bd->m_special.chas_builtin(builtin_ids::colorize)) {
		return bd->m_special.m_colorize;
	}
	return get_rpars().m_colorize;
}


palette& get_cur_palette()
{
	auto* bd = get_global_bd();
	if (bd && bd->m_special.chas_builtin(builtin_ids::palette)) {
		return bd->m_special.m_pal;
	}
	return get_rpars().m_palette;
}

background& get_background()
{
	auto* b = g_ps.get_first_background();
	return b ? *b : get_rpars().m_background;
}

float* get_background_data_rgb()
{
	return get_background().data.data();
}

build_data* get_global_bd()
{
	auto* xd = g_ps.get_first_build_data2();
	if (!xd)return nullptr;
	return xd;
};

static oper_block* get_global_block(ifs_object_type t)
{
	auto* xd = get_global_bd();
	if (!xd)return nullptr;
	return xd->get_direct(t);
};

static ifs_object_type get_build_mode()
{
	return g_boundary_mode;
};


oper_block* get_global_block()
{
	return get_global_block(get_build_mode());
};

void ui_update_maps()
{
	auto* xd = get_global_bd();
	if (!xd || xd->empty()) return;
	xd->m_bi.recalc_graph();
	xd->m_changed = true;
}

static void do_fit_to_screen()
{
	stop_build_then([]() {
		draw_task dt;
		dt.fit = true;
		do_build(dt);
	});
}

void rand_current_set()
{
	ims_func_static variator_ex g_vex;

	auto* bd = get_global_bd();
	if (!bd || bd->empty()) return;

	stop_build_then([bd]() {

		auto nb = std::make_unique<oper_block>();

		variator_params vp = finder::get().m_var_par;
		vp.m_change_all = true;
		vp.m_kernel_defect = ims_max;

		let nv = g_vex.variate(
			*nb,
			*bd->m_block_sq,
			vp,
			ims_random::getR());

		if (nv > 0) {
			std::swap(bd->m_block_sq, nb);

			ui_update_maps();

			do_fit_to_screen();
		};

	});
}

static build_data* get_cur_for_save(ifs_object_type t)
{
	auto* xd = get_global_bd();
	if (!xd || !xd->m_bi.exists())return nullptr;
	xd->m_special.sync_builtins(false, *xd->get_direct(t));
	return xd;
}

struct ims_worker& get_thread(e_ims_threads v)
{
	return ims_worker::get_thread((size_t)v);
};

void on_change_store_time_flag(bool b)
{
	auto& lst = ifs_list_get();
	for (let id : lst.m_blocks) {
		auto* q = lst.get_block(id);
		q->m_flags.has_timestamp = b;
	}
}

report_params& get_report_params()
{
	return g_rp;
};





void init6(bool need_save)
{
	g_ps.init7();
	get_vb().init9(ifs_list_get());

	ims_info_get().m_need_save = need_save;
}


void set_block_direct(const oper_block* b)
{
	let& vp = finder::get().m_var_par;
	g_ps.set_block3(b, vp);
}

void set_block_ex(size_t idx)
{
	let* src = get_vb().get_vis(idx);
	if (!src) {
		assert(false);
		return;
	}
	get_vb().m_cur_block_pos = idx;

	set_block_direct(src);
}


bool is_thumb_enabled()
{
	return g_thum_enabled;
}

static bool is_thumb_list() 
{
	return g_thum_enabled && g_thum_is_list;
}

static bool is_thumb_comp()
{
	return g_thum_enabled && !g_thum_is_list;
}

bool editor_ready()
{
	return !is_thumb_list();//allowed in component mode
}

void clear_before_load()
{
	//prevent the help block from crashing when CTRL-R is pressed
	for (auto& q : g_ps.m_build_data) {
		q.clear();
	}

	ims_err_reset();

	g_ims_info.reset();
};


//may take a long time to work since it initializes the entire list up to the appropriate one
size_t get_default_block()
{
	//try the first visible block without the hidden flag
	let& vb = get_vb().m_vis_blocks;
	for (size_t i = 0; i < vb.size(); ++i) {
		let* q = vb[i];
		if (!q->m_flags.hidden) {
			return i;
		}	
	}

	return 0;//first in the list
}


static std::string create_filename_for_current_block()
{
	let* b = get_cur_block();
	if (!b) {
		return "";
	}

	std::string ret;
	if (b->m_name.empty()) {
		ret = std::to_string(b->m_block_id);
	}
	else {
		ret = b->m_name;
		std::replace(ret.begin(), ret.end(), '.', '_');
		std::replace(ret.begin(), ret.end(), ' ', '_');
	}

	return ret;
}

static std::string create_filepath_for_current_block()
{
	return get_settings().m_last_folder + create_filename_for_current_block();
}


static void update_bitmap()
{
	if (g_ps.can_upload_img(min_build_time_ms)) {
		update_ui_async();
	}

}

bool timer_callback100ms(size_t)
{
	return !g_ps.m_img_draw_uploaded;
}



void on_change_max_thumb(size_t new_val)
{
	stop_build_then([new_val] {
		get_settings().m_max_thmb = (size_t)new_val;
		if (is_thumb_enabled()) {
			do_fit_to_screen();
		};
	});
};


void set_last_file(const std::string& filename, bool set_name, bool set_folder)
{
	std::string name;

	let par = ims_file::get_parent(ims_file::adjust(filename), &name);

	if (set_name) {
		g_last_file_all = name;
		set_window_title(filename.c_str());
	}
	if (set_folder && !par.empty()) {
		get_settings().m_last_folder = par;
	}
}



#if 0
static void stop_backround_tasks()
{
	constexpr std::array ts =
	{
		e_ims_threads::search,
		e_ims_threads::aux
	};


	for (let t : ts)get_thread(t).stop();

	for (;;) {
		bool ready = true;
		for (let t : ts) {
			if (get_thread(t).is_running()) {
				ready = false;
			}
		}
		if (ready)break;
	}
};
#endif

static void stop_search_then(std::function<void()>&& f)
{
	assert(ims_worker::is_main_thread());

	auto& t = get_thread(e_ims_threads::search);

	if (!t.is_running()) {
		f();
	}
	else {
		t.stop();


		auto lambda = [g = std::move(f)]() mutable {
			stop_search_then(std::move(g));
		};

		main_thread::post(lambda);
	}
};




void stop_build_then(std::function<void()>&& f, uint64_t time_ms)
{
	assert(ims_worker::is_main_thread());

	auto& t = get_thread(e_ims_threads::build);

	if (!t.is_running()) {
		//became irrelevant because a new one appeared
		g_build_complete_proc = nullptr;
		f();
	}
	else {
		g_build_complete_proc = std::move(f);
		t.stop(time_ms);
	}
};





static std::string get_fullpath_for_current_file()
{
	return get_settings().m_last_folder + g_last_file_all;
}


////////////////////////////////////////////////////////////////////////////////
static bool build_mesh(
	mesh& msh,
	const block_info& bi,
	size_t root,
	const subspace_info<double>& si,
	const palette& pal,
	const colorize_params& crz
)
{
	box<double> bx;

	let& gm = bi.get_fg();
	let& rp = get_rpars();
	auto& cm = g_ps.m_cm;
	auto& ss = g_ps.m_ss;
	

	builder::adjust_box(
		bx,
		0.1,
		si,
		bi.m_em,
		bi.m_vb,
		gm,
		root);

	auto& nfo = *ims_worker::get();

	builder_mesh mesh_builder;
	voxel_volume vol;

	vol.init(bx, rp.m_mesh_resolution, rp.m_mesh_colors);

	////////////////////////////////////////////////////////////////////
	nfo.m_stage_name = "Building...";
	nfo.work_reset();

	cm.init_cmaps(
		bi.m_style,
		bi.get_fg(),
		pal,
		crz.shift,
		crz.type == colorize_params::e_vertex
	);

	auto cdp = std::pow(2.0, -crz.get_depth());
	cdp += ims_num_traits<double>::almost_zero();

	ss.gm = &gm;
	ss.m_psi = &si;
	ss.ri = bi.m_em;
	ss.vb = bi.m_vb;
	ss.mes_mul = bi.m_im.mes_mul;
	ss.icm = &cm;
	ss.cdpx = cdp;
	

	bool res = mesh_builder.calc_buffer(
		ss,
		vol,
		rp.m_quality,
		root);

	if (ims_need_stop()) {
		return  false;
	}

	if (!res) {
		ims_error("Failed to create mesh");
		return  false;
	}

	////////////////////////////////////////////////////////////////////
	nfo.m_stage_name = "Mesh generation...";
	nfo.work_reset();

	vol.triangulate(msh);
	if (ims_need_stop())return  false;

	vol.clear_mem();//no more data needed

	nfo.m_stage_name = "Post processing...";
	nfo.work_reset();

	msh.post_process();
	if (ims_need_stop())return false;
	nfo.work_add(0.7);

	msh.calc_normals();
	vol.revert_to_space(msh);



	return true;

};

static bool save_mesh(
	std::ostream& stream,
	const block_info& bi,
	size_t root,
	const subspace_info<double>& si,
	const palette& pal,
	const colorize_params& crz
)
{
	mesh msh;

	let res = build_mesh(
		msh,
		bi,
		root,
		si,
		pal,
		crz);

	if (!res)return false;

	auto& nfo = *ims_worker::get();
	nfo.m_stage_name = "Saving...";
	nfo.work_reset();

	msh.save_ply(stream);

	return true;
};





static void switch_thumbnail()
{
	stop_build_then([]() {

		g_thum_enabled = !g_thum_enabled;
	do_rebuild_sync();
		}, 0);//instant stop
};

static size_t get_rel_block(int step)
{
	let sz = get_vb().m_vis_blocks.size();

	if (sz < 2)return false;//nothing can change

	int nt;
	
	if (!is_thumb_list()) {
		nt = 1;
	}else {
		size_t hx, hy;
		nt = (int)g_thumbnail.calc_thumb(hx, hy, ims_max, get_settings().m_max_thmb);
	}

	return (size_t)ims_clamp(
		static_cast<int>(get_vb().m_cur_block_pos) + step * nt, 0, (int)sz - 1);
};

void set_new_ver_ref(size_t new_ver_ref)
{
	stop_build_then([new_ver_ref]() {
		auto* xd = get_global_bd();
		assert(xd->m_bi.exists());//TODO: if you press several buttons to the right and down at the same time
		if (!xd->m_bi.exists())return;
		xd->get_direct(get_build_mode())->set_active_ref(new_ver_ref);
		xd->adjust_roots(get_build_mode());
		do_fit_to_screen();
	});

}



static bool change_set(int step, bool cycle)
{
	auto* xd = get_global_bd();
	if (!xd || !xd->m_bi.exists())return false;
	

	if (is_thumb_comp()) {
		size_t hx, hy;
		step *= (int)g_thumbnail.calc_thumb(hx, hy, ims_max,
			get_settings().m_max_thmb);
	}

	let new_ref = s_ui.m_set_selector.m_brefs.change_set2(*xd, step, cycle);

	if (new_ref == ims_max) {
		return false;
	}

	set_new_ver_ref(new_ref);
	return true;
}


static bool change_block(int step, bool next_uniq)
{
	let& vb = get_vb();
	let& vis = vb.m_vis_blocks;

	if (vis.empty()) {
		return false;
	}

	size_t new_block = vb.m_cur_block_pos;
	if (next_uniq) {

		let* cur = get_cur_block();
		if (!cur)return false;

		let& col = data_column::g_cols[columns::get().m_last_sorted_idx];

		for (;;) {
			if (step < 0) {
				if (new_block == 0) {
					break;
				};
				--new_block;
			}
			else {
				++new_block;
				if (new_block >= vis.size()) {
					new_block = vis.size() - 1;
					break;
				}
			}

			let* b = vb.get_vis(new_block);
			if (!b)return false;

			if (!col.is_same(b, cur)) {
				break;
			}
		}
	}
	else {
		new_block = get_rel_block(step);
	}

	if (new_block == vb.m_cur_block_pos)return false;

	set_block_and_build(new_block);

	return true;
};

static bool ims_is_base64(std::string_view str) 
{
	auto p = str.find(g_url_prefix_import);
	if (p != str.npos) {
		str.remove_prefix(p + g_url_prefix_import.length());
		p = str.find_first_of('&');
		if (p != str.npos) {
			str.remove_suffix(str.length() - p);
		}
	}

	return is_base64(str);
}


//if decoding failed, returns an empty string
static std::string try_decode_base64(std::string_view str)
{
	auto p = str.find(g_url_prefix_import);
	if (p != str.npos) {
		str.remove_prefix(p + g_url_prefix_import.length());
		p = str.find_first_of('&');
		if (p != str.npos) {
			str.remove_suffix(str.length() - p);
		}
	}

	if (!ims_is_base64(str)) {
		return "";
	}

	std::string dec;
	if (!base64_decode(str.data(), str.length(), dec)) {
		return "";
	};

	return dec;
}




void do_rebuild()
{
	stop_build_then([]() {
		do_rebuild_sync();
	}, 0);//instant stop
};


void on_change_color()
{
	if (ifs_list_get().empty())return;

	draw_task dt;
	dt.build_2d_std = true;
	dt.build_3d = true;
	dt.paint_2d_ext = true;
	do_build(dt);
};

static bool set_help_block()
{
	s_ui.m_help.set_mode(ws_help::mode::block);

	let* xd = get_global_bd();
	if (xd && !xd->empty()) {
		s_ui.m_help.m_cur_text = xd->m_block_sq->get_block_decription();
		return !s_ui.m_help.m_cur_text.empty();
	}

	s_ui.m_help.m_cur_text.clear();
	return false;
}

static bool set_help_file()
{
	s_ui.m_help.set_mode(ws_help::mode::file);
	s_ui.m_help.m_cur_text = ims_info_get().m_js_description;
	return !s_ui.m_help.m_cur_text.empty();
}


void set_block_and_build(size_t idx)
{
	stop_build_then([idx]() {
		set_block_ex(idx);
		s_ui.m_ifs_list.scroll_to_row(idx);

		if (s_ui.m_help.m_mode2 == ws_help::mode::block) {
			set_help_block();
		};

		do_rebuild_sync();
	});
};

static void scroll_to_last_vis()
{
	let sz = get_vb().m_vis_blocks.size();
	assert(sz > 0);
	if (sz == get_vb().m_cur_block_pos + 1) {
		return;//already the last one
	}

	let idx = sz - 1;

	if (s_ui.m_ifs_list.m_find_scroll) {
		s_ui.m_ifs_list.scroll_to_row(idx);
	}


	if (!s_ui.m_ifs_list.m_find_build) {
		return;
	}

	//let the previous one fully build up
	if (get_thread(e_ims_threads::build).is_running()) {
		return;
	}

	if (!get_settings().m_save_every_picture) {//forces to build
		if (is_program_minimized()) {
			return;
		}

		if (g_show_pane &&
			get_settings().m_window_mode == window_mode_type::full &&
			get_settings().m_backgr_alpha >= 1.0f)
		{
			//the image is not visible
			return;
		}
	}


	auto lambda = [idx]() {
		stop_build_then([idx]() {
			set_block_ex(idx);
			do_rebuild_sync();
		});
	};
	main_thread::post(lambda);
}


ims_static ims_chrono g_init_started;

static void finder_cb(finder::check_result r, oper_block* sr)
{
	switch (r)
	{
	case finder::init_started:
		g_init_started = finder::get().m_start_time;
		return;
	case finder::search_started:
	{
		let df = ims_chrono::dif_micro(g_init_started, finder::get().m_start_time);
		std::cout << "Search is initialized: " << df * 1e-6 << " s" << std::endl;
		return;
	}
	case finder::new_found:
	{
		std::scoped_lock lock(get_list_lock());

		//the block has already been added to the main list
		let ret = get_vb().append_block(sr);
		if (ret)scroll_to_last_vis();
		return;
	}
	case finder::best_replaced:
	case finder::some_replaced:
		ims_info_get().m_need_save = true;
		return;
	case finder::interrupted:
	{
		std::cout << "interrupted at: " << std::endl;
		ims_write_block(std::cout, sr);
		return;
	}
	default:
		break;
	}

};


static void do_copy_to_clipboard_ex(
	std::vector<const oper_block*>& arr,
	bool base64,
	bool ignore_js)
{
	std::ostringstream stream;

	ims_precision prec(stream);
	prec.template max<DefNumTypes::Real>();

	{
		std::scoped_lock lock(get_list_lock());
		aifs_printer de;
		auto& nfo = ims_info_get();
		if (ignore_js) {
			nfo.print_js(stream);
		}
		de.ims_to_text(stream, nfo.m_list, arr, false, false, ignore_js);
	}

	auto s = stream.str();
	if (s.empty())return;

	boost::algorithm::trim(s);

	if (base64) {
		while (s.length() % 3)s += ' ';//so that there is no '=' at the end
		std::string enc;
		base64_encode(s.c_str(), s.length(), enc);
		s = std::string(g_url_prefix_export) + enc;
	}

	platform::ims_to_clipboard(s);
};


void do_copy_all(bool base64)
{
	std::vector<const oper_block*> arr;
	do_copy_to_clipboard_ex(arr, base64, true);
}

//merge_parents - merge with all parents
 void do_copy_to_clipboard(bool base64, bool merge_parents)
{
	bool err = true;
	IMS_SCOPE([&] {
		if (err) {
			ims_show_message("Nothing to copy");
		};
	});

	let* bi = get_cur_for_save(get_build_mode());

	const oper_block* b;
	if (bi) {
		b = &bi->get_block();
	} else if (get_build_mode() == ifs_object_type::normal) {
		b = get_cur_block();
	}else {
		return;
	}


	b = b->elevate_empty();
	if (!b)return;

	auto nb = std::make_unique<oper_block>();
	std::vector<const oper_block*> arr(1);

	ovr_data opod;

	if (merge_parents) {
		opod.init(*b);
		opod.merge_from(*nb, *b);

		//TODO: we can also delete the expressions a=b

		//TODO: doesn't work for builtin dependent variables
		//nb->remove_unused_geom();

		arr.front() = nb.get();
	}
	else {
		arr.front() = b;
	}

	do_copy_to_clipboard_ex(arr, base64, !merge_parents);

	err = false;
};



static size_t save(
	std::ostream& of,
	ifs_object_type mode,
	const save_type st)
{

	std::vector<const oper_block*> arr;

	if (st == save_type::single) {
		//the current block may not even be in the list
		let* bi = get_cur_for_save(mode);

		const oper_block* b;
		if (bi) {
			b = &bi->get_block();
		}
		else if (mode == ifs_object_type::normal) {
			b = get_vb().get_cur_block();
		}
		else {
			return 0;
		}

		arr.emplace_back(b);
	}

	
	std::scoped_lock lock(get_list_lock());

	aifs_printer de;
	auto& nfo = ims_info_get();
	nfo.print_js(of);
	return de.ims_to_text(
		of,
		nfo.m_list,
		arr,
		st == save_type::checked,
		st == save_type::single,
		true
	);
	
};





static bool save_fts(std::ostream& of)
{
	auto* bp = g_ps.get_first_thumb_elem();
	if (!bp)return false;
	let* bd = bp->m_data3;
	if (!bd || !bd->m_bi.exists())return false;
	let& sv = bd->m_special;

	camera<DefNumTypes::Real> cam;

	let dim_proj = bd->m_bi.common_dim_proj();

	if (dim_proj == 3) {
		cam = bp->m_pcam->m_camera;
	} else if (dim_proj == 2) {
		let& s = bp->m_pcam->m_sd;
		cam.m_ref << s.c[0], s.c[1], 0;
		cam.m_loc = cam.m_ref;
		cam.m_loc[2] -= s.r;
		cam.m_ver << 0, 1, 0;
		cam.m_fov = 90;
	} else {
		return false;
	};

	let& pal = sv.chas_builtin(builtin_ids::palette) ?
		sv.m_pal : get_rpars().m_palette;

	ims_to_fractracer(
		of,
		dim_proj,
		bd->m_bi.get_fg(),
		cam,
		bd->get_froot(),
		bd->m_bi.m_em,
		pal
	);

	return true;
};



static void try_autosave()
{
	if (!get_settings().m_save_every_picture) {
		return;
	}

	let fn = get_settings().m_last_picture_folder + create_filename_for_current_block();

	std::string filename = fn + ".png";
	for (size_t i = 1;; ++i) {
		if (!ims_file::is_exists(filename))break;
		filename = fn + "_" + std::to_string(i) + ".png";
	}

	std::ofstream fs;
	ims_file::open(fs, filename);
	if (fs.bad()) {
		ims_warning("Could not save file: {}", filename);
		return;
	}
	g_ps.save_png(
		ims_info_get(),
		[&fs](const void* p, size_t sz) {fs.write((const char*)p, sz); },
		get_rpars(),
		true);
}


static void set_build_mode(ifs_object_type t)
{
	if (get_build_mode() == t)return;

	stop_build_then([t]() {
		auto* xd = get_global_bd();
		if (!xd){
			return;
		}
		xd->on_change_mode();
		g_boundary_mode = t;
		do_rebuild_sync();
	});
};

static void do_zoom_out()
{
	if (g_thumbnail.get_num() > 1)return;

	stop_build_then([]() {

		auto* bp = g_ps.get_first_thumb_elem();
		if (!bp || !bp->m_data3)return;
		auto& sv = bp->m_data3->m_special;

		const int zoom_out_coeff = 8;

		auto& xc = sv.m_xcam2;
		if (sv.m_si2.get_section_dim() <= 2) {
			if (xc.empty(2))return;
			xc.m_sd.r *= zoom_out_coeff;
		}
		else {
			if (xc.empty(3))return;
			bp->m_buf3d_di->zoom_out_cam(xc.m_camera, 
				s_ui.m_location.m_lock_dist_target);
		}

		do_rebuild_sync();
	});
}


static void close_fds_ex(bool success)
{
	auto& sp = get_fds();
	sp.close_stream();

	if (!success) {
		platform::remove_file(sp.get_full_name().c_str());
#ifdef DEVELOPER_VERSION
		SDL_Log("close_fds_ex: success = false");
#endif // DEVELOPER_VERSION
		return;
	}

#if defined(__EMSCRIPTEN__)
	if (sp.m_download && sp.m_type == file_dialog_state::type::save) {
		call_main_thread([]() {
			void Emscripten_download(std::string_view fn);
			Emscripten_download(get_fds().get_full_name());
		});
};
#endif
}

static void close_fds()
{
	close_fds_ex(true);
};

static void export_csv(const std::string&)
{
	auto& ofs = get_fds().m_fw.stream();
	IMS_SCOPE(close_fds);

	//header
	for (size_t v = 0; v < column_id::NUM_COLS; ++v) {
		if (!columns::get().m_col_visible[v])continue;
		let& h = data_column::g_cols[v];
		ofs << h.title << ";";
	}
	ofs << "\r\n";

	std::string s;

	for (size_t i = 0;; ++i) {
		let* sr = get_vb().get_vis(i);
		if (!sr)break;

		for (size_t v = 0; v < column_id::NUM_COLS; ++v) {
			if (!columns::get().m_col_visible[v])continue;
			data_column::g_cols[v].get_column_str(*sr, s, true);
			ofs << s << ";";
		}
		ofs << "\r\n";
	}

};

template<typename F>
void console_compute(const F& f)
{
	auto& ta = get_thread(e_ims_threads::aux);

	if (ta.is_running()) {
		return;
	}


	auto* xd = get_global_bd();
	if (!xd || !xd->m_bi.exists())return;


	ta.start([xd, f]() {
		set_thread_name("Computing");
		ims_worker::get()->m_stage_name = "Computing...";
		f(&xd->m_bi);
		redraw_gui(1);
	});
};





void console_print(e_what_print what)
{
	ims_precision prec(std::cout);
	prec.template max<DefNumTypes::Real>();


	auto* xd = get_global_bd();
	if (!xd || !xd->m_block_sq)return;

	auto& sr = xd->get_block();
	auto& bi = xd->m_bi;

	

	switch (what) {
	case e_what_print::Definition:
		print_ifs_def(sr);
		break;
	case e_what_print::AST:
		print_ast(sr, bi, g_ps.m_am);
		break;
	case e_what_print::Data:
	{
		let* b = get_cur_block();
		if (b)print_ifs_data(*b);
		break;
	}
	case e_what_print::Evaluation:
		print_ifs_eval(sr, g_ps.m_ctx);
		break;
	case e_what_print::NormalMaps:
		print_normal_maps(sr, g_ps.m_ctx);
		break;
	case e_what_print::Projection:
		//can only print the current block
		print_ifs_proj(sr, bi);
		break;
	case e_what_print::Dimension:
		console_compute([](const block_info* bi) {
			print_dimensions(*bi);
		});
		break;
	case e_what_print::Geometry:
		console_compute([&](const block_info* bi) {
			print_balls(sr, bi);
			print_diams(sr, bi);
		});
		break;
	case e_what_print::Measure:
		print_measure(sr, &bi);	
		break;
	case e_what_print::Subspaces:
	{
		console_compute([&](const block_info* bi) {
			print_subspaces(sr, bi);
		});
		break;
	}
	

#if 0		
	case 7:
		print_ifs_value(user_input_value.data());
		break;
#endif
	}
}



bool add_block2(std::unique_ptr<oper_block>& nb, std::string_view id)
{
	std::scoped_lock lock(get_list_lock());
	auto* sr = nb.get();//because it will be moved

	sr->fix_js_parent();
	
	ifs_list_get().move_block(nb, id);
	return get_vb().append_block(sr);
}

//move the block's child to the list
void add_view_to_list(build_data* bd, const oper_block* bb)
{
	assert(bd->can_create_view());
	auto nb = std::make_unique<oper_block>();
	nb->inherit_view(*bb);

	static size_t view_index = 0;
	nb->m_name += " view ";
	nb->m_name += std::to_string(view_index++);

	bd->m_special.add_all_builtins(*nb, get_rpars(), true);
	add_block2(nb, "");
};


void set_id_for_checked() 
{
	auto& lst = ifs_list_get();

	//m_list will change, references to elements will be invalidated
	let sz = lst.m_blocks.size();
	for (size_t i = 0; i < sz; ++i) {//loop through converters
		auto* c = lst.get_block_by_idx(i);
		if (!c->m_flags.checked)continue;
		if (!c->str_id4().empty())continue;

		let prefix = std::string("B") + std::to_string(c->m_block_id);
		let str_id = lst.m_idf.gen_unique_block_id(prefix);

		auto& d = lst.m_idf.get_data(str_id);
		assert(!d.has_block());
		
		d.block_id = c->m_block_id;
		lst.m_id2data[c->m_block_id].m_str_id = d.unk_id;
	}
};

//returns how many new ones were created
size_t apply_converters()
{
	let& lst = ifs_list_get();
	block_converter converter;

	
	size_t num = 0;

	//m_list will change, references to elements will be invalidated
	let sz = lst.m_blocks.size();
	for (size_t i = 0; i < sz; ++i) {//loop through converters

		let* c = lst.get_block_by_idx(i);
		
		if (!c->m_flags.checked)continue;

		if (!c->is_converter())continue;

		let* to = lst.get_block(c->m_conv_id);
		if (!to)continue;

		converter.init(*c, to);
		size_t num_blocks = get_vb().m_vis_blocks.size();

		for (size_t idx = 0; idx < num_blocks; ++idx) {//loop through all visible
			let* cur = get_vb().get_vis(idx); if (!cur)break;

			if (!cur->m_flags.checked) {
				continue;
			}
			let* p = cur->get_parent();
			if (c->get_parent() != p)continue;


			check_block(cur);

			if (!cur->m_graph) {
				continue;
			}
			
			if (!cur->m_flags.only_var) {
				continue;
			}
		
			auto dst = std::make_unique<oper_block>();

			if (converter.convert(*dst, *cur)) {
				add_block2(dst, "");
				++num;
			}
		}
	}

	//leave checked only for newly created blocks
	for (size_t i = 0; i < sz; ++i) {
		lst.get_block_by_idx(i)->m_flags.checked = false;
	}

	get_vb().update_num_checked(lst);

	return num;
}



affine_calc& get_global_ac()
{
	return g_ps.m_bc;
};

eval_info& get_global_ei()
{
	return g_ps.m_ev;
};


void show_generic_error_msg()
{
	ims_show_message("Not applicable");
}

////////////////////////////////////////////////////////////////////////////////

void remove_search()
{
	auto& lst = ifs_list_get();
	for (let id : lst.m_blocks) {
		auto* q = lst.m_id2data[id].b.get();
		q->remove_search();
	}
	finder::get().init();
}



void remove_checked()
{
	auto& lst = ifs_list_get();

	//deletion flag, do not delete if the selected element is NOT in the visible list
	for (let id : lst.m_blocks) {
		auto* q = lst.get_block(id);
		q->m_flags.marked = false;
	}

	//we delete only those that are visible
	for (auto* ob : get_vb().m_vis_blocks) {
		(const_cast<oper_block*>(ob))->m_flags.marked = ob->m_flags.checked;
	}


	//those that can't be removed grow,
	//we constantly add to them those on whom they depend
	std::vector<oper_block*> arr;

	for (let id : lst.m_blocks) {
		auto* q = lst.get_block(id);
		if (!q->m_flags.marked) {
			arr.emplace_back(q);
		}
	}

	//remove the deletion flag from all references to undeletables
	//the array will grow longer as iterated
	ast_stack ai;
	for (size_t i = 0; i < arr.size(); ++i) {
		auto* b = arr[i];
		assert(!b->m_flags.marked);

		if (b->get_parent() && b->get_parent()->m_flags.marked) {
			auto* p = const_cast<oper_block*>(b->get_parent());
			if (p->m_flags.marked) {
				p->m_flags.marked = false;
				arr.emplace_back(p);
			}
		}
		if (b->is_converter()) {
			auto* p = lst.get_block(b->m_conv_id);
			if (p->m_flags.marked) {
				p->m_flags.marked = false;
				arr.emplace_back(p);
			}
		}

		for (let& q : *b) {
			ai.reset3({ b, &b->m_ops[q.pos5].hdr });
			for (auto& x : ai) {
				if (x.h->tt != ETYPE::unk_reference)continue;
				auto* p = lst.get_block_from_unk(x.h->get_unk_id());

				if (p->m_flags.marked) {
					p->m_flags.marked = false;
					arr.emplace_back(p);
				}
			}
		}
	}

	//deleting building information
	for (auto& q : g_ps.m_build_data) {
		if (q.empty())continue;
		let* p = q.get_block().m_parent;
		if (p && p->m_flags.marked) {
			q.clear_bd();
		}
	
	}

	////////////////////////////////////////////////////////////////////////////
	//deleting search information
	for (let id : lst.m_blocks) {
		auto* e = lst.get_block(id);
		if (e->m_flags.marked && e->m_calc_data && e->m_calc_data->m_structure) {
			--e->m_calc_data->m_structure->second.num_ref;
		}
	}
	auto& fnd = finder::get();
	fnd.adjust_structure_hash();
	fnd.adjust_metric_hash();

	//finally, we delete the blocks themselves
	get_vb().remove_marked(lst);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


struct frect
{
	float x1, y1, x2, y2;
	float width() const { return x2 - x1; };
	float height() const { return y2 - y1; };

	void clear() { x1 = y1 = x2 = y2 = 0; };

	bool empty() const { return width() <= 0 || height() <= 0; };
};
enum class MouseAction
{
	zoom = 0,
	rotate = 1,//pitch-yaw
	pan = 2,
	//roll =3,
	num_act,
};


ims_static gl_helper g_gl;
ims_static bool g_toolbar_drag_in_progress = false;
ims_static std::vector<ListViewMode> g_windows_history;
ims_static ListViewMode g_last_shown_window = ListViewMode::NUM_WINDOWS;


//the resolution at which the next image will be built
ims_static size_t g_width = 0;
ims_static size_t g_height = 0;

//action by left mouse button
ims_static MouseAction g_mouse_action = MouseAction::zoom;



ims_static bool g_reset_window_width = false;

ims_static const char* s_pane_id = "###IMSWND";
////////////////////////////////////////////////////////////////////////////////


ims_static std::string g_status_text;

//image position
ims_static screen_rect g_image_pos;
ims_static Eigen::Vector2f g_scroll_pos;

ims_static const float g_toolbar_font_height = 32;
ims_static float g_toolbar_from_y = 0;
ims_static float g_toolbar_heigth = 0;
ims_static float g_toolbar_button_heigth = 0;




ims_static bool g_need_update_vis_cols = false;

ims_static float g_toolbar_from_x = 0;
ims_static float g_toolbar_width = 0;



#ifndef IMGUI_DISABLE_DEMO_WINDOWS
ims_static bool g_show_test_window = false;
#endif



//current screen area for drawing a set
//top does not include the menu, bottom does not include the status bar
//left does not include the sidebar
ims_static frect g_rect;



static constexpr std::array<window_state*, (size_t)ListViewMode::NUM_WINDOWS>
s_all_windows =
{
	&s_ui.m_ifs_list,
	&s_ui.m_finder,
	&s_ui.m_filter_columns,
	&s_ui.m_sel_visible,
	&s_ui.m_examples,
	&s_ui.m_creator,
	&s_ui.m_editor,
	&s_ui.m_console,
	&s_ui.m_location,
	&s_ui.m_palette,
	&s_ui.m_settings,
	&s_ui.m_render_par,
	&s_ui.m_set_selector,
	&s_ui.m_graph_list,
	&s_ui.m_animation,
	&s_ui.m_help,
	&s_ui.m_clipboard,
};



static void show_aux_dialog();
static void draw_base(const frect& rc);
static void switch_tool_windows();



static window_state* get_window_data(ListViewMode m)
{
	return s_all_windows[(size_t)m];
};


static void set_help_active_window() 
{
	s_ui.m_help.set_mode(ws_help::mode::other);

	let m = get_cur_mode();
	if (m != ListViewMode::HELP) {
		s_ui.m_help.m_help_id = get_window_data(m)->get_help_id();
		set_view_mode(ListViewMode::HELP);
	}
	
}

//always after set_block_ex, in the main thread
void ui_onload()
{
	assert(ims_worker::is_main_thread());
	for (auto& w : s_all_windows) {
		w->on_load();
	}

	//reload help
	if (get_cur_mode() == ListViewMode::HELP) {
		if (s_ui.m_help.m_mode2 == ws_help::mode::file) {
			set_help_file();
		}

		else if (s_ui.m_help.m_mode2 == ws_help::mode::block) {
			set_help_block();
		}

	}
}



static frect get_working_area()
{
	frect ret = g_rect;
	let& s = get_settings();
	let sz = s.is_docked() && g_show_pane ? s.m_docked_size : 0;

	switch (s.m_window_mode)
	{
	default:
		break;
	case window_mode_type::left:
		ret.x1 += sz;
		break;
	case window_mode_type::right:
		ret.x2 -= sz;
		break;
	case window_mode_type::up:
		ret.y1 += sz;
		break;
	case window_mode_type::down:
		ret.y2 -= sz;
		break;
	}
	return ret;
}

ListViewMode get_cur_mode()
{
	return g_windows_history.back();
}


void on_back_button_pressed()
{
	if(s_ui.close_modals()){
		return;
	}
	
	if (g_windows_history.size() <= 1) {
		switch_tool_windows();
		return;
	}

	g_windows_history.pop_back();

	set_view_mode(g_windows_history.back());
}


void StartSearch()
{
	assert(ims_worker::is_main_thread());
	set_view_mode(ListViewMode::LIST);

	finder::get().init_search_domain(ifs_list_get());

	get_thread(e_ims_threads::search).start([]()
	{
		set_thread_name("Search");

		void set_thread_low_priority();
		set_thread_low_priority();

		finder::find_set(
			finder::get(),
			ifs_list_get(),
			columns::get(),
			get_list_lock(),
			finder_cb);
	});

	
}

bool load_env_block_ex(bool fparams, bool rparams)
{
	env_block_data ebd;
	ebd.cols = fparams ? &columns::get() : nullptr;
	ebd.fparams = fparams ? &finder::get() : nullptr;
	ebd.rparams = rparams ? &get_rpars(): nullptr;

	auto* xb = ifs_list_get().find_block2(ims_keywords::search_params_block, "");
	if (!xb)return false;
	return load_env_block(xb, ebd);
};


void save_env_block(bool only_if_exists, bool fparams, bool rparams)
{

	std::unique_ptr<oper_block> nb;

	auto& lst = ifs_list_get();

	auto* xb = lst.find_block2(ims_keywords::search_params_block, "");

	if (!xb) {//does not exist
		if (only_if_exists) {
			return;
		}
		nb.reset(new oper_block);
		nb->m_flags.hidden = true;
	}else {
		xb->m_class.reset();
		xb->m_src2.reset();//important
		//don't touch the flags
	}

	auto& b = xb ? *xb : *nb;

	env_block_data ebd;
	ebd.cols = fparams ? &columns::get():nullptr;
	ebd.fparams = fparams ? &finder::get(): nullptr;
	ebd.rparams = rparams ? &get_rpars(): nullptr;

	set_env_block(lst.m_idf, b, ebd);
	ast_stack ai;
	ims_info::link_refs_for_block(ims_info_get(), &b, ai);

	if (!xb) {
		add_block2(nb, ims_keywords::search_params_block);
	}
};


bool is_shift_down()
{
	return ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
	
	//let* keys = SDL_GetKeyboardState(nullptr);
	//return keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
}

bool is_control_down()
{
	return ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);

	//let* keys = SDL_GetKeyboardState(nullptr);
	//return keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
}

static void do_open_file()
{
	//TODO: If you put try_open_file inside, it will crash
	//on Android and Emscripten, but only in Release
	redraw_gui(1);//if you don't do this, the dialog won't be drawn the first time

	try_open_file([]() {

		let name = get_fullpath_for_current_file();
		get_fds().open_file("Open", name, { "aifs", "js", "*" },
			[](let& fn)
			{
				open_file(fn, "", false, true, nullptr);
			});
	});
}


static void do_save_as()
{
	uint8_t flags = 0;
	if (get_vb().get_vis_checked() > 0) flags |= 1 << (uint8_t)save_type::checked;
	if (get_cur_block())				flags |= 1 << (uint8_t)save_type::single;
	if (!ifs_list_get().empty())		flags |= 1 << (uint8_t)save_type::all;

	if (flags == 0) {
		ims_show_message("There is no data to save");
		return;
	}

	//////////////////////////////////////////////////////////////////
	auto& sp = get_fds();

	sp.m_filename_single = create_filename_for_current_block();
	sp.m_filename_all = g_last_file_all;

	let name = get_settings().m_last_folder +
		(sp.m_save_type == save_type::single ?
			sp.m_filename_single : sp.m_filename_all);


	//////////////////////////////////////////////////////////////////
	redraw_gui(1);

	sp.save_file("Save As", name, flags, { "aifs", "js", "fts","flame" },
		[](let& fn)
	{
		size_t num_saved = 0;

		IMS_SCOPE(close_fds);

		let st = get_fds().m_save_type;

		auto& ofs = get_fds().m_fw.stream();

		ims_precision prec(ofs);	prec.template max<DefNumTypes::Real>();

		let ext = ims_file::get_extension(fn);

		let& vp = finder::get().m_var_par;
		if (ext == "fts") {
			save_fts(ofs);
			num_saved = 1;
		}
		else if (ext == "flame") {

			screen_disk<DefNumTypes::Real> sd;
			let* bd = get_global_bd();

			std::span<const oper_block*> vis;

			const oper_block* sr = nullptr;

			if (st == save_type::single) {
				if (!bd)return;
				sr = &bd->get_block();
				vis = { &sr, (& sr) + 1};
				sd = g_ps.get_first_thumb_elem()->m_pcam->m_sd;
			} else {
				vis = get_vb().m_vis_blocks;
			}

			

			num_saved = calc_flame(
				ofs,
				get_global_ei(),
				g_ps.m_ctx,
				g_ps.m_am,
				get_global_ac(), 
				st,
				vp,
				get_rpars().m_palette,
				get_rpars().m_background,
				sd,
				get_vb().m_vis_blocks);

		}
		else {//native format

			if (st == save_type::all) {
				save_env_block(true, 
					env_block_data::s_use_fparams, 
					env_block_data::s_use_rparams);
			}

			num_saved = save(ofs, get_build_mode(), st);

			if (get_fds().m_fw.accessed_by_name()) {
				get_samples().add_recent(fn);
			}

			set_last_file(fn, st == save_type::all, true);
		}

		//////////////////////////////////
		std::cout << "[" << ims_chrono::fmt_time_t_to_hmsdmY().buf << "] Saved " << num_saved
			<< " elements to " << fn << std::endl;
		
	});
}
#ifndef NDEBUG
static void do_save_x3d()
{
	redraw_gui(1);
	get_fds().select_folder("Save As X3D", "", [](let& fn)
		{
			ims_to_x3d(fn, ifs_list_get());
	return true;
		});
}
#endif


static bool from_image(float& x, float& y)
{
	y = float(g_height) - 1 - y;

	assert(x<float(g_width));
	assert(y<float(g_height));

	x -= g_scroll_pos[0];
	y -= g_scroll_pos[1];

	x += g_image_pos.c[0];
	y += g_image_pos.c[1];

	return true;
}

static bool to_image_ex(float& x, float& y)
{
	x -= g_image_pos.c[0];
	y -= g_image_pos.c[1];

	if (x < 0 || y < 0 ||
		x >= g_image_pos.s[0] || y >= g_image_pos.s[1]) {
		return false;
	};

	x += g_scroll_pos[0];
	y += g_scroll_pos[1];

	assert(x<float(g_width));
	assert(y<float(g_height));

	y = float(g_height) - 1 - y;

	return true;
};



static bool use_zoom_box()
{
	auto* bp = g_ps.get_first_thumb_elem();
	if (!bp || !bp->m_data3) {
		return false;
	}

	auto& sv = bp->m_data3->m_special;

	if (sv.m_si_empty) {
		return false;
	}
	
	
	let& z = g_zoom_box;

	let sw = z[1][0] - z[0][0];
	let sh = z[1][1] - z[0][1];

	if (sw <= 0 || sh <= 0) {
		return false;
	}

	auto cx = (float)floor((z[0][0] + z[1][0]) / 2);
	auto cy = (float)floor((z[0][1] + z[1][1]) / 2);

	if (!to_image_ex(cx, cy)) {
		return false;
	}


	let w = g_width;
	let h = g_height;

	

	let& si = sv.m_si2;
	let d = si.get_section_dim();
	if (d <= 2) {
		auto& sd = sv.m_xcam2.m_sd;

		
		//find the world coordinates of the current screen
		let pixel_size = sd.r * 2 / std::min(w, h);

		let k = pixel_size;

		let a = sd.a * boost::math::constants::pi<double>() / 180;
		let c = cos(a);
		let s = sin(a);
		Eigen::Matrix2d m;
		m << c, s, -s, c;

		Eigen::Vector2d v = { cx - w / 2, cy - h / 2 };
		v = m * v;

		sd.c[0] += v[0] * k;
		if (d == 2) {
			sd.c[1] += v[1] * k;
		}

		sd.r *= sw/w;
		return true;
	}

	//3D
	let qx = std::max(cx - sw / 2, 0.0f);
	let qy = std::max(cy - sh / 2, 0.0f);

	let iw = std::min(w - qx, sw);
	let ih = std::min(h - qy, sh);

	if (!bp->m_buf3d_di->zoom_in_cam(sv.m_xcam2.m_camera,
		qx / w, qy / h, iw / w, ih / h, 
		s_ui.m_location.m_lock_dist_target)) 
	{
		return false;
	}


	return true;

}


static bool use_mouse_wheel(float mx, float my, float mw)
{
	auto* bp = g_ps.get_first_thumb_elem();
	if (!bp || !bp->m_data3) {
		return false;
	}

	auto& sv = bp->m_data3->m_special;

	if (sv.m_si_empty) {
		return false;
	}
	
	auto cx = mx;
	auto cy = my;

	if (!to_image_ex(cx, cy)) {
		return false;
	}


	let scale = exp(mw / 10);

	let w = g_width;
	let h = g_height;

	let& si = sv.m_si2;
	let d = si.get_section_dim();
	if (d <= 2) {

		
		
		auto& sd = sv.m_xcam2.m_sd;

		//find the world coordinates of the current screen
		let pixel_size = sd.r * 2 / std::min(w, h);

		let k = ((scale - 1) * pixel_size);

		let a = sd.a * boost::math::constants::pi<double>() / 180;
		let c = cos(a);
		let s = sin(a);
		Eigen::Matrix2d m;
		m << c, s, -s, c;

		Eigen::Vector2d v = { cx - w / 2, cy - h / 2 };
		v = m * v;

		sd.c[0] += v[0] * k;
		if (d == 2) {
			sd.c[1] += v[1] * k;
		}

		sd.r /= scale;

		return true;
	}

	//3D


	;

	auto& c = sv.m_xcam2.m_camera;
	const Eigen::Vector3d  lr = c.m_loc - c.m_ref;
	if (s_ui.m_location.m_lock_dist_target) {
		let dlr = lr * (1 / scale - 1)* s_ui.m_location.get_zt();
		c.m_loc += dlr;
		c.m_ref += dlr;
	} else {
		c.m_loc = lr / scale + c.m_ref;
	}
	


	return true;
}


static void build_task(draw_task task)
{
	set_thread_name("Build Images");

	auto& rp = get_report_params();

	rp.max_complexity = finder::get().get_extra_complexity(columns::get());
	rp.max_bits = finder::get().m_max_bits;
	rp.find_prec2 = finder::get().m_find_prec;

	auto& rth = *ims_worker::get();

	g_ps.build_image(
		ifs_list_get().m_idf,
		update_bitmap,
		g_thumbnail,
		task,
		rp,
		g_thum_is_list,
		g_thum_enabled ? get_settings().m_max_thmb: 1,
		s_ui.m_location.m_force2D,
		get_rpars(),
		get_build_mode(),
		rth);


	if (!ims_need_stop() && g_ps.m_ts2 == 1) {
		try_autosave();
	}

	update_ui_async();
};



void do_build(const draw_task& t)
{
	draw_task task = t;//TODO: copying is probably not necessary

	assert(ims_worker::is_main_thread());

	if (get_thread(e_ims_threads::build).is_running()) {
		//TODO: Make sure this doesn't happen
		//It happens because while the message with this call was flying through
		//post_message, someone from the main thread start to build
		assert(false);
		SDL_Log("--Error--");
		return;
	}


	if (reset_resolution()) {
		//if the user has changed the window size, repaint is not enough - it will crash
		task.build_2d_ext = true;
		task.build_2d_std = true;
		task.build_3d = true;
	};


	task.use_low_res = true;

	//for example, in WASM, the developer console can completely block the workspace
	//and then, with zero values, the program will crash due to division by 0
	if (g_width == 0 || g_height == 0) {
		return;
	}
	g_thumbnail.set_res(g_width, g_height);

	let& vp = finder::get().m_var_par;


	if (!g_ps.on_start_build(
		g_thumbnail,
		g_thum_is_list,
		g_thum_enabled ? get_settings().m_max_thmb : 1,
		get_vb(),
		vp))
	{
		return;
	}

	//start the thread
	get_thread(e_ims_threads::build).start([task]() {build_task(task); });

};


void do_rebuild_sync()
{
	draw_task dt;
	dt.rebuild();
	do_build(dt);
}

//0-left, 1-right, 2-middle
static MouseAction get_action(int but)
{
	return MouseAction((int(g_mouse_action) + but) % int(MouseAction::num_act));
};

void get_working_res(size_t& w, size_t& h)
{
	w = g_width;
	h = g_height;
}

bool& need_update_vis_cols()
{
	return g_need_update_vis_cols;
}

void reset_window_width()
{
	g_reset_window_width = true;
}


static void do_show_about_dlg()
{
	s_ui.m_about.m_show = true;
	redraw_gui(1);
};

static void do_show_clipboard_dlg()
{
	s_ui.m_clipboard.m_show = true;
	redraw_gui(1);
};

static void do_clear_console()
{
	s_ui.m_console.clear_console();
};

static void switch_tool_windows()
{
	g_show_pane = !g_show_pane;
}
static void on_ctrl_tab()
{
	if (g_windows_history.size() < 2)return;
	auto& h = g_windows_history[g_windows_history.size() - 2];
	let new_mode = h;
	h = g_windows_history.back();
	g_windows_history.pop_back();

	set_view_mode(new_mode);
};


void set_view_mode(ListViewMode m)
{
	if (g_windows_history.empty()) {
		g_windows_history.emplace_back(m);
	}
	else if (get_cur_mode() != m) {
		g_windows_history.emplace_back(m);
	}

	if (m == ListViewMode::LIST || m == ListViewMode::Components) {
		if ((m == ListViewMode::LIST) != g_thum_is_list) {
			if (g_thum_enabled) {
				stop_build_then([]() {
					g_thum_is_list = !g_thum_is_list;
					do_fit_to_screen();
				});
			} else {
				g_thum_is_list = !g_thum_is_list;
			}
		}
	}

	//bool do_recalc = !g_show_pane;
	g_show_pane = true;

	//if (do_recalc) {
		//recalc_layout();
	//}

	redraw_gui(2);//the column visibility selection is not drawn at once
};

//always stops the search, but if the file has been changed, it may ask the user
void try_open_file(std::function<void()>&& F, bool use_confirm)
{
	assert(ims_worker::is_main_thread());

	if (get_thread(e_ims_threads::aux).is_running()) {
		//just do nothing because there's a modal window on the screen
		//and the user will understand everything
		return;
	}

	if (!use_confirm || !ims_info_get().m_need_save) {
		stop_search_then(std::move(F));
		return;
	}

	ims_confirm_dlg("There are unsaved changes, do you wish to discard them?",
		[F = std::move(F)]() mutable
		{
			ims_info_get().m_need_save = false;//the user clearly gave permission
			stop_search_then(std::move(F));
		});
};

bool is_search_started()
{
	return get_thread(e_ims_threads::search).is_running();
};

//the user decided to close the program
void do_exit_confirm()
{

	let busy = get_thread(e_ims_threads::aux).is_running() ||
		is_search_started() || is_batch_in_progress();

	if (!ims_info_get().m_need_save && !busy) {
		ims_worker::exit_program();
	}
	else {
		ims_confirm_dlg("Are you sure you want to exit?", []()
			{
				ims_worker::exit_program();
			});
	}
}

void do_build_mesh()
{
	if (get_thread(e_ims_threads::aux).is_running()) {
		return;
	}

	let* bp = g_ps.get_first_thumb_elem();
	if (!bp || !bp->m_data3)return;
	let& xd = *bp->m_data3;
	if (!xd.m_bi.exists())return;
	
	auto root = bp->m_root2;
	if (root == ims_max) {
		root = xd.get_froot();
	}

	if (root >= xd.m_bi.get_fg().num_ver()) {
		return;
	}

	redraw_gui(1);

	let& sv = xd.m_special;

	get_fds().save_file("Mesh", create_filepath_for_current_block(), 0, { "ply" },
		[&sv, &xd, root](let&)
		{
			get_thread(e_ims_threads::aux).start([&sv, &xd, root]() {

				bool ret = false;
				IMS_SCOPE([&ret]() {close_fds_ex(ret); });

				set_thread_name("Build Mesh");

				let& pal = sv.chas_builtin(builtin_ids::palette) ?
					sv.m_pal : get_rpars().m_palette;

				let& crz = sv.chas_builtin(builtin_ids::colorize) ?
					sv.m_colorize : get_rpars().m_colorize;

				ret = save_mesh(
					get_fds().m_fw.stream(),
					xd.m_bi,
					root,
					sv.m_si2,
					pal,
					crz);
			});

		});
};
void check_navi_ex(bool c)
{
	g_is_check_navi = c;
}

void update_num_checked_ex()
{
	get_vb().update_num_checked(ifs_list_get());
}


static void from_text(const std::string& text)
{
	auto& nfo = ims_info_get();
	auto& lst = ifs_list_get();
	bool was_empty = lst.empty();

	std::istringstream istr(text);
	auto nbeg = std::istreambuf_iterator<char>(istr);
	let nend = std::istreambuf_iterator<char>();

	let fn = get_fullpath_for_current_file();

	ims_err_reset();

	error_helper::use_buf ehu;

	bool is_ok = ims_load7(nfo, fn, nbeg, nend, true);

	if (!is_ok) {
		let err_msg = ehu.get_buf();
		std::cerr << err_msg;
		ims_show_message(err_msg);
	}

	if (was_empty) {
		init6(true);
		finder::get().init();
	}
	else {
		get_vb().reset_vis_blocks(lst);
		get_vb().update_num_checked(lst);
	}

	if (!is_ok) {//the error has already been displayed
		return;
	}
	nfo.m_need_save = true;

	let sz = get_vb().m_vis_blocks.size();

	if (sz > 0) {
		s_ui.m_ifs_list.scroll_to_row(sz - 1);
		set_block_ex(sz - 1);
		do_build(draw_task(true));
		set_view_mode(ListViewMode::LIST);
	};

	ui_onload();
};



//always start in the main thread
//if the file is already loading, we do nothing
//asynchronously wait for the build to stop
//start the file loading thread
//when the loading is complete, we switch to the main thread
//perform initialization

void open_file(
	const std::string& filename,
	std::string&& data,//content (may be empty)
	bool keep_set,
	bool to_recent, //add only if the file was actually opened
	const edit_info* edit)
{
	assert(ims_worker::is_main_thread());

	ims_err_reset();

	if (g_file_open_in_progress) {
		return;
	}
	g_file_open_in_progress = true;

	assert(!edit || filename.empty());

	if (!edit && ims_file::get_extension(filename) == "gpl") {
		IMS_SCOPE([&] {g_file_open_in_progress = false; });
		ims_reader reader;
		if (!reader.open(filename, data) ||
			!get_rpars().m_palette.load_gimp(reader.stream()))
		{
			ims_show_message("Invalid palette !");
		}
		return;
	}

	auto lfile = [filename, data = std::move(data), keep_set, to_recent, edit]() {
		assert(ims_worker::is_main_thread());

		//maybe the file name is the source text
		if (!edit && data.empty() && !ims_file::is_exists(filename)) {
			IMS_SCOPE([&] {g_file_open_in_progress = false; });

			//possibly base64
			let dec = try_decode_base64(filename);
			if (!dec.empty()) {
				from_text(dec);
				return;
			}

			if (filename.find_first_of('@') != filename.npos) {
				from_text(filename);
				return;
			}

			get_samples().remove_recent(filename);
			ims_show_message(std::string("Could not open file ") + filename);

			return;
		}

		////////////////////////////////////////////////////////////////////////

		const oper_block* keep = nullptr;//who are we trying to save?

		if (keep_set || edit) {
			if (edit) {
				keep = edit->b;
			}
			if (!keep) {
				keep = get_cur_block();
			}
		}
		
		//full reset, needs to be removed so the GUI doesn't get into undesirable places
		if (!edit) {

			set_last_file(filename, true, to_recent);//even in case of an error
			if (to_recent && data.empty()) {
				//WASM: if data!=empty then the file may not exist in the file system
				get_samples().add_recent(filename);
			}
			save_settings(get_ini_filename());

			do_clear_console();

			g_boundary_mode = ifs_object_type::normal;

			get_vb().m_vis_blocks.clear();

			////////////////////////////////////////////////////////////////////////
			//clear the image on the screen
			g_ps.clear_draw_image();
			g_ps.init7();
			g_gl.clear_tmp_bmp();

			
		}

		auto lworker = [filename, data = std::move(data), keep, edit]() {


			assert(!ims_worker::is_main_thread());

			set_thread_name("Loading");
			ims_worker::get()->m_stage_name = "Loading...";

			ims_reader reader;
			if (!edit) {
				if (!reader.open(filename, data)) {
					ims_error("Could not open file: {}", filename);
					g_file_open_in_progress = false;
					return;
				}
			}

			std::string block_name;
			std::string block_id;
			size_t op_id = ims_max;

			if (keep) {
				block_name = keep->m_name;
				block_id = keep->str_id4();
				let root_ref = keep->get_active_ref();
				if (root_ref != ims_max) {
					op_id = keep->get_class()->m_refs[root_ref].unk_id;
				}
			}

			const oper_block* keep_new = nullptr;

			std::unique_ptr<ims_info> pnfo;

			error_helper::use_buf ehu;

			bool load_ok;


			if (edit) {
				load_ok = ims_apply_source(ims_info_get(), pnfo, &keep_new,
					edit->src, edit->js_mode ? keep : edit->b, edit->js_mode);
			} else {
				clock_print cp(filename);

				auto iter = std::istreambuf_iterator<char>(reader.stream());
				let end = std::istreambuf_iterator<char>();

				if (ims_file::get_extension(filename) == "png") {
					ims_png::find_aifs(iter);
				};


				clear_before_load();

				finder::get().init();


				pnfo.reset(new ims_info);
				load_ok = ims_load7(*pnfo, filename, iter, end, false);

				if (load_ok) {
					keep_new = pnfo->m_list.find_block2(block_id, block_name);
					std::swap(g_ims_info, pnfo);
				}

				pnfo.reset();//indicator that it does not "apply"

				init6(false);//just loaded from file, no need to save

				if (ims_need_stop()) {
					g_file_open_in_progress = false;
					return;
				}
			}

			std::string err_msg;
			if (!load_ok) {
				err_msg = ehu.get_buf();		
				assert(!err_msg.empty());
			}

			if (load_ok && keep_new && op_id != ims_max) {
				let* g = keep_new->get_class();
				let it = g->m_unk2var.find(op_id);
				if (it != g->m_unk2var.end()) {
					const_cast<oper_block*>(keep_new)->set_active_ref(it->second);
				};
			}

			auto lmain = [load_ok, err_msg, keep_new, pnfo = std::move(pnfo)]() mutable
			{
				assert(ims_worker::is_main_thread());
				g_file_open_in_progress = false;


				if (!load_ok) {
					std::cerr << err_msg;
					ims_show_message(err_msg);
					update_ui_async();
					return;
				}

				
				if (pnfo) {//Apply
					//successful loading, replace

					std::swap(pnfo, g_ims_info);
					pnfo.reset();//delete the old one immediately
					
					
					init6(true);//loaded from memory, needs to be saved

					finder::get().init();
				}

				if (load_env_block_ex(true, true)) {
					//TODO: duplicate call, the first one was already in init6
					get_vb().reset_vis_blocks(ifs_list_get());
				};

				////////////////////////////////////////////////////////////////
				size_t start_idx = ims_max;

				if (keep_new) {
					start_idx = get_vb().find_vis_block(keep_new);
				}else {
					start_idx = get_default_block();
					
					if (get_vb().m_vis_blocks.empty()) {
						set_view_mode(ListViewMode::CONSOLE);
					}else {
						if (start_idx != ims_max) {
							keep_new = get_vb().m_vis_blocks[start_idx];
						}
						set_view_mode(ListViewMode::LIST);
					}					
				}

				get_vb().m_cur_block_pos = start_idx;//can be ims_max

				if (keep_new) {
					let& vp = finder::get().m_var_par;
					g_ps.set_block3(keep_new, vp);
					do_build(draw_task(true));
				}

				ui_onload();
			};

			main_thread::post(lmain);
		};

		get_thread(e_ims_threads::aux).start(lworker);
	};

	stop_build_then(lfile, 0);
};



#ifdef __EMSCRIPTEN__
std::string Emscripten_paste_buffer;
#endif

bool ims_from_clipboard(std::string& dst)
{	
	assert(ims_worker::is_main_thread());
	dst.clear();
	
#ifdef __EMSCRIPTEN__
	if (Emscripten_paste_buffer.empty()) {
		//will eventually call string_open
		void Emscripten_switch_to_editor();
		Emscripten_switch_to_editor();		
		return false;
	}
	
	dst = Emscripten_paste_buffer;
	Emscripten_paste_buffer.clear();//so as not to confuse the user
	return true;

#else
	auto* text = SDL_GetClipboardText();
	if (!text)return false;
	dst = text;
	SDL_free(text);
#endif
	return true;
};


static void append_aifs_as_text(const std::string& str)
{
	if (str.empty()) {
		ims_show_message("Error: empty text");
		return;
	}
	std::string dec = try_decode_base64(str);
	if (dec.empty()) {
		dec = str;
	}

	stop_build_then([dec]() {
		from_text(dec);
	});
};

bool do_from_clipboard()
{
	std::string str;
	if (!ims_from_clipboard(str)) {
		return false;//The web version will show a text box
	};

	boost::algorithm::trim(str);

	if (str.empty()) {
		ims_show_message("Error: empty text");
		return false;
	}

	//the file remains, we just add it to the end
	stop_search_then([str]() {
		append_aifs_as_text(str);
	});

	return true;
};


static void do_reload_file()
{
	if (g_last_file_all.empty()) {
		return;
	}

	try_open_file([]() {
		open_file(get_fullpath_for_current_file(),
		"", true, false, nullptr);
		});
};


static void get_resolution(size_t& nw, size_t& nh) 
{
	if (get_rpars().m_use_window_res) {
		let rc = get_working_area();
		nw = (size_t)std::max(0.0f, rc.width());
		nh = (size_t)std::max(0.0f, rc.height());
	} else {
		nw = (size_t)get_rpars().m_resolution[0];
		nh = (size_t)get_rpars().m_resolution[1];
	}
}

static bool reset_resolution()
{
	//the only place where we change g_width, g_height
	assert(!get_thread(e_ims_threads::build).is_running());


	//new image size
	size_t nw, nh;
	get_resolution(nw, nh);

	bool clear = false;


	if (nw > 0 && nh > 0 && (nw != g_width || nh != g_height)) {
		clear = true;
		g_width = nw;
		g_height = nh;
	}

	if (clear) {
		g_ps.clear_draw_image();
		g_gl.clear_tmp_bmp();
	}

	return clear;
};

void do_save_palette(const palette& p)
{
	std::string name = p.name;
	if (name.empty()) {
		name = "palette";
	};

	let fn = create_filepath_for_current_block();
	redraw_gui(1);
	get_fds().save_file("Palette", fn, 0, { "gpl" },
		[&p](let&)
		{
			IMS_SCOPE(close_fds);
			p.save_gimp(get_fds().m_fw.stream());
		});
};

void do_load_palette(palette& p)
{
	std::string name = p.name;
	if (name.empty()) {
		name = "palette";
	};

	let fn = create_filepath_for_current_block();
	redraw_gui(1);
	get_fds().open_file("Palette", fn, { "gpl" },
		[&p](let& fn)
		{
			std::ifstream fs;
			ims_file::open(fs, fn);
			p.load_gimp(fs);
		});
};


void do_save_exr()
{
	if (!get_global_bd())return;//protection


	let& be = *g_ps.get_first_thumb_elem()->m_buf_ext_di;

	if (get_thread(e_ims_threads::build).is_running() || g_ps.m_ts2 != 1 || be.m_img.empty()) {
		ims_show_message("Image is not ready!\n");
		return;
	}

	let fn = create_filepath_for_current_block();
	redraw_gui(1);

	get_fds().save_file("Save As EXR", fn, 0, { "exr" },
		[&be](let&)
		{
			bool ret = false;
	IMS_SCOPE([&ret]() {close_fds_ex(ret); });

	let w = be.m_img.w();
	let h = be.m_img.h();
	let sz = w * h;
	std::vector<float> img(sz);

	for (size_t i = 0; i < sz; ++i) {
		double x;
		if (!be.get_pixel_raw(x, i, 0)) {
			x = std::numeric_limits<float>::infinity();
		}
		img[i] = (float)x;
	}

	exr_data ex;
	ex.init(w, h, 1);
	ex.set_channel(0, img.data(), "Y");

	ret = ex.save(get_fds().m_fw.stream());
	if (!ret) {
		ims_show_message(std::string("Could not save EXR"));
	}

		});
};

void ims_show_message(std::string_view msg)
{
	s_ui.m_modal_msg.message(msg);
}

void ims_confirm_dlg(std::string_view msg, std::function<void()>&& F, bool show)
{
	if (!show) {
		F();
	} else {
		s_ui.m_modal_msg.confirm(msg, std::move(F));
	}
}


static void do_save_picture()
{
	if (g_width == 0) {
		ims_show_message("Image is not ready!\n");
		return;
	};

	let fn = get_settings().m_last_picture_folder.empty() ?
		create_filepath_for_current_block() :
		get_settings().m_last_picture_folder + create_filename_for_current_block();

	redraw_gui(1);
	get_fds().save_file("Save Image", fn, 0, { "png" }, [](let&) {

		bool ret = false;
		IMS_SCOPE([&ret]() {close_fds_ex(ret); });

		{
			std::scoped_lock lock(g_ps.m_lock_draw);
			ret = g_ps.save_png(
				ims_info_get(), 
				[](const void* p, size_t sz) {
					get_fds().m_fw.stream().write((const char*)p, sz); 
				}, get_rpars(), true);
			get_settings().m_last_picture_folder = ims_file::get_parent(get_fds().get_full_name());
		}

		if (!ret) {
			ims_show_message(std::string("Could not save picture"));
		}
	});

};


static bool upload_img()
{
	std::scoped_lock lock(g_ps.m_lock_draw);
	g_ps.m_img_draw_uploaded = true;
	auto& img = g_ps.get_img_draw();
	if (!img.empty() && g_width > 0 && g_height > 0) {
		if (!g_gl.prepare_screen_bitmap(img, g_width, g_height)) {
			return false;
		}
		g_gl.upload_screen_bitmap();
		return true;
	}
	return false;
}


static bool is_thumb_sel(int but)
{
	return g_right_select && but == 0 || !g_right_select && but == 1;
}


static void do_export_csv()
{
	let fn = create_filepath_for_current_block();
	redraw_gui(1);
	get_fds().save_file("Export CSV", fn, 0, { "csv" }, export_csv);
};


struct render_context 
{
	screen_area  scr;
	std::string dir;
	report_params rp;
	ifs_object_type mode;
	draw_task task;
	variator_params vp;
	render_params ren_par;
	bool force2d;

	void render() const
	{
		program_state cur_ps;
		cur_ps.init7();

		cur_ps.m_build_data.resize(1);

		auto& rth = *ims_worker::get();

		for (size_t i = 0;; ++i) {

			if (ims_need_stop()) {
				break;
			}

			let* sr = get_vb().get_vis(i);
			if (!sr)break;
			if (!sr->m_flags.checked)continue;

			auto fn(sr->m_name);//copy
			if (fn.empty()) {
				fn = std::to_string(sr->m_block_id);
			}


			FILE* f = ims_file::open_exclusive_write(dir + fn + ".png");
			if (!f)continue;
			IMS_SCOPE([&] {	fclose(f); });

			////////////////////////////////////////////////////////////

			cur_ps.set_block3(sr, vp);

			//TODO: rework logic (build_image may change)
			auto scr_loc = scr;

			cur_ps.build_image(
				ifs_list_get().m_idf,
				update_ui_async,
				scr_loc,
				task,
				rp,
				true,//doesn't matter
				1,
				force2d,
				ren_par,
				mode,
				rth);


			if (ims_need_stop()) {
				break;
			}

			////////////////////////////////////////////////////////////////////
			//we save the text of the set itself, not the boundary
			cur_ps.save_png(
				ims_info_get(),
				[f](const void* p, size_t sz) {fwrite(p, 1, sz, f); },
				get_rpars(),
				false);
		};

	};
};

ims_static render_context g_rc;
ims_static std::atomic<uint32_t> g_rc_num { 0 };

bool is_batch_in_progress() 
{
	return g_rc_num > 0;
};

void do_batch_rendering()
{
	assert(!is_batch_in_progress());

	if (get_vb().get_vis_checked() == 0) {
		ims_show_message("Check items before running this action.\n");
		return;
	}

	size_t width, height;
	get_resolution(width, height);
	if (width == 0 || height == 0) {
		return;
	}

	auto& rc = g_rc;

	rc.scr = g_thumbnail;
	rc.scr.set_res(width, height);
	rc.scr.initX(1, 1);//disable thumbnails

	rc.dir = get_settings().m_last_picture_folder;
	platform::create_directory(rc.dir.c_str());

	let& fnd = finder::get();

	rc.rp = get_report_params();
	rc.rp.max_complexity = fnd.get_extra_complexity(columns::get());
	rc.rp.max_bits = fnd.m_max_bits;
	rc.rp.find_prec2 = fnd.m_find_prec;

	rc.force2d = s_ui.m_location.m_force2D;
	rc.mode = get_build_mode();

	rc.task.set_all(true);
	rc.task.use_low_res = false;

	rc.vp = fnd.m_var_par;

	rc.ren_par = get_rpars();

	let nt = get_settings().m_num_render_threads;

	let thread_start_idx = (size_t)e_ims_threads::num;
	ims_worker::create_threads(thread_start_idx + nt);

	for (size_t i = 0; i < nt; ++i) {
		auto& rth = ims_worker::get_thread(i + thread_start_idx);
		rth.start([&rc]() {
			++g_rc_num;
			rc.render();
			--g_rc_num;
		});
	}
};

static std::string pad_left(const std::string& str, size_t width, char c) {
	if (str.length() < width) {
		return std::string(width - str.length(), c) + str;
	}
	return str; // No padding needed
};

void do_create_anim(size_t ref_time)
{
	std::vector<animator::keyframe> k;

	eval_context* common_graph = nullptr;

	ovr_data opod;

	for (size_t i = 0;; ++i) {
		let* sr = get_vb().get_vis(i);
		if (!sr)break;
		if (!sr->m_flags.checked)continue;

		check_block(sr);

		auto& kf = k.emplace_back();

		if (!common_graph) {
			common_graph = sr->ctx();
		} else if (common_graph != sr->ctx()) {
			ims_show_message("Keyframes must have the same graph");
			return;
		}
		opod.init(*sr, true);
		opod.merge_from(kf.b, *sr);
	}

	if (k.size() < 2) {
		ims_show_message("There must be at least 2 keyframes");
		return;
	};


	bool error_in_console = false;
	IMS_SCOPE([&] {
		if (error_in_console) {
			ims_show_message("There are errors in the animation, see console");
			set_view_mode(ListViewMode::CONSOLE);
		}
	});

	////////////////////////////////////////////////////////////////////////
	//init keyframes
	let& vp = finder::get().m_var_par;

	build_data bd;

	for (auto& kf : k) {
		bd.clear_bd();

		auto& ei = get_global_ei();
		auto& ac = get_global_ac();
		auto& ec = g_ps.m_ctx;
		auto& am = g_ps.m_am;

		bd.pre_init(&kf.b, vp);

		if (!bd.init_normal_block(ei, ec, am, ac)) {
			ims_error("Block {}: evaluation error", kf.b.m_name);
			error_in_console = true;
			return;
		}

		kf.sv = bd.m_special;
	}

	////////////////////////////////////////////////////////////////////////
	animator anim;
	if (!anim.anim_init(k, ref_time)) {
		error_in_console = true;
		return;
	}

	//works quickly, we do it in the main thread
	let nf = s_ui.m_animation.m_num_frames;
	let suffix_length = std::to_string(nf).length();
	for (size_t i = 0; i <= nf; ++i) {
		auto nb = std::make_unique<oper_block>();
		anim.interpolate(*nb, double(i) / nf);

		nb->m_name = s_ui.m_animation.m_anim_prefix +
			pad_left(std::to_string(i), suffix_length, '0');
		
		add_block2(nb, "");
	}

	set_view_mode(ListViewMode::LIST);
};





static void show_helper(ListViewMode m)
{
	if (g_show_pane && get_cur_mode() == m) {
		g_show_pane = false;
	}
	else {
		set_view_mode(m);
	};
};



//the auxiliary thread is running, we show the modal window
static void show_aux_dialog()
{
	auto& ct = get_thread(e_ims_threads::aux);

	let* tt = ct.m_stage_name.empty() ? "Computing" : ct.m_stage_name.c_str();

	std::array<char, 32> buf = { 0 };
	fmt::format_to_n(buf.data(), buf.size(), "{}###Compute\0", tt);

	std::array<char, 32> text = { 0 };
	let w = ct.work_done();
	if (w <= 1) {
		fmt::format_to_n(text.data(), text.size(), "{:5.2f}% Completed\0", w * 100);
	}else {
		fmt::format_to_n(text.data(), text.size(), "{} Completed\0", (int)w);
	}

	bool b_cancel = false;
	ws_modal_msg::show_ext(buf.data(), text.data(), nullptr, &b_cancel);
	if (b_cancel) {
		//do not call CloseCurrentPopup(), we need to wait for the thread to complete
		ct.stop();
		redraw_gui(1);
	}

}

static bool has_modal()
{
	return
		get_thread(e_ims_threads::aux).is_running() ||
		s_ui.m_modal_msg.need_to_show();
}
//arguments - the position of the mouse cursor and whether the button 0,1,2 is pressed
static void check_thumb_input(float mx, float my, const bool* mouse_clicked) 
{
	//the block the mouse is over
	size_t hov_block = ims_max;

	auto ix = mx;
	auto iy = my;

	if (!to_image_ex(ix, iy)) {
		return;
	}

	hov_block = g_thumbnail.get_idx(size_t(ix), size_t(iy));

	if (is_thumb_comp()) {
		if (hov_block < g_ps.m_subsets_arr.size()) {


			auto* b = get_global_block();
			if (!b)return;

			let* g = b->get_class();

			let v = g_ps.m_subsets_arr[hov_block];
			if (v < g->m_refs.size()) {
				g_status_text += std::string(" ");
				g_status_text += g->get_var_name(v);
				

				int but = g_right_select ? 1 : 0;

				if (mouse_clicked[but]) {

					stop_build_then([v, &b]() {
						g_thum_enabled = false;
						b->set_active_ref(v);
						do_fit_to_screen();
					});
				}
			}
		}
	}else if (is_thumb_list()) {
		let block_idx = get_vb().m_cur_block_pos + hov_block;
		oper_block* sr = get_vb().get_vis(block_idx);
		if (sr) {
			g_status_text += " " + sr->m_name;

			let lb = mouse_clicked[0];
			let rb = mouse_clicked[1];

			if (lb || rb) {

				int but = lb ? 0 : 1;

				if (is_thumb_sel(but)) {
					get_vb().set_checked(sr, !sr->m_flags.checked);
				}
				else {
					stop_build_then([block_idx]() {

						g_thum_enabled = false;

						set_block_ex(block_idx);
						s_ui.m_ifs_list.scroll_to_row(block_idx);
						do_rebuild_sync();
					});
				}
			}
		}
	}

}

//arguments - the position of the mouse cursor and whether the button 0,1,2 is pressed
static void check_base_input(float mx, float my, const bool* mouse_clicked)
{
	let* bp = g_ps.get_first_thumb_elem();

	if (!bp || !bp->m_data3) return;
	let& sv = bp->m_data3->m_special;

	auto ix = mx;
	auto iy = my;

	if (!to_image_ex(ix, iy))return;

	let w = g_width;
	let h = g_height;

	let& si = sv.m_si2;
	let& xc = sv.m_xcam2;

	let rd = si.get_section_dim();

	if (rd <= 2) {
		if (!xc.empty(2)) {
			let& sd = xc.m_sd;
			let pixel_size = sd.r * 2 / std::min(w, h);
			let hx = pixel_size * w / 2;
			let hy = pixel_size * h / 2;

			let a = sd.a * boost::math::constants::pi<double>() / 180;
			let c = cos(a);
			let s = sin(a);
			Eigen::Matrix2d m;
			m << c, s, -s, c;

			Eigen::Vector2d v = { pixel_size * ix - hx, pixel_size * iy - hy };
			v = m * v;

			////////////////////////////////////

			if (get_cur_mode() == ListViewMode::LOCATION) {
				Eigen::Vector3d p;
				p[0] = sd.c[0] + v[0];
				p[1] = sd.c[1] + v[1];
				p[2] = bp->m_square;
				s_ui.m_location.from_mouse(xc, g_status_text, p, 
					mouse_clicked[0], true);
			}

			
		}
	}
	else {
		if (!xc.empty(3) && bp->m_buf3d_di) {

			//this object may change in another thread
			//let's hope not much...
			let& da = bp->m_buf3d_di->m_img;

			let sx = (size_t)(ix * float(da.w()) / w);
			let sy = (size_t)(iy * float(da.h()) / h);

			if (sx < da.w() && sy < da.h()) {

				let z = da(sx, sy).z;
				if (z>0){
					let& c = xc.m_camera;

					cam_proj< DefNumTypes::Real> proj;
					proj.init(c, da.w(), da.h());

					let dir = proj.back_proj_dir(c, sx, sy);

					const Eigen::Vector3d drl = dir * z / dir.norm();
					const Eigen::Vector3d p = c.m_loc + drl;

					if (get_cur_mode() == ListViewMode::LOCATION) {
						s_ui.m_location.from_mouse(xc, g_status_text, p, 
							mouse_clicked[0], false);
					}
					
				

				}
			}
		}
	};

	//print the pixel color - TODO: we need to somehow provide m_lock_draw
	let& img = g_ps.get_img_draw();
	auto iix = size_t(ix);
	auto iiy = size_t(iy);

	if (g_ps.m_ts2 > 0) {
		iix /= g_ps.m_ts2;
		iiy /= g_ps.m_ts2;
	}

	if (iix < img.w() && iiy < img.h()) {
		const ims_rgba c = img(iix, iiy);//copy
		fmt::format_to(std::back_inserter(g_status_text), 
			" c={},{},{},{}", c.r, c.g, c.b, c.a);
	}

	if (mouse_clicked[0]) {
		std::cout << g_status_text << std::endl;
	}

};


//cx,cy - where did user start drag from
//mx,my - drag offset
static bool check_mouse_dragging(
	build_data* bd,
	float cx, float cy,
	float mx, float my,
	MouseAction act)
{
	if (act == MouseAction::zoom) {

		auto ax = mx;
		auto ay = my;

		//the ratio should be the same as the picture
		let ly = abs(g_width * ay);
		let lx = abs(g_height * ax);
		if (ly > lx) {
			ax = ly / g_height;
			if (mx < 0)ax *= -1;
		}
		else {
			ay = lx / g_width;
			if (my < 0)ay *= -1;
		}

		if (get_settings().m_select_fom_corner) {
			if (ax > 0) {
				g_zoom_box[0][0] = cx;
				g_zoom_box[1][0] = cx + ax;
			}
			else {
				g_zoom_box[0][0] = cx + ax;
				g_zoom_box[1][0] = cx;
			}

			if (ay > 0) {
				g_zoom_box[0][1] = cy;
				g_zoom_box[1][1] = cy + ay;
			}
			else {
				g_zoom_box[0][1] = cy + ay;
				g_zoom_box[1][1] = cy;
			}
		}
		else {
			g_zoom_box[0] = { cx - abs(ax), cy - abs(ay) };
			g_zoom_box[1] = { cx + abs(ax), cy + abs(ay) };
		}




		g_zoom_box_visible = true;

		return false;
	}

	auto& sv = bd->m_special;

	auto& si = sv.m_si2;
	auto& xc = sv.m_xcam2;

	let dim = si.get_section_dim();

	//std::cout << mx << " " << my << std::endl;
#if 0
	if (act == MouseAction::roll) {
		//let fw = float(g_image_pos.width());
		let fh = float(g_image_pos.height());
		double scale =  exp(my / fh);

		if (dim <= 2) {

			stop_build_then([&xc, scale]() {
				auto& sd = xc.m_sd;
				sd.r /= scale;
				do_rebuild_sync();
			});

			return true;
		} else {
			stop_build_then([&xc, scale]() {
				auto& c = xc.m_camera;
				let lr = c.m_loc - c.m_ref;
				c.m_loc = lr / scale + c.m_ref;
				do_rebuild_sync();
			});

		
		}

		
	} else  
#endif
	if (act == MouseAction::rotate) {

		let fw = float(g_image_pos.width());
		let fh = float(g_image_pos.height());


		if (dim == 3) {

			float dx = mx / fw/2;
			float dy = my / fh/2;

			stop_build_then([dx, dy, &xc]() {
				auto& c = xc.m_camera;
				c.rotate(dy, -dx, s_ui.m_location.m_lock_dist_target);
				c.init();
				do_rebuild_sync();
			});

			return true;
		}

		if (dim == 2) {

			std::complex<double> m0(
				cx - g_image_pos.c[0] - fw / 2,
				cy - g_image_pos.c[1] - fh / 2);

			std::complex<double> m1(m0.real() + mx, m0.imag() + my);

			let dot = m0.real() * m1.imag() - m0.imag() * m1.real();
			let nr = std::abs(m0) * std::abs(m1);

			constexpr double mul = 180 / boost::math::constants::pi<double>();

			let da = -dot / nr * mul;

			stop_build_then([da, &xc]()
			{
				xc.m_sd.a += da;
				do_rebuild_sync();
			});

			return true;
		}

	}

	else  if (act == MouseAction::pan) {
		if (dim == 3) {

			stop_build_then([mx, my, &xc]() {
				auto& c = xc.m_camera;

				using Vec3 = camera<double>::Vec3;

				cam_proj<double> proj;

				proj.init(c, g_width, g_height);

				let d = proj.m_prj[2];

				Vec3 lr = c.m_loc - c.m_ref;
				Vec3 rdir = lr.cross(c.m_ver);

				let vv = (rdir * mx + c.m_ver * my * lr.norm()) * 
					s_ui.m_location.get_zt()/ d;

				c.m_loc += vv;
				c.m_ref += vv;

				c.init();
				do_rebuild_sync();

			});

			return true;
		}

		if (dim <= 2) {

			let ps = xc.m_sd.get_ps(g_width, g_height);

			Eigen::Vector2d d;

			d[0] = -mx * ps;
			d[1] = dim < 2 ? 0 : my * ps;

			let a = xc.m_sd.a * boost::math::constants::pi<double>() / 180;
			let c = cos(a);
			let s = sin(a);
			Eigen::Matrix<double, 2, 2> m;
			m << c, s, -s, c;

			if (dim == 2) {
				d = m * d;
			}

			stop_build_then([d, &xc]() {
				auto& c = xc.m_sd.c;
				c[0] += d[0];
				c[1] += d[1];
				do_rebuild_sync();
			});

			return true;
		}

	}
	return false;
}





//before on_start
void init_resolution(int w, int h, float scale)
{
	(void)h;
	//from the design of the widest sidebar window, it follows that its
	//minimum width = 320 * scale
	//assuming we've expanded the program to full screen, we get that
	//this should be no more than half the screen
	//because the other half is for the image

	get_ui_scale() = scale;

	get_settings().m_window_mode =
		640.0f * scale <= float(w) ?
		window_mode_type::left : window_mode_type::full;
}

static void load_default_palette() 
{
	std::ifstream fs;
	let fname = get_palette_filename();
	ims_file::open(fs, fname);

	auto& p = get_rpars().m_palette;
	if (fs.fail() || !p.load_gimp(fs) || p.data.size() == 0) {
		ims_warning("Failed to load palette from: {}", fname);
		p.reset();
	};
};

static void save_default_palette()
{
	std::ofstream fs;
	ims_file::open(fs, get_palette_filename());
	if (fs.fail()) return;
	get_rpars().m_palette.save_gimp(fs);
};


void on_start()
{
	columns::get().init_columns();

	constexpr auto prec = std::numeric_limits<DefNumTypes::Real>::digits10 + 1;
	std::cout << std::setprecision(prec);

	get_settings().m_last_folder = platform::getPathHome();
	get_settings().m_last_picture_folder = platform::getPathHome();

	get_rpars().reset_render_params();

	load_default_palette();

	////////////////////////////////////////////////////////////////////////////

	load_settings(get_ini_filename());

	set_ui_scale(get_ui_scale());

	g_gl.init();

	init_fonts();

	set_dark_theme(get_settings().m_dark);

	ims_worker::create_threads((size_t)e_ims_threads::num);

	//at the very end of loading
	for (auto& w : s_all_windows) {
		w->on_create();
	}

	set_view_mode(ListViewMode::EXAMPLES);
}

void save_settings() 
{
	save_default_palette();
	save_settings(get_ini_filename());
	ims_write_sync();
}

void on_exit()
{
	ims_worker::destroy_threads();
	g_gl.deinit();
};

////////////////////////////////////////////////////////////////////////////////

void ims_on_key_down(const SDL_KeyboardEvent& ekey)
{
	const bool is_shift = ekey.mod & SDL_KMOD_SHIFT;
	const bool is_cntrl = ekey.mod & SDL_KMOD_CTRL;

	switch (ekey.scancode) {
	case SDL_SCANCODE_HOME:
		s_ui.m_ifs_list.scroll_to_row(0);
		break;
	case SDL_SCANCODE_END:
	{
		let n = get_vb().m_vis_blocks.size();
		if (n > 0)s_ui.m_ifs_list.scroll_to_row(n - 1);
	}
	break;
	case SDL_SCANCODE_PAGEUP:
		if (g_is_check_navi)change_block(-s_ui.m_ifs_list.m_height_in_rows, false);
		break;
	case SDL_SCANCODE_PAGEDOWN:
		if (g_is_check_navi)change_block(s_ui.m_ifs_list.m_height_in_rows, false);
		break;
	case SDL_SCANCODE_UP:
		if (!get_fds().empty3()) {
			get_fds().scroll_list(true);
		} else {
			if (g_is_check_navi)change_block(-1, is_shift);
		}
		break;
	case SDL_SCANCODE_DOWN:
		if (!get_fds().empty3()) {
			get_fds().scroll_list(false);
		} else {
			if (g_is_check_navi)change_block(1, is_shift);
		}
		break;
	case SDL_SCANCODE_LEFT:
		if (g_is_check_navi)change_set(-1, true);
		break;
	case SDL_SCANCODE_RIGHT:
		if (g_is_check_navi)change_set(1, true);
		break;
	case SDL_SCANCODE_S:
		if (is_cntrl) {
			if (get_fds().empty3())do_save_as();
		}
		break;
	case SDL_SCANCODE_U:
		if (is_cntrl) {
			rand_current_set();
		}
		break;
	case SDL_SCANCODE_O:
		if (is_cntrl) {
			if (get_fds().empty3())do_open_file();
		}
		break;
	case SDL_SCANCODE_TAB:
		if (is_cntrl) {
			on_ctrl_tab();
		}
		break;
	case SDL_SCANCODE_F7:
		//do_rebuild();
		break;
#if !defined(__EMSCRIPTEN__)
		//works incorrectly when returning from Fullscreen, so we handle it in JS
		//https://github.com/emscripten-ports/SDL2/issues/128#issuecomment-851759160
	case SDL_SCANCODE_F11:
		switch_fullscreen();
		break;
	case SDL_SCANCODE_F1:
		if (get_cur_mode() == ListViewMode::HELP) {
			on_back_button_pressed();
		} else {
			if (!set_help_file()) {
				set_help_active_window();
			}
			set_view_mode(ListViewMode::HELP);
		}
		break;
#endif
	case SDL_SCANCODE_ESCAPE:
		s_ui.close_modals();
		break;
	case SDL_SCANCODE_R:
		if (is_cntrl) {
			do_reload_file();
		}
		break;
	case SDL_SCANCODE_EQUALS:
		if (is_cntrl) {
			auto* xd = get_global_bd();
			if (xd) {
				let* bb = get_cur_block();
				if (!bb || !xd->can_create_view()) {
					ims_show_message("Unable to create set - block does not exist or has been modified.");
				} else {
					add_view_to_list(xd, bb);
				}
			}
		}
		break;
	case SDL_SCANCODE_F4:
		if (is_shift) {
			switch_tool_windows();
		}
		break;
	case SDL_SCANCODE_AC_BACK:
		on_back_button_pressed();
		break;
	default:
		break;
	}

};



static bool menu_item_mode(ListViewMode m)
{
	bool sel = get_cur_mode() == m && g_show_pane;
	bool ret = ImGui::MenuItem(get_window_data(m)->get_title(), nullptr, &sel);
	if (ret) {
		set_view_mode(m);
	};
	return ret;
}


static void create_new_document()
{
	try_open_file([]() {
		g_ims_info.reset();
		get_vb().init9(ifs_list_get());
		s_ui.m_editor.m_eidt_type = ws_editor::EDITOR_SOURCE;
		set_view_mode(ListViewMode::EDITOR);
	});
};


static void ShowMainMenu()
{

	if (ImGui::BeginMenu("File")) {
		IMS_SCOPE([] {ImGui::EndMenu(); });

		if (ImGui::MenuItem("New")) {
			create_new_document();
		};


		if (ImGui::MenuItem("Open", "Ctrl-O")) {
			do_open_file();
/*
			auto DialogFileCallback=[](void* userdata, const char* const* filelist, int filter)
			{
					int qq = 0;

			};
			SDL_Window* MainWindow_get();
			SDL_ShowOpenFileDialog(DialogFileCallback, nullptr, MainWindow_get(),
				nullptr, 0, "C:/Temp/", SDL_FALSE);

*/

		};


		if (!g_last_file_all.empty()) {
			if (ImGui::MenuItem("Reload", "Ctrl-R")) {
				do_reload_file();
			};
		}

		if (ImGui::MenuItem("Open Example")) {
			set_view_mode(ListViewMode::EXAMPLES);
		};

		if (ImGui::MenuItem("Save", "Ctrl-S")) {
			do_save_as();
		}


		ImGui::Separator();


		if (ImGui::MenuItem("Clipboard...")) {
			do_show_clipboard_dlg();
		}

		
#ifndef NDEBUG
		if (ImGui::MenuItem("Save X3D")) {
			do_save_x3d();
		}
#endif

		ImGui::Separator();
		if (ImGui::MenuItem("Save Picture As")) {
			do_save_picture();
		};


		if (ImGui::MenuItem("Export CSV")) {
			do_export_csv();
		}
#if 0
	

		ImGui::Separator();
		if (ImGui::BeginMenu("Recent")) {
			IMS_SCOPE([&] {ImGui::EndMenu(); });
			ImGui::MenuItem("Dummy", nullptr, nullptr);
		}
#endif

		ImGui::Separator();

#ifdef __EMSCRIPTEN__
		//SDL_SetEventFilter doesn't work on Chrome Android
		if (ImGui::MenuItem("Save workspace", nullptr)) {
			save_settings();
			//emscripten_run_script("window.close();");
		};
#else
		if (ImGui::MenuItem("Exit", nullptr)) {
			do_exit_confirm();
		};
#endif

}
	if (ImGui::BeginMenu("View")) {
		IMS_SCOPE([] {ImGui::EndMenu(); });

		menu_item_mode(ListViewMode::SETTINGS);
		menu_item_mode(ListViewMode::CONSOLE);
		ImGui::Separator();
		menu_item_mode(ListViewMode::LIST);
		menu_item_mode(ListViewMode::Components);
		menu_item_mode(ListViewMode::EDITOR);
		menu_item_mode(ListViewMode::LOCATION);
		menu_item_mode(ListViewMode::PALETTE);
		menu_item_mode(ListViewMode::RENDER);
		menu_item_mode(ListViewMode::ANIM);
		menu_item_mode(ListViewMode::FINDER);
		menu_item_mode(ListViewMode::CREATOR);
		

#ifndef IMGUI_DISABLE_DEMO_WINDOWS
		ImGui::Separator();
		ImGui::MenuItem("Test", nullptr, &g_show_test_window);
#endif

	}


	if (ImGui::BeginMenu("Build")) {
		IMS_SCOPE([] {ImGui::EndMenu(); });

		if (ImGui::MenuItem("Fit to screen")) {
			do_fit_to_screen();
		};
		//if (ImGui::MenuItem("Zoom Out")) {
		//	do_zoom_out();
		//};
		ImGui::Separator();

		{
			{
				bool m = get_build_mode() == ifs_object_type::normal;
				if (ImGui::MenuItem("Normal", nullptr, &m)) {
					set_build_mode(ifs_object_type::normal);
				};
			}
			{
				bool m = get_build_mode() == ifs_object_type::boundary;
				if (ImGui::MenuItem("Boundary", nullptr, &m)) {
					set_build_mode(ifs_object_type::boundary);
				};
			}

			if (!get_report_params().empty())
			{
				bool m = get_build_mode() == ifs_object_type::custom;
				if (ImGui::MenuItem("Custom", nullptr, &m)) {
					set_build_mode(ifs_object_type::custom);
				};
			}
		};

		ImGui::Separator();

		{
			bool m;

			m = g_thum_enabled;
			if (ImGui::MenuItem("Thumbnail", nullptr, &m)) {
				switch_thumbnail();
			}
		
			m = !g_right_select;
			if (ImGui::MenuItem("Select", nullptr, &m, g_thum_enabled)) {
				g_right_select = false;
			};

			m = g_right_select;
			if (ImGui::MenuItem("Check", nullptr, &m, g_thum_enabled)) {
				g_right_select = true;
			};
		}
		
		ImGui::Separator();

		if (ImGui::MenuItem("Pause")) {
			ims_worker::pause_workers(true);
		};
		if (ImGui::MenuItem("Resume")) {
			ims_worker::pause_workers(false);
		};
		if (ImGui::MenuItem("Stop")) {
			ims_worker::stop_all();
		};
	}

	if (ImGui::BeginMenu("Tools")) {
		IMS_SCOPE([] {ImGui::EndMenu(); });

		{
			bool m = g_mouse_action == MouseAction::zoom;
			if (ImGui::MenuItem("Zoom box", nullptr, &m)) {
				g_mouse_action = MouseAction::zoom;
			};
		}
		{
			bool m = g_mouse_action == MouseAction::rotate;
			if (ImGui::MenuItem("Rotate", nullptr, &m)) {
				g_mouse_action = MouseAction::rotate;
			};
		}
		{
			bool m = g_mouse_action == MouseAction::pan;
			if (ImGui::MenuItem("Pan", nullptr, &m)) {
				g_mouse_action = MouseAction::pan;
			};
		}
#if 0
		{
			bool m = g_mouse_action == MouseAction::roll;
			if (ImGui::MenuItem("Roll/zoom", nullptr, &m)) {
				g_mouse_action = MouseAction::roll;
			};
		}
#endif

		ImGui::Separator();
		{
			auto* xd = get_global_bd();

			let* bb = xd && xd->can_create_view() ? get_cur_block() : nullptr;

			if (ImGui::MenuItem("View->List", "Ctrl =", false, bb != nullptr)) {
				add_view_to_list(xd, bb);
			};
		
#if 0
			if (ImGui::MenuItem("Full screen", "F11", false)) {
				switch_fullscreen();
			};
#endif
		}
		
	}

	if (ImGui::BeginMenu("Help")) {
		IMS_SCOPE([] {ImGui::EndMenu(); });

#if 0
		static const char* dlgid = "UI Guide";
		if(ImGui::MenuItem(dlgid)){
			static bool popen = true;
			if(ImGui::Begin(dlgid, &popen)){
				ImGui::ShowUserGuide();
			};
			ImGui::End();
		}
#endif

		if (ImGui::MenuItem("Active window")) {
			set_help_active_window();
		}
		if (ImGui::MenuItem("Current file")) {
			set_help_file();
			set_view_mode(ListViewMode::HELP);
		}
		if (ImGui::MenuItem("Current block")) {
			set_help_block();
			set_view_mode(ListViewMode::HELP);
		}
		if (ImGui::MenuItem("Shortcuts")) {
			s_ui.m_help.set_mode(ws_help::mode::other);
			s_ui.m_help.m_help_id = "Shortcuts";
			set_view_mode(ListViewMode::HELP);
		}
		if (ImGui::MenuItem("File Format")) {
			s_ui.m_help.set_mode(ws_help::mode::other);
			s_ui.m_help.m_help_id = "AIFS";
			set_view_mode(ListViewMode::HELP);
		}
		ImGui::Separator();
		if (ImGui::MenuItem("About")) {
			do_show_about_dlg();
		}
	}
};

static float get_status_and_menu_height()
{
	return ImGui::GetFrameHeight();
}

static void navigate_by_status_bar_click(float rpos)
{
	if (rpos < 0.3333f) {
		if (get_cur_mode() == ListViewMode::Components || !change_block(-1, false)) {
			change_set(-1, true);
		}

	}
	else if (rpos > 0.6666f) {
		if (get_cur_mode() == ListViewMode::Components || !change_block(1, false)) {
			change_set(1, true);
		}
	}
	else {
		switch_tool_windows();
	}
};

static void ShowStatusBar(std::string_view text)
{
	let& ds = ImGui::GetIO().DisplaySize;
	if (ds.x <= 0 || ds.y <= 0)return;

	let sh = get_status_and_menu_height();

	ImGui::SetNextWindowPos(ImVec2(0, ds.y - sh));
	ImGui::SetNextWindowSize(ImVec2(ds.x, sh));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImGui::GetStyle().FramePadding);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::Begin("Statusbar", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::TextUnformatted(text.data(), text.data() + text.size());

	if (ImGui::GetIO().MouseClicked[0]) {
		let& pos = ImGui::GetIO().MouseClickedPos[0];
		if (pos.y > ds.y - sh) {
			navigate_by_status_bar_click(pos.x / ds.x);
		}
	}

	ImGui::End();

	ImGui::PopStyleVar(3);
}




static bool toolbar_button(float sz, const char* label, const char* tip, bool enabled = false)
{
	extern ImFont* g_font_awesome;

	ImGui::SameLine();
	auto& style = ImGui::GetStyle();
	let& bg_color = ImGui::GetStyle().Colors[ImGuiCol_ChildBg];
	let& fg_color = style.Colors[enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled];

	

	//ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.0,0.0 });
	
	ImGui::PushStyleColor(ImGuiCol_Text, fg_color);
	ImGui::PushStyleColor(ImGuiCol_Button, bg_color);

	ImGui::PushFont(g_font_awesome, g_toolbar_font_height * 1.3f * sz);
	ImGui::PushID(label);
	

	const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);

	let bh = g_toolbar_button_heigth * 1.18f;
	let bw = bh * label_size.x / label_size.y*1.3f;

	bool ret = ImGui::Button(label, ImVec2(bw, bh));
	ImGui::PopID();
	ImGui::PopFont();
	ImGui::PopStyleColor(2);
	//ImGui::PopStyleVar();

	set_tooltip(tip);

	
	return !g_toolbar_drag_in_progress && ret;
}



static void ShowToolbar()
{

	let& ds = ImGui::GetIO().DisplaySize;

	{
		let m = ds.x - g_toolbar_width;
		if (g_toolbar_from_x < m)g_toolbar_from_x = m;
		if (g_toolbar_from_x > 0)g_toolbar_from_x = 0;
	}


	ImGui::SetNextWindowPos(ImVec2(g_toolbar_from_x, g_toolbar_from_y));
	ImGui::SetNextWindowSize(ImVec2(ds.x - g_toolbar_from_x, g_toolbar_heigth));

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::Begin("Toolbar", nullptr,
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoBringToFrontOnFocus);

	IMS_SCOPE([] {
		ImGui::End();
		ImGui::PopStyleVar(6);
	});





	if (ImGui::IsWindowFocused()) {
		check_navi_ex(true);

		if (ImGui::IsMouseDragging(0)) {
			let delta = ImGui::GetMouseDragDelta(0);
			g_toolbar_from_x += delta.x;
			ImGui::ResetMouseDragDelta(0);
			g_toolbar_drag_in_progress = true;
		}
	}


	////////////////////////////////////////////////////////////////////////////
	

	if (toolbar_button(1.0f, ICON_FK_CHEVRON_LEFT, "Back")) {
		on_back_button_pressed();
	}

	if (get_settings().m_window_mode == window_mode_type::full)
	{
		if (toolbar_button(1.0f, ICON_FK_EJECT, "Show/Hide Viewport")) {
			switch_tool_windows();
		}
	};

	if (toolbar_button(0.8f, ICON_FK_FILE, "Creator", 
		get_cur_mode() == ListViewMode::CREATOR)) {
		show_helper(ListViewMode::CREATOR);
	}


	if (toolbar_button(1.0f, ICON_FK_FOLDER_OPEN, "Open File")) {
		do_open_file();
	}


	let not_empty = !ifs_list_get().empty();

	if (not_empty) {
		if (toolbar_button(1.0f, ICON_FK_FLOPPY_O, "Save")) {
			do_save_as();
		}
	}

	if (toolbar_button(1.0f, ICON_FK_STAR, "Open Examples", 
		get_cur_mode() == ListViewMode::EXAMPLES)) {
		show_helper(ListViewMode::EXAMPLES);
	}


	if (not_empty) {

		bool enabled = 
			get_cur_mode() == ListViewMode::LIST ||
			get_cur_mode() == ListViewMode::Components;
		if (toolbar_button(1.0, ICON_FK_BARS, "IFS List/Components", enabled)){
			//unusual
			show_helper((enabled != g_thum_is_list) ?
				ListViewMode::LIST : ListViewMode::Components);
		}


		if (toolbar_button(0.8f, ICON_FK_BINOCULARS, "Finder",
			get_cur_mode() == ListViewMode::FINDER)) {
			show_helper(ListViewMode::FINDER);
		}

		if (toolbar_button(1.0f, ICON_FK_PENCIL_SQUARE_O, "Editor",
			get_cur_mode() == ListViewMode::EDITOR)) {
			show_helper(ListViewMode::EDITOR);
		}


		if (toolbar_button(1.0f, ICON_FK_STOP_CIRCLE_O, "Stop All")) {
			ims_worker::stop_all();
		}

		{

			let m = get_build_mode();
			const char* title_next_mode;
			const char* icon;
			ifs_object_type next_mode;

			float fsz = 1.0f;

			if (m == ifs_object_type::normal) {
				icon = ICON_FK_SQUARE;
				title_next_mode = "Switch to Boundary Mode";
				next_mode = ifs_object_type::boundary;
			}
			else if (m == ifs_object_type::boundary) {
				icon = ICON_FK_SQUARE_O;
				if (!get_report_params().empty()) {
					title_next_mode = "Switch to Custom Mode";
					next_mode = ifs_object_type::custom;
				}
				else {
					title_next_mode = "Switch to Normal Mode";
					next_mode = ifs_object_type::normal;
				}
			}
			else {
				icon = ICON_FK_FUTBOL_O;
				fsz = 0.8f;
				title_next_mode = "Switch to Normal Mode";
				next_mode = ifs_object_type::normal;
			}

			if (toolbar_button(fsz, icon, title_next_mode)) {
				set_build_mode(next_mode);
			}
		}

		if (toolbar_button(1.0f, ICON_FK_CROSSHAIRS, "Fit to screen")) {
			do_fit_to_screen();
		}



		if (toolbar_button(1.0f, ICON_FK_SEARCH_MINUS, "Zoom Out")) {
			do_zoom_out();
		}


#if 0	
		if (toolbar_button(g_right_zoom ? ICON_FK_UNDO :
			ICON_FK_SEARCH_PLUS,
			g_right_zoom ? "Rotation Mode" : "Zoom Mode"
		))
		{
			g_right_zoom = !g_right_zoom;
	}
#endif		


		if (toolbar_button(1.0f, ICON_FK_TH, "Thumbnail view", g_thum_enabled)) {
			switch_thumbnail();
		}


		let is_shift = is_shift_down();

		if (toolbar_button(1.0f, ICON_FK_ARROW_UP, "Previous IFS")) {
			change_block(-1, is_shift);
		}



		if (toolbar_button(1.0f, ICON_FK_ARROW_DOWN, "Next IFS")) {
			change_block(1, is_shift);
		}
};

	if (toolbar_button(0.9f, ICON_FK_WRENCH, "UI Settings",
		get_cur_mode() == ListViewMode::SETTINGS)) {
		show_helper(ListViewMode::SETTINGS);
	}


	if (toolbar_button(1.0f, ICON_FK_QUESTION_CIRCLE_O, "About...")) {
		do_show_about_dlg();
	}

	ImGui::SameLine(0, 0);
	g_toolbar_width = ImGui::GetCursorPosX();

	if (ImGui::IsWindowFocused() && ImGui::IsMouseReleased(0)) {
		g_toolbar_drag_in_progress = false;
	}
}


//returns the actual width of the panel
static float show_pane(window_state* pWnd, const frect& rc, bool use_base)
{
	let& st = get_settings();
	int pop_styles = 0;

	////////////////////////////////////////////////////////////////////////
	let alpha = use_base || st.is_docked() ? 1.0f : st.m_window_alpha;
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha); ++pop_styles;
	////////////////////////////////////////////////////////////////////////
	let bg_alpha = use_base || st.is_docked() ? 1.0f : st.m_backgr_alpha;
	ImGui::SetNextWindowBgAlpha(bg_alpha);

	static std::array<char, 256> wnd_id = { 0 };
	fmt::format_to_n(
		wnd_id.data(), wnd_id.size(), "{}{}\0", pWnd->get_title(), s_pane_id);

	int flags_size = ImGuiCond_Always;
	int flags = ImGuiWindowFlags_None;
	ImVec2 sz;

	if (use_base || st.m_window_mode == window_mode_type::full) {

		ImGui::SetNextWindowPos(ImVec2(rc.x1, rc.y1));
		sz = ImVec2(rc.width(), rc.height());

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0); ++pop_styles;

		flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings;
	}
	else {
		let& ds = ImGui::GetIO().DisplaySize;

		if (st.is_docked()) {
			ImGui::GetIO().ConfigWindowsResizeFromEdges = true;
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0); ++pop_styles;

			ImGui::SetNextWindowBgAlpha(1.0f);

			ImVec2 ps(0, 0);

			let z = (float)get_settings().m_docked_size;

			switch (st.m_window_mode) {
			case window_mode_type::left:
				ps = { rc.x1, rc.y1 };
				sz = { z, rc.height() };
				break;
			case window_mode_type::right:
				ps = { rc.x2 - z, rc.y1 };
				sz = { z, rc.height() };
				break;
			case window_mode_type::up:
				ps = { rc.x1, rc.y1 };
				sz = { rc.width(), z };
				break;
			case window_mode_type::down:
				ps = { rc.x1, rc.y2 - z };
				sz = { rc.width(), z };
				break;
			case window_mode_type::full:
			case window_mode_type::floating:
				break;
			}
			ImGui::SetNextWindowPos(ps, ImGuiCond_Always);

			flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove;
		}
		else {//window_mode_type::floating
			sz = ImVec2(ds.x / 4, ds.y / 2);
			flags_size = ImGuiCond_FirstUseEver;
		}

		if (g_reset_window_width || sz.x <= 0) {
			g_reset_window_width = false;
			sz.x = ds.x / 2;
			flags_size = ImGuiCond_Always;
			redraw_gui(1);
		}
	}

	///////////////////////////////////////////////////////////////////////////
	ImGui::SetNextWindowSize(sz, flags_size);
	bool b = ImGui::Begin(use_base? base_window_name : wnd_id.data(), &g_show_pane, flags);
	IMS_SCOPE([&] {
		ImGui::End();
		ImGui::PopStyleVar(pop_styles);
	});

	float ret = 0;

	if (st.is_docked(true)) {
		ret = ImGui::GetWindowWidth();
	}
	else if (st.is_docked(false)) {
		ret = ImGui::GetWindowHeight();
	}

	if (!b)return ret;

	
	auto& p = pWnd->m_scroll_pos;

	if (g_last_shown_window != get_cur_mode()) {
		if (g_last_shown_window < ListViewMode::NUM_WINDOWS) {
			get_window_data(g_last_shown_window)->on_hide();
		}
		g_last_shown_window = get_cur_mode();

		ImGui::SetScrollX(p[0]);
		ImGui::SetScrollY(p[1]);
		pWnd->on_show();
	}

	if (!g_file_open_in_progress) {
		pWnd->show();
	}

	p = { ImGui::GetScrollX(), ImGui::GetScrollY() };

	return ret;
}


//we can't return from the middle of this function
bool on_draw()
{
	let& ds = ImGui::GetIO().DisplaySize;

	if (ds.x <= 0 || ds.y <= 0) {//when minimizing the window, we do nothing
		g_rect.clear();
		return false;
	}

	let fp = ImGui::GetStyle().FramePadding.y * 2.0f;

	let sh = get_status_and_menu_height();

	auto& st = get_settings();

	g_toolbar_from_y = st.m_show_menu ? sh : 0;
	g_toolbar_button_heigth = g_toolbar_font_height * get_ui_scale();
	g_toolbar_heigth = g_toolbar_button_heigth + fp;

	//height of menu+toolbar
	let mh = g_toolbar_from_y + g_toolbar_heigth;

	let status_height = sh;


	g_rect.x1 = 0;
	g_rect.y1 = mh;
	g_rect.x2 = ds.x;
	g_rect.y2 = ds.y - status_height;


	if (ds.y <= status_height + mh) {
		return false;
	}


#if 0 //may be outdated
	auto* w = MainWindow_get();

	if (ImGui::GetIO().WantTextInput) {

		let h = 16.0f * get_ui_scale();//16.0f - ImGui magic
		let& m = ImGui::GetIO().MouseClickedPos[0];

		SDL_Rect rc{ 0, (int)m.y,(int)ds.x,(int)h };

		SDL_SetTextInputArea(w, &rc, 0);

		SDL_StartTextInput(w);
	}
	else {
		SDL_StopTextInput(w);
	}
#endif
	check_navi_ex(false);


	////////////////////////////////////////////////////////////////////////////
	if (s_ui.m_modal_msg.need_to_show()) {
		s_ui.m_about.m_show = false;
		s_ui.m_clipboard.m_show = false;
		s_ui.m_modal_msg.show();
	}

	if (!get_fds().empty3()) {
		s_ui.m_filedialog.show();
		return false;//occupies the entire screen
	}

	////////////////////////////////////////////////////////////////////////////
	if (st.m_show_menu) {
		//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		if (ImGui::BeginMainMenuBar()) {
			IMS_SCOPE([] { ImGui::EndMainMenuBar(); });
			ShowMainMenu();
		}
		ImGui::PopStyleVar(1);
	};

	////////////////////////////////////////////////////////////////////////////

	ShowToolbar();

	////////////////////////////////////////////////////////////////////////////
	//show the panel and find its width
	//call with the previous frame's work area

	if (g_show_pane) {
		let w = show_pane(get_window_data(get_cur_mode()), g_rect, false);
		if (w > 0) {
			st.m_docked_size = w;
		}
	}

	
#if 0
	{
		
		ims_func_static bool g_show_keybord = true;

		int flags = 0;

		bool b = ImGui::Begin("keyboard", &g_show_keybord, flags);

		static std::string buf;
		if (ImGui::InputText("Text", &buf)) {}

		let key = ImGui::VirtualKeyboard();

		//let keyChar = ImGui::GetKeyName(key);

		if (key >= ImGuiKey_0 && key <= ImGuiKey_Z) {
			char sym = '0' + key - ImGuiKey_0;
			buf.push_back(sym);
		}
		if (key == ImGuiKey_Backspace && !buf.empty()) {
			buf.pop_back();
		}

		ImGui::End();
	
	}
#endif
	

	////////////////////////////////////////////////////////////////////////////

	//on top of the rest
	if (s_ui.m_about.m_show) {
		s_ui.m_about.show();
	}

	if (s_ui.m_clipboard.m_show) {
		s_ui.m_clipboard.show();
	}

	////////////////////////////////////////////////////////////////////////////
	if (!g_ps.m_img_draw_uploaded) {
		//without subtracting one unit, freezes on 3D examples
		let u1 = g_ps.can_upload_img(min_build_time_ms - 1);
		let u2 = get_thread(e_ims_threads::build).is_running();
		//std::cout << u1 << " " << u2 << std::endl;
		if (u1 || !u2) {
			upload_img();
		}
	}

#if 0	
	if (get_ui_scale() != 1.0f) ImGui::PushFont(g_font_ttf);
	IMS_SCOPE([&] {
		if (ImGui::GetFont() == g_font_ttf)ImGui::PopFont();
	});
#endif


	////////////////////////////////////////////////////////////////////////////
#ifndef IMGUI_DISABLE_DEMO_WINDOWS
	if (g_show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
		ImGui::ShowDemoWindow(&g_show_test_window);
	}
#endif //IMGUI_DISABLE_DEMO_WINDOWS
	////////////////////////////////////////////////////////////////////////////
	g_status_text.clear();

	if (is_search_started()) {
		g_status_text += std::to_string(g_rate_checked) + "/s ";
	}

	let& tb = get_thread(e_ims_threads::build);

	if (tb.is_running()) {
		let bs = g_ps.m_build_scale;
		if (bs == 0) {
			g_status_text += std::string("[init]");
		}
		else {
			switch (g_ps.m_build_mode) {
			case program_state::build_mode::paint:
				g_status_text += "[p]";
				break;
			case program_state::build_mode::ssao:
				g_status_text += "[s]";
				break;
			default:
				break;
			}

			if (bs > 1) {
				g_status_text += std::string("[1/") + std::to_string(bs) + "]";
			}
		}
		g_status_text +=
			std::to_string(tb.work_done() * 100) + "%";

	}

	//before drawing the StatusBar because it changes the text

	

	let rc = get_working_area();
	if (!rc.empty()) {
		bool show_console = 
			get_cur_mode() != ListViewMode::CONSOLE &&
			get_settings().m_window_mode != window_mode_type::full;

		if (show_console) {
			let* bi = get_global_bd();
			if (bi && bi->m_bi.exists()) {
				show_console = false;
			}
		}

		if (show_console) {
			show_pane(get_window_data(ListViewMode::CONSOLE), rc, true);
		} else {
			draw_base(rc);
		}

	};

	if (g_build_complete_proc && !tb.is_running()) {
		g_build_complete_proc();//here the build thread is usually started again
		g_build_complete_proc = nullptr;
	}

	if (g_status_text.empty()) {
		g_status_text = "Ready";
	}
	ShowStatusBar(g_status_text);


	////////////////////////////////////////////////////////////////////////

	//at the very end, otherwise IMGUI may crash, for example in the file open dialog
	if (get_thread(e_ims_threads::aux).is_running() &&
		!s_ui.m_modal_msg.need_to_show())
	{
		show_aux_dialog();
	}


	//let str = std::to_string(ImGui::GetIO().DisplayFramebufferScale.x);
	//ims_show_message(str.c_str());
	
	////////////////////////////////////////////////////////////////////////////
	// Rendering

	return true;
}

//background window - Image+ZoomBox
//must call ImGui::Begin in any case, otherwise IMGUI will have problems with focus
static void draw_base(const frect& rc)
{

	let bx = (float)g_width;
	let by = (float)g_height;
	let dx = rc.width();
	let dy = rc.height();

	ImGui::SetNextWindowContentSize(ImVec2(bx, by));

	ImGui::SetNextWindowPos(ImVec2(rc.x1, rc.y1));
	ImGui::SetNextWindowSize(ImVec2(dx, dy));
	ImGui::SetNextWindowBgAlpha(0);
	////////////////////////////////////////////////////////////////////////////
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	ImGui::Begin(base_window_name, nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_HorizontalScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBringToFrontOnFocus);

	IMS_SCOPE([&] {
		ImGui::End();
		ImGui::PopStyleVar(3);
	});


	let msx = ImGui::GetScrollMaxX();
	let msy = ImGui::GetScrollMaxY();

	let sx = ImGui::GetScrollX();
	let sy = ImGui::GetScrollY();

	let rx = bx - msx;
	let ry = by - msy;


	//SDL_Log("set_region %f, %f, %f, %f", sx, msy - sy, rx, ry);
	bool redr = g_gl.set_region((size_t)sx, (size_t)(msy - sy), (size_t)rx, (size_t)ry);

	if (redr) {
		upload_img();
	}

	//active area dimensions

	let sbs = (size_t)ImGui::GetStyle().ScrollbarSize;

	//a horizontal bar reduces the vertical area and vice versa
	let ax = msy > 0 ? (dx > sbs ? dx - sbs : 0) : dx;
	let ay = msx > 0 ? (dy > sbs ? dy - sbs : 0) : dy;

	let kx = ax <= rx ? 0 : (ax - rx) / 2;
	let ky = ay <= ry ? 0 : (ay - ry) / 2;

	g_image_pos.c[0] = int(rc.x1 + kx);
	g_image_pos.c[1] = int(rc.y1 + ky);
	g_image_pos.s[0] = (int)rx;
	g_image_pos.s[1] = (int)ry;

	g_scroll_pos = { sx, sy };


	////////////////////////////////////////////////////////////////////////////
	//after filling g_image_pos
	let hm = has_modal();


	if (!hm && ImGui::IsWindowHovered()) {

		check_navi_ex(true);

		let mp = ImGui::GetMousePos();

		let* mc = ImGui::GetIO().MouseClicked;
		if (!g_thum_enabled) {
			check_base_input(mp.x, mp.y, mc);
		}else {
			check_thumb_input(mp.x, mp.y, mc);
		}
		
#if 0
		if (ImGui::GetMouseCursor() == ImGuiMouseCursor_Arrow) {
			ImGui::SetMouseCursor(g_rotation_mode ?
				ImGuiMouseCursor_Hand :
				ImGuiMouseCursor_ResizeAll);
		};
#endif


		if (!g_thum_enabled) {

			int but;
			if (ImGui::IsMouseDragging(0))but = 0;
			else if (ImGui::IsMouseDragging(1))but = 1;
			else if (ImGui::IsMouseDragging(2))but = 2;
			else but = -1;

			if (but >= 0) {
				let md = ImGui::GetMouseDragDelta(but);

				//where did the use start drag from?
				let cx = mp.x - md.x;
				let cy = mp.y - md.y;

				//it is not allowed to start drag from scrollbars
				if (g_image_pos.contain2((int)cx, (int)cy)) {
					if (md.x != 0 || md.y != 0) {
						if (g_thumbnail.get_num() == 1) {
							auto* bd = get_global_bd();
							if (bd) {
								let act = get_action(but);
								bool b = check_mouse_dragging(bd, cx, cy, md.x, md.y, act);
								if (b && !get_thread(e_ims_threads::build).is_running()) {
									ImGui::ResetMouseDragDelta(but);
								}
							}
						}
					}
				}
			}
			else {
				if (g_zoom_box_visible) {
					g_zoom_box_visible = false;

					if (use_zoom_box()) {
						do_rebuild();
					};
				}
				else {
					let mw = ImGui::GetIO().MouseWheel;
					if (mw != 0) {
						if (use_mouse_wheel(mp.x, mp.y, mw)) {
							do_rebuild();
						};
					}
				}
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	auto* dl = ImGui::GetWindowDrawList();

	if (!g_gl.empty()) {

		ImVec2 a, b;
		a.x = float(rc.x1 + kx);
		a.y = float(rc.y1 + ky);
		b.x = float(rc.x1 + kx + rx);
		b.y = float(rc.y1 + ky + ry);

		let mx = g_gl.get_scale(0) * float(rx);
		let my = g_gl.get_scale(1) * float(ry);
		if (mx > 0 && my > 0) {
			dl->AddImage(g_gl.get_screen_texture(),
				a, b, ImVec2(0, my), ImVec2(mx, 0));
		}
	}


	if (is_thumb_list() && g_right_select) {

		for (size_t i = 0;; ++i) {
			let block_idx = get_vb().m_cur_block_pos + i;
			oper_block* sr = get_vb().get_vis(block_idx);
			if (!sr)break;

			size_t ix, iy;
			g_thumbnail.get_xy(i, ix, iy);
			iy = g_thumbnail.get_ny() - iy - 1;

			auto fx = float(ix * g_thumbnail.get_tx() + g_thumbnail.get_tx() / 2);
			auto fy = float(iy * g_thumbnail.get_ty() + g_thumbnail.get_ty() / 2);


			if (!from_image(fx, fy))break;

			auto sf = (float)std::min({ 
				g_thumbnail.get_tx() / 4,
				g_thumbnail.get_ty() / 4 });
			if (sf == 0)sf = 1;

			if (sr->m_flags.checked) {
				dl->AddRect(
					ImVec2(fx - sf, fy - sf),
					ImVec2(fx + sf, fy + sf),
					ImColor(0, 0, 0),
					0, 0, 1);

				sf -= 1;

				dl->AddRect(
					ImVec2(fx - sf, fy - sf),
					ImVec2(fx + sf, fy + sf),
					ImColor(255, 255, 255),
					0, 0, 1);
			}
		}





	}

	if (g_zoom_box_visible) {

		let& z0 = g_zoom_box[0];
		let& z1 = g_zoom_box[1];

		dl->AddRect(
			ImVec2(z0[0], z0[1]),
			ImVec2(z1[0], z1[1]),
			ImColor(0, 0, 0),
			0, 0, 1);

		dl->AddRect(
			ImVec2(z0[0] - 1, z0[1] - 1),
			ImVec2(z1[0] + 1, z1[1] + 1),
			ImColor(255, 255, 255),
			0, 0, 1);
	}
}

