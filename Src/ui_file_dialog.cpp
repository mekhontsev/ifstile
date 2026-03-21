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
#include "ui_file_dialog.h"
#include "ims_window_drag.h"
#include "gui.h"
#include "file_dialog_state.h"
#include "ims_file.h"
#include "platform.h"
#include "edit_helper.h"

SDL_Window* MainWindow_get();

#ifdef __ANDROID__

void OnChooseFile(int fd, int is_save, const std::string& filename)
{
	auto& sp = get_fds();

	if (0 == is_save) {

		auto* f = fdopen(fd, "r");
		if (!f)return;

		std::string total;
		
		std::array<char, 1024> buf;

		for (;;) {
			let sz = fread(buf.data(), 1, buf.size(), f);
			if (sz)total.append(buf.data(), sz);
			if (sz < buf.size()) { break; }
		}

		fclose(f);

		open_file(filename, std::move(total), false, false, nullptr);
		sp.clear();//closes the dialog

	} else {

		auto* f = fdopen(fd, "w");//fclose is not done here

		if (!f) {
			ims_show_message(std::string("Could not save file:\n") + filename);
			return;
		}

		sp.m_fw.set_file(f);
		sp.call_cb_int(filename);
		sp.clear();
#if 0
		ims_print("f={} filename={} fd={} requestCode={}\n",
			(size_t)f, filename, fd, (int)t);
#endif
	}

}

void AndroidChooseFile(const std::string& filename, int is_save);

#endif  //__ANDROID__



const char* ws_file_dialog::get_title()
{
	return get_fds().m_title.c_str();
}

void ws_file_dialog::show()
{
	auto& sp = get_fds();
	assert(!sp.empty3());
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	IMS_SCOPE([] {ImGui::PopStyleVar(); });
	let flags =
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar;

	let& ds = ImGui::GetIO().DisplaySize;
	ImGui::SetNextWindowSizeConstraints(ds, ds);
	ImGui::SetNextWindowBgAlpha(1.0f);

	ImGui::Begin(get_title(), nullptr, flags);
	IMS_SCOPE([] {ImGui::End(); });

	int next_id = 0;

	let& s = ImGui::GetStyle();

	let wsz = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
	let num = sp.m_path.size();


	for (size_t i = 0; i < sp.m_path.size(); ++i) {

		ImGui::PushID(next_id++);
		bool b = ImGui::Button(sp.m_path[i].c_str());
		ImGui::PopID();

		if (b) {
			//a refresh will occur if the last button was pressed
			if (sp.shrink_path(i + 1)) {
				break;//cut the path
			};
		}

		if (i + 1 < num) {
			//button size
			let sz = ImGui::CalcTextSize(sp.m_path[i + 1].c_str()).x +
				s.FramePadding.x * 2;
			if (ImGui::GetItemRectMax().x + s.ItemSpacing.x + sz < wsz) {
				ImGui::SameLine();
			}
		}
	}

	let ws = ImGui::GetWindowSize().x;

	edit_helper::begin(next_id++);
	let cp = ImGui::GetCursorPosX();
	ImGui::PushItemWidth(ws - 2 * cp);
	ImGui::InputText("", &sp.m_name);
	ImGui::PopItemWidth();
	edit_helper::end();

	ImGui::Separator();
	///////////////////////////////////////////////////////
	if (ImGui::Button(" OK ")) {

		if (!sp.check_for_path()) {
			on_file_dlg_ok(false);
		}
	}

	{
		SAME_LINE();
		if (ImGui::Button("Cancel")) {
			sp.clear();
		}
	}

	{
		SAME_LINE();
		if (ImGui::Button("Home")) {
			sp.set_path(platform::getPathHome());
		}
	}

#if 0 && defined(__ANDROID__)
	{
		SAME_LINE();
		if (ImGui::Button("Ext")) {
			let* ep = AndroidGetExternalPath(
				sp.m_type == file_dialog_state::type::open);
			if (ep) {
				sp.set_path(ep);
			} else {
				ims_show_message("External storage is not accessible");
			}
		}
	}
#endif


	if (sp.m_type != file_dialog_state::type::open) {
		SAME_LINE();
		if (ims_button("+Dir", "Create Directory")) {
			if (!sp.create_dir()) {
				let err = std::string("Could not create dir ") + sp.m_name;
				ims_show_message(err);
			}
		}
	} else {
		SAME_LINE();
		if (ims_button("-Del", "Remove File/Directory")) {

			let msg = std::string("Are you sure to remove?\n") + 
				sp.get_full_path() + sp.m_name;

			ims_confirm_dlg(msg, []{
				if (!get_fds().remove_dir()) {
					let err = std::string("Could not remove dir");
					ims_show_message(err);
				}
			});
		}
	}

	if (sp.m_type == file_dialog_state::type::folder) {
		SAME_LINE();
		ImGui::PushID(next_id++);
		ImGui::Checkbox("show files", &sp.m_show_files);
		ImGui::PopID();
	} else {
		if (sp.m_type == file_dialog_state::type::save && sp.m_flags != 0) {

			//"all" is always available
			
			static constexpr auto cb = { "Current","Checked","All" };
			std::array<uint8_t, 1+(uint8_t)save_type::all> arr;
			size_t num_items = 0;
			int cur_sel = -1;
			for (uint8_t i = 0; i < arr.size(); ++i) {
				if ((sp.m_flags & (1 << i)) != 0) {
					if (i == (uint8_t)sp.m_save_type) {
						cur_sel = (int)num_items;
					}
					arr[num_items++] = i;
				};
			};

			assert(num_items > 0);

			if (cur_sel < 0)cur_sel = int(num_items - 1);

			auto getter = [](void* data, int idx)
			{
				let* arr = (uint8_t*)data;
				return cb.begin()[arr[idx]];
			};


			SAME_LINE();
			ImGui::PushItemWidth(80.0f * get_ui_scale());
			ImGui::PushID(next_id++);
			let b=ImGui::Combo("", &cur_sel, getter, arr.data(), (int)num_items);		
			ImGui::PopID();
			ImGui::PopItemWidth();


			sp.m_save_type = (save_type)arr[cur_sel];//set in any case
			
			if (b) {
				
				if (sp.m_save_type == save_type::single) {
					if (!sp.m_filename_single.empty())sp.m_name = sp.m_filename_single;
				} else {
					if (!sp.m_filename_all.empty())sp.m_name = sp.m_filename_all;
				}
			}
		}

		if(sp.m_ext.size() > 1){
			auto getter = [](void* data, int idx)->const char*
			{
				let* sp = (file_dialog_state*)data;
				if ((size_t)idx >= sp->m_ext.size())return nullptr;
				return sp->m_ext[idx].c_str();
			};

			SAME_LINE();
			ImGui::PushItemWidth(75.0f * get_ui_scale());
			ImGui::PushID(next_id++);
			let b = ImGui::Combo("", &sp.m_cur_ext_idx, getter, &sp, (int)sp.m_ext.size());
			ImGui::PopID();
			ImGui::PopItemWidth();
			if (b) {
				sp.on_change_ext();//change the extension if necessary
			};
		
		}
	
#if defined(__EMSCRIPTEN__)
		if (sp.m_type == file_dialog_state::type::save) {
			SAME_LINE();
			ImGui::PushID(next_id++);
			ImGui::Checkbox("download", &sp.m_download);
			ImGui::PopID();	
		}
#endif			
	}

#if defined(__EMSCRIPTEN__)

	void Emscripten_upload();
	void Emscripten_download(std::string_view fn);

	if (file_dialog_state::type::open == sp.m_type) {
		{
			SAME_LINE();
			if (ImGui::Button("Download")) {
				//will cause the file to "download" in the user's browser
				let fn = sp.get_full_name();
				if (ims_file::is_exists(fn)) {
					Emscripten_download(fn);
				} else {
					ims_show_message("The file does not exists.");
				}
			}
		}
		{
			SAME_LINE();
			if (ImGui::Button("Upload")) {
				//for some reason it doesn't work synchronously
				call_main_thread(Emscripten_upload);
				sp.clear();
			}
		}
	};
#endif



#if defined(__ANDROID__)
	if (file_dialog_state::type::folder != sp.m_type) {
		{
			SAME_LINE();
			if (ImGui::Button("Browse")) {
				AndroidChooseFile(sp.m_name, 
					sp.m_type== file_dialog_state::type::open?0:1);
			}
		}
	};
#endif

#if !defined(__EMSCRIPTEN__) &&  !defined(__ANDROID__)
	{
		SAME_LINE();
		if (ImGui::Button("Browse")) {

			struct browse_cb
			{
				static void SDLCALL cb(void* userdata, const char* const* filelist, int)
				{
					if (!filelist)return;
					auto* p = filelist[0];
					if (!p)return;

					auto* sp = ((file_dialog_state*)userdata);
					sp->set_path(p);

					if (on_file_dlg_ok(true)) {
						if (sp->m_type == file_dialog_state::type::folder) {
							set_last_file(p, false, true);
						}
					}
				};

			};


			auto fn = sp.get_full_name();
			

			
			auto* w = MainWindow_get();

			SDL_DialogFileFilter flt[10];
			size_t num_filter = 0;

			auto set_filters = [&]() {
				for (let& f : sp.m_ext) {
					auto* str = f.c_str();
					flt[num_filter].name = str;
					flt[num_filter].pattern = str;
					++num_filter;
				}
			};
			assert(std::size(flt) >= num_filter);

			if (sp.m_type == file_dialog_state::type::folder) {
				SDL_ShowOpenFolderDialog(
					browse_cb::cb,
					&sp,
					w,
					fn.c_str(),
					false);
			} else if (sp.m_type == file_dialog_state::type::open) {
				set_filters();
				SDL_ShowOpenFileDialog(
					browse_cb::cb,
					&sp,
					w,
					flt,
					(int)num_filter,
					fn.c_str(),
					false);

			} else if (sp.m_type == file_dialog_state::type::save) {
				set_filters();
				SDL_ShowSaveFileDialog(
					browse_cb::cb,
					&sp,
					w,
					flt,
					(int)num_filter,
					fn.c_str());
			}

			
		};
	}
#endif


	////////////////////////////////////////////////////////////////////////////
	constexpr std::array sl{ "Name" };
	bool bv = ImGui::BeginTable("Files", (int)std::size(sl),
		ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable);
	ims_window_drag w_ch(true);

	if (bv) {
		ImGui::TableSetupScrollFreeze(0, 1);
		for (let* c : sl)ImGui::TableSetupColumn(c);
		ImGui::TableHeadersRow();

		let ext = sp.get_ext();

		for (size_t i = 0; i < sp.m_list.size(); ++i) {
			let& sr = sp.m_list[i];
			//filter
			if (sr.type != file_dialog_state::file_type::DIR) {
				if (!ext.empty() && sr.ext != ext)
				{
					continue;
				}
				if (sp.m_type == file_dialog_state::type::folder &&
					!sp.m_show_files)
				{
					continue;
				}
			}


			ImGui::TableNextRow();

			////////////////////////////////////////////////////////////////////////
			ImGui::TableNextColumn();

			ImGui::PushID(next_id++);
			bool b = ImGui::Selectable(sr.ui_name.c_str(), sp.m_cur_sel == i,
				ImGuiSelectableFlags_SpanAllColumns) && w_ch.allow();
			ImGui::PopID();

			if (b) {
				if (sp.select_item(i)) {
					sp.m_cur_sel = i;
				} else {
					sp.m_cur_sel = ims_max;
				}
			}
			////////////////////////////////////////////////////////////////////

		}
		ImGui::EndTable();
	}
	////////////////////////////////////////////////////////////////////////////
	
}

bool ws_file_dialog::on_file_dlg_ok(bool can_overwrite)
{
	auto& sp = get_fds();

	std::string fn;

	let err = sp.check(fn);
	if (err.empty()) {
		if (file_dialog_state::type::save == sp.m_type) {
			
			if (!can_overwrite && ims_file::is_exists(fn))
			{
				ims_confirm_dlg(
					std::string("The file already exists. Do you want to overwrite it?\n") + fn, []
					{
						on_file_dlg_ok(true);
					});

				return true;
			}
			if (!sp.m_fw.open_stream(fn)) {
				ims_show_message(std::string("Could not save file:\n") + fn);
				return false;
			}
		}
		sp.call_cb_int(fn);
		sp.clear();//no error - close
	} else {
		ims_show_message(err);
	}

	return err.empty();
}

bool ws_file_dialog::close()
{
	auto& sp = get_fds();

	if (sp.empty3()) {
		return false;
	}
	sp.clear();
	return true;
}
