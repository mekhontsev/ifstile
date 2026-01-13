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
#include "file_dialog_state.h"
#include "ims_file.h"
#include "platform.h"

#ifdef _MSC_VER
#include "dirent/dirent.h"
#else
#include "dirent.h"
#endif


static bool rmtree(const char* path)
{
	DIR* dir;
	struct dirent* entry;

	// if not possible to read the directory for this user
	if ((dir = opendir(path)) == nullptr) {
		return false;
	}

	std::string full_path;

	// iteration through entries in the directory
	while ((entry = readdir(dir)) != nullptr) {

		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;// skip entries "." and ".."
		}

		full_path = std::string(path) + "/" + entry->d_name;

		if (entry->d_type == DT_DIR) {
			rmtree(full_path.c_str());
		}
		else {
			platform::remove_file(full_path.c_str());
		}
	}

	closedir(dir);

	// remove the devastated directory and close the object of it
	return platform::remove_empty_directory(path);
}


std::string file_dialog_state::get_ext() const
{
	if (m_ext.empty())return "";
	auto ret = m_ext[m_cur_ext_idx];
	if (ret == "*")ret.clear();
	return ret;
}

static bool IsDirectoryExist(const std::string& name)
{
	auto pDir = opendir(name.c_str());
	if (!pDir)return false;
	closedir(pDir);
	return true;
}


bool file_dialog_state::create_dir()
{
	if (m_name.empty() || !ims_file::get_extension(m_name).empty()) {
		return false;
	}

	let fn = get_new_dir_name();

	if (!IsDirectoryExist(fn)) {

		if (!platform::create_directory(fn.c_str())) {
			return false;
		};

		if (!IsDirectoryExist(fn)) {
			return false;//not created
		}
	}

	//go to the folder
	m_path.emplace_back(m_name);
	m_name.clear();
	refresh_list();
	return true;
}

bool file_dialog_state::remove_dir()
{
	if (!m_name.empty()) {
		if (!platform::remove_file((get_full_path() + m_name).c_str())) {
			return false;
		}
		refresh_list();
	}
	else {
		if (!rmtree(get_full_path().c_str())) {
			return false;
		}
		shrink_path(m_path.size() - 1);
	}
	return true;
}

void file_dialog_state::init(const STRLIST& ea)
{
	m_ext.clear();
	m_cur_ext_idx = 0;

	for (let& e : ea) {
		m_ext.emplace_back(e);
	}
}


bool file_dialog_state::shrink_path(size_t sz)
{
	if (sz > m_path.size())return false;

	auto* dir = opendir(get_full_path(sz).c_str());
	if (!dir)return false;
	closedir(dir);

	m_path.resize(sz);
	refresh_list();

	return true;
}


bool file_dialog_state::select_item(size_t idx)
{
	let& e = m_list[idx];
	if (e.type == file_type::DIR) {
		m_path.emplace_back(e.name);
		refresh_list();
		return false;
	}

	m_name = e.name;
	return true;
}

size_t file_dialog_state::refresh_list(size_t sz)
{
	if (sz == 0)sz = m_path.size();

	m_list.clear();

	let fn = get_full_path(sz);

#ifdef _MSC_VER
	if (ims_file::is_root(fn)) {

		std::array<char, 512> buf;
		buf[0] = 0;
		let res = GetLogicalDriveStringsA((DWORD)std::size(buf), buf.data());
		if (0 == res)return 0;

		for (char* c = buf.data(); *c; ++c) {
			auto& e = m_list.emplace_back();
			e.type = file_type::DIR;
			e.name = c;
			while (e.name.back() == '\\')e.name.pop_back();
			ims_file::adjust(e.name);
			e.ui_name = e.name;
			while (*c)++c;
		}
		return m_list.size();//all disks are folders
	}
#endif // _MSC_VER

	//use strcoll for sorting!!!
	struct dirent** files = nullptr;
	let n = scandir(fn.c_str(), &files, nullptr, alphasort);
	IMS_SCOPE([&] {free(files); });

	if (n < 0)return 0;

	size_t ret = 0;

	for (size_t i = 0; i < (size_t)n; ++i) {
		let* ent = files[i];
		if (strcmp(ent->d_name, ".") == 0)continue;
		if (strcmp(ent->d_name, "..") == 0)continue;

		auto& e = m_list.emplace_back();
		e.name = ent->d_name;


		switch (ent->d_type)
		{
		case DT_REG:
			e.type = file_type::REG;
			e.ext = ims_file::get_extension(e.name);
			break;
		case DT_DIR:
			++ret;
			e.type = file_type::DIR;
			break;
		case DT_LNK:
			e.type = file_type::LNK;
			break;
		default:
			e.type = file_type::UNK;
		}

		if (e.type == file_type::DIR) {
			e.ui_name = std::string("[") + e.name + "]";
		}
		else {
			e.ui_name = e.name;
		}
	}



	std::sort(m_list.begin(), m_list.end(), [](let& e1, let& e2) {
		if (e1.type == file_type::DIR && e2.type != file_type::DIR)return true;
		if (e1.type != file_type::DIR && e2.type == file_type::DIR)return false;

		let& s1 = e1.name;
		let& s2 = e2.name;

		//put files starting with '.' at the end
		if (!s1.empty() && !s2.empty()) {
			let c1 = s1.front();
			let c2 = s2.front();
			if (c1 != '.' && c2 == '.')return true;
			if (c1 == '.' && c2 != '.')return false;
			//normal comparison
		}

		return strcoll(s1.c_str(), s2.c_str()) < 0;
	});

	return ret;
}

void file_dialog_state::adjust_ext(std::string& p) const
{
	let ce = get_ext();
	if (ce.empty() || p.empty() || p.back() == '/')return;
	let e = ims_file::get_extension(p);
	if (e == ce)return;
	p = p.substr(0, p.length() - e.length());
	if (p.empty() || p.back() != '.')p += ".";
	p += ce;
}

void file_dialog_state::on_change_ext()
{
	adjust_name(m_name);//not necessary
	adjust_ext(m_name);
};

void file_dialog_state::adjust_name(std::string& p)
{
	p = p.c_str();//c_str is important to trim possible zeros
	for (auto& q : p) {
		if (q == '<' || q == '>')q = '_';
	}
	boost::algorithm::trim_if(p, boost::algorithm::is_any_of(" \t\r\n"));
}

void file_dialog_state::set_path(std::string_view par)
{
	std::string p{ par };
	if (p.empty()) {//take the last name
		p = get_full_name();
	}
	p = ims_file::adjust(p);

	if (type::folder == m_type && (p.empty() || p.back() != '/')) {
		p += '/';
	}



	////////////////////////////////////////////////////////////////////////////
	m_path.clear();
	std::string t;
	while (!ims_file::is_root(p)) {
		p = ims_file::get_parent(p, &t);
		adjust_name(t);
		if (!t.empty())m_path.emplace_back(t);
	};
	m_path.emplace_back("/");
	std::reverse(m_path.begin(), m_path.end());

	////////////////////////////////////////////////////////////////////////////

	if (type::folder == m_type) {
		m_name.clear();
	}
	else {
		while (m_path.size() > 1) {
			if (IsDirectoryExist(get_full_path()))break;
			m_name = m_path.back();
			m_path.pop_back();
		}

		//check the extension
		if (type::save == m_type && !m_ext.empty()) {
			adjust_ext(m_name);
		}
	};

	refresh_list();
}

std::string file_dialog_state::get_full_path(size_t sz)
{
	size_t i =
#ifdef _MSC_VER
		1;
#else
		0;
#endif // !_MSC_VER
	std::string ret;

	if (sz == 0)sz = m_path.size();

	for (; i < sz; ++i) {
		ret += m_path[i];
		if (i > 0)ret.push_back('/');
	}
	if (ret.empty())ret.push_back('/');
	return ret;
}


std::string file_dialog_state::get_new_dir_name()
{
	adjust_name(m_name);
	return get_full_path() + m_name;
}

std::string file_dialog_state::get_full_name()
{
	if (m_type == type::folder) {
		return get_full_path();
	}
	else {
		adjust_name(m_name);
		return get_full_path() + m_name;
	}
}

bool file_dialog_state::check_for_path()
{
	bool is_path = false;
	for (let c : m_name) {
		if (c == '\\' || c == '/') {
			is_path = true;
			break;
		}
	}
	if (!is_path)return false;

	set_path(m_name);
	return true;
}


void file_dialog_state::close_stream()
{
	m_fw.close();
}

void file_dialog_state::call_cb_int(const std::string& fn) const
{
	try {
		m_cb(fn);
	}
	catch (const std::exception& e) {
		ims_error("Error: {}", e.what());
	}
}

void file_dialog_state::scroll_list(bool up)
{
	if (m_list.empty())return;

	bool changed = false;

	if (m_cur_sel == ims_max) {
		m_cur_sel = 0;
		changed = true;
	}
	else if (up) {
		if (m_cur_sel > 0) {
			--m_cur_sel;
			changed = true;
		}
	}
	else {
		if (m_cur_sel + 1 < m_list.size()) {
			++m_cur_sel;
			changed = true;
		}
	}

	if (changed) {
		let& e = m_list[m_cur_sel];
		if (e.type != file_type::DIR) {
			m_name = e.name;
		}
	}
}

std::string file_dialog_state::check(std::string& fn)
{
	fn.clear();

	if (type::folder == m_type) {
		fn = get_full_path();
		if (!platform::is_folder_exists(fn)) {
			return std::string("The folder does not exists:\n") + fn;
		}
	}
	else {
		adjust_name(m_name);
		if (m_name.empty()) {
			return std::string("The file name is empty.");
		}

		adjust_ext(m_name);
		fn = get_full_name();

		if (type::open == m_type) {
			if (!ims_file::is_exists(fn)) {
				return std::string("The file does not exists:\n") + fn;
			}
		}
	};

	return "";
}

std::string file_dialog_state::call_cb()
{
	std::string fn;

	let err = check(fn);
	if (err.empty()) {
		call_cb_int(fn);
	}

	return err;
}

void file_dialog_state::save_file(
	const std::string& title,
	const std::string& def_path,
	uint8_t save_flags,
	const STRLIST& ext,
	CBTYPE cb)
{
	m_type = type::save;
	m_flags = save_flags;
	init(ext);
	set_path(def_path);
	m_title = title;
	m_cb = cb;
}


void file_dialog_state::open_file(
	const std::string& title,
	const std::string& def_path,
	const STRLIST& ext,
	CBTYPE cb)
{
	m_type = type::open;
	init(ext);
	set_path(def_path);
	m_title = title;
	m_cb = cb;
}

void file_dialog_state::select_folder(
	const std::string& title,
	const std::string& def_path,
	CBTYPE cb)
{
	m_type = type::folder;
	init({});
	set_path(def_path);
	m_title = title;
	m_cb = cb;
}
