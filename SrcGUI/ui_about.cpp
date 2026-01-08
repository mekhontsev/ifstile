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
#include "ui_about.h"
#include "ims_markdown.h"
#include "gui.h"
#include "version.h"
#include "quickjs.h"
#include "miniz.h"

const char* ws_about::get_title()
{
	return "About";
}


void ws_about::show_markdown(std::string_view msg) 
{
	let s = get_ui_scale();
	let& ds = ImGui::GetIO().DisplaySize;
	ImVec2 ms{
		std::min(ds.x - 30 * s, 190 * s),
		s*180
	};

	ImGui::BeginChild("MD", ms);

	markdown(msg);

	ImGui::EndChild();
}


static std::string libs;

void ws_about::show()
{
	ImGui::OpenPopup(get_title());

	ImGui::SetNextWindowPos(
		ImGui::GetMainViewport()->GetCenter(),
		ImGuiCond_Always, 
		ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal(get_title(), nullptr, 
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		return;
	}
	IMS_SCOPE([] {ImGui::EndPopup(); });

	



	{
		
		if (!ImGui::BeginTabBar("About"))return;
		IMS_SCOPE([] { ImGui::EndTabBar(); });

		if (ImGui::BeginTabItem("Version"))
		{
			constexpr std::string_view msg =
				"##### " APPLICATION_TITLE " v" PROJECT_VERSION


#ifdef DEVELOPER_VERSION
				" DEV"
#endif
				"\n"
				COPYRIGHT
				"\n"
				APPLICATION_COMPANY "\n"
				"[" APPLICATION_SITE "](" APPLICATION_SITE ")\n"
				"[" SUPPORT_EMAIL "](mailto:" SUPPORT_EMAIL ")\n"
				//__DATE__ "\n"
				;

			show_markdown(msg);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Libs"))
		{
			if (libs.empty()) {

				libs = 
					"SDL " \
					BOOST_PP_STRINGIZE(SDL_MAJOR_VERSION) "."\
					BOOST_PP_STRINGIZE(SDL_MINOR_VERSION) "."\
					BOOST_PP_STRINGIZE(SDL_MICRO_VERSION) "\n"
					"Eigen "\
					BOOST_PP_STRINGIZE(EIGEN_WORLD_VERSION) "."\
					BOOST_PP_STRINGIZE(EIGEN_MAJOR_VERSION) "."\
					BOOST_PP_STRINGIZE(EIGEN_MINOR_VERSION) "\n"
					"QuickJS-NG "\
					BOOST_PP_STRINGIZE(QJS_VERSION_MAJOR) "."\
					BOOST_PP_STRINGIZE(QJS_VERSION_MINOR) "."\
					BOOST_PP_STRINGIZE(QJS_VERSION_PATCH) "\n";

				fmt::format_to(std::back_inserter(libs), "Boost {}.{}.{}\n", 
					BOOST_VERSION / 100000,
					(BOOST_VERSION / 100) % 1000,
					BOOST_VERSION % 100);

				fmt::format_to(std::back_inserter(libs), "Dear ImGui {}.{}.{}\n",
					IMGUI_VERSION_NUM / 10000,
					(IMGUI_VERSION_NUM / 100) % 100,
					IMGUI_VERSION_NUM % 100);

#ifdef __EMSCRIPTEN__				
				libs += "Emscripten "\
					BOOST_PP_STRINGIZE(__EMSCRIPTEN_major__) "."\
					BOOST_PP_STRINGIZE(__EMSCRIPTEN_minor__) "."\
					BOOST_PP_STRINGIZE(__EMSCRIPTEN_tiny__) "\n";
#endif	
				libs += "Miniz " MZ_VERSION "\n";
				libs += "MD4C 0.5.2\n";
				libs += "cute_png 1.05\n";
				libs += "imgui_md 1.00\n";
				libs += "TinyEXR 1.0.12\n";
				libs += "Fork Awesome 1.2.0\n";
#ifdef _MSC_VER	
				libs += "Dirent for MSVC 1.0.12\n";
				
#endif
				
				
				libs += "unordered_dense "\
					BOOST_PP_STRINGIZE(ANKERL_UNORDERED_DENSE_VERSION_MAJOR) "."\
					BOOST_PP_STRINGIZE(ANKERL_UNORDERED_DENSE_VERSION_MINOR) "."\
					BOOST_PP_STRINGIZE(ANKERL_UNORDERED_DENSE_VERSION_PATCH) "\n";

	
				libs += "\n";
				libs += BOOST_COMPILER;
#ifndef _IMS_64_
				libs += " (32 bit)";
#endif
				libs += "\n";
				
			}

			show_markdown(libs);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("License"))
		{
	
			show_markdown(
				
				R"(
IFStile is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or at your option any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see [ www.gnu.org/licenses/ ]( https://www.gnu.org/licenses/ )
)");
			
			ImGui::EndTabItem();
		}

		
	}

	ImGui::Separator();

	if (ImGui::Button("   OK   ")) {
		ImGui::CloseCurrentPopup();
		m_show = false;
	}
}

bool ws_about::close()
{
	if (!m_show) {
		return false;
	}
	m_show = false;
	redraw_gui(1);
	return true;
}
