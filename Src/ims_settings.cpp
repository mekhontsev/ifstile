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
#include "ims_settings.h"
#include "ims_file.h"
#include "ims_rw.h"
#include "samples.h"
#include "render_params.h"

#include "ims_ini.h"

#include "gui.h"//overkill


bool load_settings(const std::string& ini_filename)
{
	
	std::ifstream fs;
	ims_file::open(fs, ini_filename);
	if (fs.fail()) {
		return false;
	}

	ims_ini inifile;
	inifile.read(fs);

	auto& st = get_settings();
	auto& rend = get_rpars();


	auto& uiscale = get_ui_scale();
	inifile.get("UIScale", uiscale);
	uiscale = std::clamp(uiscale, ims_setting::min_ui_scale, ims_setting::max_ui_scale);
	

	auto& uialpha = st.m_window_alpha;
	inifile.get("UIAlpha", uialpha);
	uialpha = std::clamp(uialpha, ims_setting::min_ui_alpha, ims_setting::max_ui_alpha);

	auto& bgalpha = st.m_backgr_alpha;
	inifile.get("BGAlpha", bgalpha);
	bgalpha = std::clamp(bgalpha, ims_setting::min_bg_alpha, ims_setting::max_bg_alpha);

	inifile.get("UIStyle", st.m_dark);

	auto& thmb = st.m_max_thmb;
	inifile.get("Thumbnail", thmb);
	thmb = std::clamp(thmb, ims_setting::min_thumbnail, ims_setting::max_thumbnail);

	auto& rt = st.m_num_render_threads;
	inifile.get("RenderThreads", rt);
	rt = std::clamp(rt, size_t(1), ims_setting::max_threads());


	inifile.get("ShowMenu", st.m_show_menu);
	inifile.get("SelectCorner", st.m_select_fom_corner);
	inifile.get("PaneMode", (int&)st.m_window_mode);
	inifile.get("DockedSize", st.m_docked_size);

	////////////////////////////////////////////////////////////////////////////

	inifile.get("Quality", rend.m_quality);
	rend.adjust_quality();

	//g_ps.m_thickness = 1; inifile.get("Thickness", g_ps.m_thickness);
	inifile.get("Brightness", rend.m_brightness);
	inifile.get("Contrast", rend.m_contrast);
	inifile.get("Borders", rend.m_border_pow);
	inifile.get("ResolutionX", rend.m_resolution[0]);
	inifile.get("ResolutionY", rend.m_resolution[1]);

	

	inifile.get("AO_Radius", rend.m_ssao_rad_perc);
	inifile.get("AO_Amount", rend.m_ssao_density);
	inifile.get("AO_Samples", rend.m_ssao_samples);

	//inifile.get("Oversampling", rend.m_oversamp);

	auto& b = rend.m_background.data;

	inifile.get("BackgroundR", b[0]);
	inifile.get("BackgroundG", b[1]);
	inifile.get("BackgroundB", b[2]);
	//inifile.get("FogDensity", b[3]);//intentionally zero out

	////////////////////////////////////////////////////////////////////////////
	rend.m_mesh_colors = inifile.get("MeshColors", rend.m_mesh_colors);
	inifile.get("ResolutionMesh", rend.m_mesh_resolution);
	rend.adjust_mesh_resolution();

	////////////////////////////////////////////////////////////////////////////

	inifile.get("Folder", st.m_last_folder);
	inifile.get("FolderPicture", st.m_last_picture_folder);

	for (size_t i = 0;; ++i) {
		std::string val;
		let key = std::string("Recent") + std::to_string(i);
		if (!inifile.get(key, val)) {
			break;
		};
		get_samples().add_recent(val, false);
	}


	return true;
}

bool save_settings(const std::string& ini_filename)
{
	let& st = get_settings();
	let& rend = get_rpars();

	ims_ini inifile;

	inifile.put("UIScale", get_ui_scale());
	inifile.put("UIAlpha", st.m_window_alpha);
	inifile.put("BGAlpha", st.m_backgr_alpha);
	inifile.put("UIStyle", st.m_dark);

	inifile.put("Thumbnail", st.m_max_thmb);
	inifile.put("RenderThreads", st.m_num_render_threads);

	inifile.put("ShowMenu", st.m_show_menu);
	inifile.put("SelectCorner", st.m_select_fom_corner);

	inifile.put("PaneMode", (int)st.m_window_mode);
	inifile.put("DockedSize", st.m_docked_size);


	inifile.put("Quality", rend.m_quality);
	inifile.put("Thickness", rend.m_thickness);
	inifile.put("Brightness", rend.m_brightness);
	inifile.put("Contrast", rend.m_contrast);
	inifile.put("Borders", rend.m_border_pow);


	inifile.put("ResolutionX", rend.m_resolution[0]);
	inifile.put("ResolutionY", rend.m_resolution[1]);
	inifile.put("ResolutionMesh", rend.m_mesh_resolution);
	inifile.put("MeshColors", rend.m_mesh_colors);

	let& b = rend.m_background.data;
	inifile.put("BackgroundR", b[0]);
	inifile.put("BackgroundG", b[1]);
	inifile.put("BackgroundB", b[2]);
	inifile.put("FogDensity", b[3]);

	inifile.put("AO_Radius", rend.m_ssao_rad_perc);
	inifile.put("AO_Amount", rend.m_ssao_density);
	inifile.put("AO_Samples", rend.m_ssao_samples);

	////////////////////////////////////////////////////////////////////////////

	inifile.puts("Folder", st.m_last_folder);
	inifile.puts("FolderPicture", st.m_last_picture_folder);

	let& rcat = get_samples().get_recent().smp;
	for (size_t i = 0; i < rcat.size(); ++i) {
		inifile.puts(std::string("Recent") + std::to_string(i), rcat[i].path);
	};


	std::ofstream str;
	ims_file::open(str, ini_filename);
	if (!str.is_open()) {
		ims_error("Write ini error: {}", ini_filename);
		return false;
	}

	inifile.write(str);

	return true;
};



size_t ims_setting::max_threads()
{
	let ret = std::thread::hardware_concurrency();
	return ret > 0 ? ret : 8;
}
