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

#include "ui_utils.h"

//types from third-party libraries should not appear here

//order is important (s_all_windows)
enum class ListViewMode
{
	LIST,
	FINDER,
	SELCOL,
	SELVIS,
	EXAMPLES,
	CREATOR,
	EDITOR,
	CONSOLE,
	LOCATION,
	PALETTE,
	SETTINGS,
	RENDER,
	Components,
	GRAPHS,
	ANIM,
	HELP,
	CLIPBOARD,
	NUM_WINDOWS,
};

enum class e_what_print : int
{
	Definition,
	Evaluation,
	NormalMaps,
	Projection,
	Dimension,
	Geometry,
	Measure,
	Subspaces,
	Data,
	AST,
	ExecJS,
	NUMBER_OF_WHAT_PRINT,
};

//in order of priority
enum class e_ims_threads
{
	aux,
	build,
	creator,
	search,
	num,
};
struct oper_block;

struct ims_worker& get_thread(e_ims_threads v);

constexpr size_t min_build_time_ms = 100;

std::mutex& get_list_lock();

void do_exit_confirm();

void ims_show_message(std::string_view msg);

template <typename... Args>
void ims_show_message_fmt(fmt::format_string<Args...> s, Args&&... args)
{
	let str = fmt::format(s, std::forward<Args>(args)...);
	return ims_show_message(str);
}

//if show==false, then it will just call the callback immediately
void ims_confirm_dlg(
	std::string_view msg, 
	std::function<void()>&& F, 
	bool show = true);

void try_open_file(std::function<void()>&& F, bool use_confirm = true);
void stop_build_then(std::function<void()>&& f, uint64_t time_ms = min_build_time_ms);

void on_constructor_success(size_t blocks_start_from);

void StartSearch();

void set_last_file(const std::string& filename, bool set_name, bool set_folder);
void set_thread_name(const char* name);
bool is_program_minimized();
void redraw_gui(unsigned v);


//insert a block with ownership transfer
//returns true if visible
//requires setting an id before
bool add_block2(std::unique_ptr<oper_block>& nb, std::string_view id);

void update_ui_async();
void do_batch_rendering();
bool is_batch_in_progress();
uint32_t get_batch_ready_blocks();
void do_create_anim(size_t ref_time);
void do_rebuild();
bool do_from_clipboard();
void do_copy_all(bool base64);
void do_copy_to_clipboard(bool base64, bool merge_parents);
size_t apply_converters();


struct draw_task;
void do_build(const draw_task&);

void do_rebuild_sync();


void do_save_exr();

void stop_build();

void open_file(
	const std::string& filename, 
	std::string&& data,//content (may be empty)
	bool keep_set, 
	bool to_recent, 
	const struct edit_info* edit);

void remove_search();
void StartSearch();
void set_view_mode(ListViewMode m);
ListViewMode get_cur_mode();

void call_main_thread(void(*f)());




struct samples& get_samples();

void reset_window_width();

void get_working_res(size_t& w, size_t& h);
bool is_thumb_enabled();
void on_change_max_thumb(size_t new_val);
void check_navi_ex(bool c);
struct oper_block* get_cur_block();

struct palette;
palette& get_cur_palette();
void on_change_color();
void do_save_palette(const palette& p);
void do_load_palette(palette& p);
struct background& get_background();
struct colorize_params& get_colorize_params();
struct render_params& get_rpars();


void do_build_mesh();
bool editor_ready();

void remove_checked();
struct visible_blocks& get_vb();
bool is_shift_down();
bool is_control_down();
void set_block_ex(size_t idx);
void set_block_direct(const oper_block* b);
void set_block_and_build(size_t idx);


void on_change_store_time_flag(bool b);
bool is_search_started();


bool& need_update_vis_cols();


struct report_params& get_report_params();
struct ims_setting& get_settings();

struct file_dialog_state& get_fds();




void ui_update_maps();



oper_block* get_global_block();

struct build_data;
build_data* get_global_bd();

struct eval_data;
eval_data& get_global_ed();

void rand_current_set();

void show_generic_error_msg();


void add_view_to_list(build_data* bd, const oper_block* bb);

void set_new_ver_ref(size_t new_ver_ref);

void console_execute(std::string_view script);
void console_print(e_what_print what);

size_t get_default_block();
void init6(bool need_save);

void ui_onload();

void save_env_block(bool only_if_exists, bool fparams, bool rparams);

bool load_env_block_ex(bool fparams, bool rparams);

void clear_before_load();

struct ims_info;
ims_info& ims_info_get();

struct ifs_list;
ifs_list& ifs_list_get();