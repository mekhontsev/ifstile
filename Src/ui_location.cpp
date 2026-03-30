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
#include "ui_location.h"
#include "gui.h"
#include "ims_window_drag.h"

#include "build_data.h"
#include "def_number_types.h"
#include "call_thread.h"

const char* ws_location::get_title()
{
	return "Location";
}


static void randomize_section(subspace_info<DefNumTypes::Real>& si)
{
	auto& b = si.basis_user;
	std::uniform_real_distribution<double> distr(-1, 1);
	auto& rng = ims_random::get().rng;
	for (int c = 0; c < b.cols(); ++c) {
		for (int r = 0; r < b.rows(); ++r) {
			b(r, c) = distr(rng);
		}
	}
}
void ws_location::on_change(standard_vars& sv, bool reset, bool c, bool si_ch)
{
	if (!si_ch && !reset && !c)return;

	//copy m_si and m_cam, otherwise it might return before the callback is called
	stop_build_then([&sv, reset, c, si_ch, si = m_state.m_si, xcam = m_state.m_xcam]() {
		if (reset) {
			sv.m_si_empty = true;
			sv.m_xcam2.m_2d_empty = true;
			sv.m_xcam2.m_3d_empty = true;
			sv.m_xcam2.m_sd.clear2();
			
		} else {
			if (si_ch) {
				sv.m_si2 = si;
				sv.m_si2.init_si();
			}
			if (c) {
				sv.m_xcam2 = xcam;
				sv.m_xcam2.m_camera.init();
			}
		}

		do_rebuild_sync();
	});
}

void ws_location::show()
{
	auto* xd = get_global_bd();
	if (!xd || !xd->m_bi.exists())return;

	auto& sv = xd->m_special;

	let dim_set = m_state.m_si.get_dim_space();
	let rd = m_state.m_si.get_section_dim();

	//copy, but memory is not allocated on each redraw
	Eigen::Vector3d tar;
	if (m_cliked_state == cliked_state::wait_point) {
		if (rd < 3) {
			tar[0] = m_state.m_xcam.m_sd.c[0];
			tar[1] = m_state.m_xcam.m_sd.c[1];
		} else {
			tar = m_state.m_xcam.m_camera.m_ref;
		}
		
	}
	m_state.m_xcam = sv.m_xcam2;//copying itself

	if (m_cliked_state == cliked_state::wait_point) {
		if (rd < 3) {
			m_state.m_xcam.m_sd.c[0] = tar[0];
			m_state.m_xcam.m_sd.c[1] = tar[1];
		} else {
			m_state.m_xcam.m_camera.m_ref = tar;
		}
	}
	m_state.m_si = sv.m_si2;

	

	

	ims_window_drag w_ch(false);

	bool reset = false;
	bool si_ch = false;
	bool c = false;

	IMS_SCOPE([&] {
		on_change(sv, reset, c, si_ch);
	});

	int next_id = 0;

	static int e = 0;
	{
		ImGui::PushID(next_id++);
		ImGui::PushItemWidth(80.0f * get_ui_scale());
		ImGui::Combo("", &e, "Camera\0Section\0\0");
		ImGui::PopID();
	}

	{
		SAME_LINE();
		ImGui::PushID(next_id++);
		if (ImGui::Checkbox("2D", &m_force2D))
		{
			if (m_force2D && rd == 3) {
				//switch to 2D
				reset = true;
				si_ch = true;
			} else if (!m_force2D && rd == 2 && dim_set > 2) {
				//switch to 3D
				reset = true;
				si_ch = true;
			}

		};

		set_tooltip("Build a 2D section for sets with dim >= 3");
		ImGui::PopID();
	}

	////////////////////////////////////////////////////////////////////////////
	{
		SAME_LINE();
		if (ims_button("Reset", "Reset camera and section", &next_id)) {
			reset = true;
			si_ch = true;
		};
	}

	{
		SAME_LINE();
		if (ims_button("Save", "Save current location to the memory slot.", &next_id)) {
			m_state_saved = m_state;
		};
	}

	{
		SAME_LINE();
		if (ims_button("Load", "Load saved location from the memory slot.", &next_id)) {
			m_state = m_state_saved;
			si_ch = true;
			c = true;
		};
	}

	////////////////////////////////////////////////////////////////////////////
	let& s = ImGui::GetStyle();
	let ws = ImGui::GetWindowSize().x - s.ScrollbarSize - s.FramePadding.x*2;

	ims_width iws(ws - 20 * get_ui_scale());

	if (e == 1) {
		ImGui::TextUnformatted("Section Origin");

		for (size_t i = 0; i < dim_set; ++i) {
			ImGui::PushID(next_id++);
			si_ch = input_double2(m_state.m_si.origin(i)) || si_ch;
			ImGui::PopID();
		}

		if (dim_set > 1) {

			static int s_xy = 0;

			ImGui::TextUnformatted("Section Basis");

			////////////////////////////////////////////////////////////////////////////
			//ImGui::SameLine();
			ImGui::PushID(next_id++);
			ImGui::RadioButton("X", &s_xy, 0);
			ImGui::PopID();

			ImGui::SameLine();
			ImGui::PushID(next_id++);
			ImGui::RadioButton("Y", &s_xy, 1);
			ImGui::PopID();

			if (rd == 3) {
				ImGui::SameLine();
				ImGui::PushID(next_id++);
				ImGui::RadioButton("Z", &s_xy, 2);
				ImGui::PopID();
			}

			if (dim_set > 2 && dim_set > rd) {
				SAME_LINE();
				if (ims_button("Rand", nullptr, &next_id)) {
					randomize_section(m_state.m_si);
					si_ch = true;
				};
			}


			if (s_xy >= (int)dim_set) {
				s_xy = 0;
			}

			for (size_t i = 0; i < dim_set; ++i) {
				ImGui::PushID(next_id++);
				si_ch = input_double2(m_state.m_si.basis_user(i, s_xy)) || si_ch;
				ImGui::PopID();
			}

		}

	
		return;
	}

	
	////////////////////////////////////////////////////////////////////////////
	if (is_thumb_enabled()) {
		return;
	}
	////////////////////////////////////////////////////////////////////////////

	if (rd <= 2) {
		ImGui::TextUnformatted("Screen");
		ImGui::SameLine();

		let wt = m_cliked_state == cliked_state::wait_point;
		if (ims_button(wt ? "Cancel" : "Pick",
			"Get point by mouse pointer", &next_id))
		{
			m_cliked_state = wt ? cliked_state::idle : cliked_state::wait_point;
		};

		
		auto& sd = m_state.m_xcam.m_sd;


		if (m_cliked_state == cliked_state::point_ready) {
			m_cliked_state = cliked_state::idle;
			
			sd.c[0] = m_p[0][0];
			sd.c[1] = m_p[0][1];
			c = true;
		}

		double m = 2 * sd.r;
		ImGui::PushID(next_id++);
		c = input_double2(sd.c[0], sd.c[0] - m, sd.c[0] + m, "x") || c;
		ImGui::PopID();

		if (rd == 2) {
			ImGui::PushID(next_id++);
			c = input_double2(sd.c[1], sd.c[1] - m, sd.c[1] + m, "y") || c;
			ImGui::PopID();
		}
		
		ImGui::PushID(next_id++);
		c = input_double2(sd.r, 0, 0, "r") || c;
		ImGui::PopID();

	
		if (rd == 2) {
			ImGui::PushID(next_id++);
			c = input_double2(sd.a, sd.a - 90, sd.a + 90, "a") || c;
			ImGui::PopID();
		}

		if (m_cliked_state == cliked_state::wait_point) {
			ImGui::Text("s=%f", m_square);
		}
	
	} else if (rd == 3) {

		auto& cam7 = m_state.m_xcam.m_camera;
		Eigen::Vector3d rl = cam7.m_ref - cam7.m_loc;
		
		if (m_cliked_state == cliked_state::point_ready) {
			m_cliked_state = cliked_state::idle;

			const Eigen::Vector3d q = m_p[0];
			if (m_state.m_lock_dist_target) {
				Eigen::Vector3d dir = m_p[0] - cam7.m_loc;
				dir.normalize();				
				cam7.m_loc = q - rl.norm() * dir;
			}
			cam7.m_ref = q;
			c = true;
			
			
#if 0
			rl.normalize();
			rl *=  rl.dot(m_p[0] - cam7.m_loc);
			cam7.m_ref = cam7.m_loc + rl;
			//do not rebuild
			sv.m_xcam.m_camera.m_ref = cam7.m_ref;
#endif

		}


		ImGui::TextUnformatted("Camera");
		ImGui::SameLine();

		ImGui::PushID(next_id++);
		ImGui::Checkbox("Fly mode", &m_state.m_lock_dist_target);
		ImGui::PopID();
		set_tooltip("The distance to the target is locked in the 'Fly' mode.");
	

		ImGui::PushID(next_id++);
		c = input_double2(cam7.m_loc[0]) || c;
		ImGui::PopID();
		ImGui::PushID(next_id++);
		c = input_double2(cam7.m_loc[1]) || c;
		ImGui::PopID();
		ImGui::PushID(next_id++);
		c = input_double2(cam7.m_loc[2]) || c;
		ImGui::PopID();

		ImGui::TextUnformatted(m_state.m_lock_dist_target ? "Sensitivity" : "Distance to the target");

		{
			ImGui::SameLine();


			ImGui::PushID(next_id++);
			bool ret = ImGui::Button("Move");
			ImGui::PopID();
			let drag_active = ImGui::IsItemActive();
			let drag_clicked = ImGui::IsItemClicked(0);
			set_tooltip(m_state.m_lock_dist_target ? "Drag to move camera" : "Drag to move target");
			if (ret) {

			}

			if (drag_active) {
				static Eigen::Vector3d start_loc;
				static double start_dist;

				if (drag_clicked) {
					start_loc = cam7.m_loc;
					start_dist = (cam7.m_loc - cam7.m_ref).norm();
				}

				if (ImGui::IsMouseDragging(0)) {
					let mp = ImGui::GetMousePos();
					if (ImGui::IsMousePosValid(&mp)) {

						let shift = start_dist * m_state.m_zt * ImGui::GetMouseDragDelta(0).x /
							ImGui::GetWindowWidth();

						Eigen::Vector3d lr;
						lr = cam7.m_ref - cam7.m_loc;
						lr.normalize();

						if (m_state.m_lock_dist_target) {
							cam7.m_loc = start_loc + shift * lr;
							cam7.m_ref = cam7.m_loc + lr * start_dist;
						} else {
							cam7.m_ref = cam7.m_loc + lr * (start_dist + shift);
						}


						c = true;
					}
				}

			}
		}
	
		ImGui::PushID(next_id++);
		{
			let old_d = m_state.m_lock_dist_target ? m_state.m_zt : rl.norm();

			double d = old_d;
			let changed = input_double2(d, 0, 0, nullptr,
				m_state.m_lock_dist_target?1:20);//TODO: magic, in theory, should be equal

			if (d == 0)d = old_d;
			if (changed) {
				
				if (m_state.m_lock_dist_target) {
					m_state.m_zt = d;
				} else {
					rl.normalize();
					cam7.m_loc = cam7.m_ref - d * rl;
					c = changed || c;
				}
			}

		}
		ImGui::PopID();

		ImGui::TextUnformatted("Target");
		ImGui::SameLine();
		
		let wt = m_cliked_state == cliked_state::wait_point;
		if (ims_button(wt ?"Cancel": "Pick",
			"Get point by mouse pointer", &next_id)) 
		{
			if (wt) {
				m_cliked_state = cliked_state::idle;
			} else {
				m_cliked_state = cliked_state::wait_point;
			}
		};
		

#if 0
		ImGui::SameLine();
		if (ims_button("<1", "Set target to the last picked point", &next_id)) {
			cam7.m_ref = m_p[0];
			c = true;
		};

		ImGui::SameLine();
		if (ims_button("<2", "Set the target to the middle of the last two picked points.", &next_id)) {
			cam7.m_ref = (m_p[0] + m_p[1]) / 2;
			c = true;
		};
#endif

		ImGui::PushID(next_id++);
		c = input_double2(cam7.m_ref[0]) || c;
		ImGui::PopID();
		ImGui::PushID(next_id++);
		c = input_double2(cam7.m_ref[1]) || c;
		ImGui::PopID();
		ImGui::PushID(next_id++);
		c = input_double2(cam7.m_ref[2]) || c;
		ImGui::PopID();

		

		ImGui::TextUnformatted("Vertical");
		ImGui::SameLine();

		ImGui::PushID(next_id++);
		bool ret = ImGui::Button("Roll");
		ImGui::PopID();
		let drag_active = ImGui::IsItemActive();
		let drag_clicked = ImGui::IsItemClicked(0);
		set_tooltip("Click to orthonormalize, drag to rotate");
		if (ret) {
			cam7.m_ver = cam7.adjusted_ver();
			c = true;
		}

		if (drag_active) {
			static Eigen::Vector3d start_ver;

			if (drag_clicked) {
				start_ver = cam7.adjusted_ver();
			}

			if (ImGui::IsMouseDragging(0)) {
				let mp = ImGui::GetMousePos();
				if (ImGui::IsMousePosValid(&mp)) {
				
					let ang = ImGui::GetMouseDragDelta(0).x / ImGui::GetWindowWidth()
						* 2 * boost::math::constants::pi<double>();

					Eigen::Vector3d lr, a;

					lr = cam7.m_loc - cam7.m_ref;
					lr.normalize();

					a = start_ver.cross(lr);

					let cs = cos(ang);
					let sn = sin(ang);

					cam7.m_ver = cs * start_ver - sn * a;
					c = true;
				}
			}
	
		}

		auto ver_ctl = [&](size_t idx)
		{
			ImGui::PushID(next_id++);
			bool ret = input_double2(cam7.m_ver[idx], -1, 1);
			ImGui::PopID();
			return ret;
		};
		c = ver_ctl(0) || c;
		c = ver_ctl(1) || c;
		c = ver_ctl(2) || c;

	
		ImGui::TextUnformatted("FOV");
		const double fov_lim[2] = { 0,180 };
		ImGui::PushID(next_id++);
		c = ImGui::DragScalar("", ImGuiDataType_Double, &cam7.m_fov, 0.3f,
			&fov_lim[0], & fov_lim[1], "%.16f",
			ImGuiSliderFlags_Logarithmic) || c;
		ImGui::PopID();

	}
}

void ws_location::from_mouse(const camera_ex& xc, std::string& status, Eigen::Vector3d p, bool clicked, bool is2d)
{
	if (!is2d) {
		let& c = xc.m_camera;
		Eigen::Vector3d rl = c.m_ref - c.m_loc;
		let rln2 = rl.squaredNorm();
		double zt = rl.dot(p - c.m_loc)/ rln2;

		fmt::format_to(std::back_inserter(status), " zt={}", zt);
		if (clicked && m_state.m_lock_dist_target) {
			m_state.m_zt = zt;
		}
	}

	if (m_cliked_state != cliked_state::wait_point) {
		return;
	}

	if (is2d) {
		m_state.m_xcam.m_sd.c[0] = p[0];
		m_state.m_xcam.m_sd.c[1] = p[1];
		m_square = p[2];
	} else {
		m_state.m_xcam.m_camera.m_ref = p;
	}

	if (!clicked) {
		return;
	}

	m_p[1] = m_p[0];
	m_p[0] = p;

	m_cliked_state = cliked_state::point_ready;
}
