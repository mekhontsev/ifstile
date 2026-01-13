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

#include "ims_rw.h"
#include "save_type.h"

struct file_dialog_state
{
	using CBTYPE = std::function<void(const std::string& filename)>;

	using STRLIST = std::initializer_list<std::string_view>;

	std::string m_title;

	std::vector<std::string> m_ext;
	int m_cur_ext_idx;

	CBTYPE m_cb;

	//splitted path
	std::vector<std::string> m_path;
	std::string m_name;//just file name (empty for folder)

	std::string get_ext() const;

	//Selecting elements to save
	//single=0,	checked=1,	all=2
	uint8_t m_flags;//allowed save types for the list
	save_type m_save_type = save_type::all;//what the user chose

	std::string m_filename_single;
	std::string m_filename_all;


	enum class file_type
	{
		DIR,
		REG,
		LNK,
		UNK,
	};

	struct dir_entry 
	{
		std::string name;
		std::string ui_name;
		std::string ext;
		file_type type;
	};

	std::vector<dir_entry> m_list;
	size_t m_cur_sel = ims_max;

	bool m_show_files = false;//show files in folder selection mode


	enum class type 
	{
		open=0,
		save,
		folder
	};
	type m_type;


	ims_writer m_fw;

	////////////////////////////////////////////////////////////////////////////
	bool empty3() const { return m_cb == nullptr; };
	void clear() { m_cb = nullptr; };
	//create a directory and enter it
	bool create_dir();
	//delete a file or delete a directory and exit it
	bool remove_dir();

	void init(const STRLIST& ea);

	//if the length is equal to the current one, then just refresh
	//permissions to the parent folder may not be available, in which case, do nothing
	bool shrink_path(size_t sz);
	//returns true if you did NOT fall into the folder
	bool select_item(size_t idx);
	//returns the number of folders in the list
	size_t refresh_list(size_t sz=0);

	void adjust_ext(std::string& p) const;
	void on_change_ext();
	static void adjust_name(std::string& p);
	//returns an error or an empty string
	std::string call_cb();

	void save_file(
		const std::string& title,
		const std::string& def_path,
		uint8_t save_flags,
		const STRLIST& ext,
		CBTYPE cb);

	void open_file(
		const std::string& title, 
		const std::string& def_path, 
		const STRLIST& ext, 
		CBTYPE cb);

	void select_folder(
		const std::string& title,
		const std::string& def_path,
		CBTYPE cb);

#if defined(__EMSCRIPTEN__)
	bool m_download = true;//load on save
#endif



	std::string get_full_name();

	void set_path(std::string_view p);

	//if the user passed a path in the name field, we follow this path
	bool check_for_path();

	void close_stream();

	void call_cb_int(const std::string& fn) const;

	void scroll_list(bool up);

	//if sz>0 then only part is returned
	std::string get_full_path(size_t sz = 0);

	//get the name of the directory to create
	std::string get_new_dir_name();

	std::string check(std::string& fn);
	
	
};
