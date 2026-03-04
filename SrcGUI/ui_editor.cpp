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
#include "ui_editor.h"
#include "ims_window_drag.h"
#include "variator.h"
#include "build_data.h"
#include "finder.h"
#include "gui.h"
#include "render_params.h"

#include "block_class.h"
#include "oper_block.h"
#include "env_block.h"
#include "ifs_list.h"
#include "eval_context.h"
#include "variable.h"


const char* ws_editor::get_title()
{
	return "Editor";
}


//TODO: get rid of a function
static bool has_childs(const oper_block* b) 
{
	let& lst = ifs_list_get();
	for (let id : lst.m_blocks) {
		let* u = lst.get_block(id);
		for (let* p = u->get_parent(); p; p = p->get_parent()) {
			if (p == b)return true;
		}
	}
	return false;
}
void ws_editor::show()
{

	if (is_batch_in_progress()) {
		ImGui::TextUnformatted("Editor is not available during batch rendering...");
		return;
	}
	
	////////////////////////////////////////////////////////////////////////


	int id = 0;

	static bool controls_advanced = false;

	ImGui::PushID(id++);
	ImGui::PushItemWidth(90 * get_ui_scale());
	static constexpr auto items = { 
		"Controls","Special", "Source", "JS", "@IFStile"};
	ImGui::Combo("", &m_eidt_type, items.begin(), (int)items.size());
	ImGui::PopItemWidth();
	ImGui::PopID();


	if (m_eidt_type == EDITOR_IFSTILE) {
	

		ImGui::Checkbox("Finder", &env_block_data::s_use_fparams);
		ImGui::Checkbox("Rendering", &env_block_data::s_use_rparams);

		{
			if (ims_button("Save", "Save parameters to the @IFSTile block")) {
				save_env_block(false, 
					env_block_data::s_use_fparams,
					env_block_data::s_use_rparams);
			}
		}

		{
			SAME_LINE();
			if (ims_button("Load", "Load parameters from the @IFSTile block")) {

				load_env_block_ex(
					env_block_data::s_use_fparams, 
					env_block_data::s_use_rparams);
			}
		}

	
		return;
	}

	static bool do_apply = false;

	{
		SAME_LINE();
		if (ims_button("Apply", "Write changes back to original block", &id)) {
			if (m_eidt_type == EDITOR_SOURCE || m_eidt_type == EDITOR_JS) {
				m_source_editor.apply(m_eidt_type == EDITOR_JS);
			} else if (editor_ready()) {
				do_apply = true;
			}
		}
	}

	
	
	if (m_eidt_type == EDITOR_SOURCE || m_eidt_type == EDITOR_JS) {
		SAME_LINE();
		m_source_editor.show(id, m_eidt_type == EDITOR_JS);
		return;
	}



	if (!editor_ready()) {
		ImGui::TextUnformatted("Not available in thumbnail mode...");
		return;
	}

	////////////////////////////////////////////////////////////////////////

	auto* xd = get_global_bd();
	if (!xd || xd->empty()) return;

	let* src = xd->get_direct(ifs_object_type::normal);

	auto& sv = xd->m_special;

	if (do_apply) {
		do_apply = false;

		//carefully overwrite the current block
		
		auto* cb = get_cur_block();
	
		if (cb && xd->m_bi.exists()) {
			
		
			let* msg = "There are dependent blocks.\nChanges will be applied to them too.";
			ims_confirm_dlg(msg, [cb, &sv, src]() {
				
				stop_build_then([cb, &sv, src]() {

					let* actual_par = src->get_parent();

					//fix parent
					if (actual_par == src->m_js_parent.get()) {
						actual_par = actual_par->get_parent();
					}

					if (actual_par->m_ops.empty()) {
						actual_par = actual_par->get_parent();
					}

					//copy the bytecode
					if (actual_par == cb->get_parent()) {
						cb->m_first_var = src->m_first_var;
						cb->m_ops = src->m_ops;
					}

					sv.sync_builtins(false, *cb);
					set_block_direct(cb);
					do_rebuild_sync();
				});
			}, has_childs(cb));
		} else {
			show_generic_error_msg();
		}

	}


	{
		SAME_LINE ();

		ImGui::BeginDisabled(xd->m_normal_parent.get());
		IMS_SCOPE([] {ImGui::EndDisabled(); });

		if (ims_button("+Block", "Save changes as a new block", &id)) {
			let* b = xd->m_block_sq.get();
			auto nb = std::make_unique<oper_block>();
			b->simple_copy(*nb);
			nb->fix_js_parent();
			let* p = nb->get_parent();
			if (p && p->m_ops.empty()) {
				nb->set_parent(p->get_parent());
			}
			sv.sync_builtins(false, *nb);
			add_block2(nb, "");
		}
	}

	if (m_eidt_type == EDITOR_CONTROLS) {
		{
			SAME_LINE();
			ImGui::PushID(id++);
			ImGui::Checkbox("", &controls_advanced);
			ImGui::PopID();
			set_tooltip("Advanced");
		}


		SAME_LINE();
		if (ims_button("Rand", "CTRL-U", &id)) {
			rand_current_set();
		}
	}

	

	ImGui::Separator();


	if (m_eidt_type == EDITOR_BUILTIN) {

		for (size_t i = 0; i < c_num_builtins; ++i) {
			let bid = builtin_ids(i);
			if (bid == builtin_ids::subspace || bid == builtin_ids::light) {
				continue;
			}
			auto v = sv.chas_builtin(bid);
			std::string name{ get_builtin_name(bid) };
			if (ImGui::Checkbox(name.c_str(), &v)) {

				if (bid == builtin_ids::camera ||
					bid == builtin_ids::root ||
					bid == builtin_ids::section)
				{
					sv.m_has_builtin[(size_t)bid] = v;
				} else {
					//needs to be rebuilt
					stop_build_then([&sv, bid, v]() {
						if (v) {
							let& rend = get_rpars();
							if (bid == builtin_ids::palette) {
								//a palette has appeared, we take the global one
								sv.m_pal = rend.m_palette;
							} if (bid == builtin_ids::background) {
								//the background appeared, we take the global one
								sv.m_bac = rend.m_background;
							} else if (bid == builtin_ids::colorize) {
								//a coloring appeared, we take the global one
								sv.m_colorize = rend.m_colorize;
							}
						}

						sv.m_has_builtin[(size_t)bid] = v;
						on_change_color();
						});
				}
			}
		}

		return;//finish here
	}


	//for those operators that are defined directly
	//information about the value of each control element is known in the block


	if (xd->m_normal_parent) {
		ImGui::TextUnformatted("Controls are not available in boundary mode...");
		return;
	}

	ImGui::BeginChild("Controls");
	ims_window_drag w_ch(false);

	//this is a copy of the object from the list
	auto& sr = xd->get_block();

	if (!sr.get_parent()) {
		return;
	}
	let* e = sr.get_parent()->ctx();
	auto* g = sr.get_class();

	m_mut.clear();
	for (let& q : sr) {
		if (!q.is_builtin() && e->m_refs5[q.gr()].is_var()) {
			m_mut.emplace_back(mut_data{ q.gr(), q.pos5 });
		}
	}

	for(let& em: m_mut){

		auto& cref = g->m_refs[em.ref];
		let* com = sr.get_comment(em.ref);

		{
			ImGui::PushID(id++);
			IMS_SCOPE([] {ImGui::PopID(); });
			std::string name{ g->get_var_name(em.ref) };
			ImGui::TextUnformatted(name.c_str());
			if (com)set_tooltip(com->c_str());
		}


		auto& q = sr.m_ops[em.pos].hdr;

		ImGui::SameLine(40 * get_ui_scale());

		if (controls_advanced) {

			{
				
				ImGui::PushID(id++);
				bool b = cref.var_is_locked;
				if (ImGui::Checkbox("", &b) && w_ch.allow()) {
					cref.var_is_locked = b;
				};
				set_tooltip("Locked");
				ImGui::PopID();
			}
	
			{
				ImGui::SameLine();
				ImGui::PushID(id++);
				bool b = cref.can_be_empty;
				if (ImGui::Checkbox("", &b) && w_ch.allow()) {
					cref.can_be_empty = b;
				};
				set_tooltip("Can be empty when randomized");
				ImGui::PopID();
			}


			bool is_empty = q.is_xempty();

			ImGui::SameLine();
			ImGui::PushID(id++);
			bool ch = ImGui::Checkbox("", &is_empty) && w_ch.allow();
			set_tooltip("Empty");
			ImGui::PopID();

			if (ch) {
				if (!is_empty) {
					//create a random one from the graph
					static control_values cv;
					cv.data.clear();
					sr.insert_op_ex(em.pos, e->m_refs5[em.ref].c, *e, &cv);
					let& vp = finder::get().m_var_par;
					sr.apply_templates(cv, vp, nullptr);
				} else {
					q.set_xempty();
				}

				sr.m_flags.ready = false;
		

				stop_build_then([]() {
					ui_update_maps();
					do_rebuild_sync();
				});
			};

			ImGui::SameLine();
			ImGui::TextUnformatted(":");
			ImGui::SameLine();
		
		}

		let control_pos = ImGui::GetCursorPosX();

		bool has_ctls = false;
		IMS_SCOPE([&] {if(!has_ctls)ImGui::NewLine(); });
		///////////////////////////////////////////////////////////////////////

		auto ctl_ptr = e->m_refs5[em.ref].c;

		while (ctl_ptr.h.tt == ETYPE::reference) {
			ctl_ptr = e->m_refs5[ctl_ptr.h.get_offset()].c;
		}

		distrib_info di{};
		if (ctl_ptr.h.tt != ETYPE::set_permutation) {
			if (!sr.get_distrib(di, ctl_ptr)) {
				continue;
			}
		}

		if (ctl_ptr.h.tt == ETYPE::set_permutation &&
			q.tt == ETYPE::vector_imm && q.ts == ESUBTYPE::integer && q.num_args()>0)
		{
			let sz = q.num_args();

			auto* p = &sr.m_ops[q.get_offset()].i64;

			{
				ImGui::PushID(id++);
				IMS_SCOPE([] {ImGui::PopID(); });

				SAME_LINE();
				if (ImGui::Button("<")) {
					std::prev_permutation(&p[0], &p[sz]);
				}
			}
			{
				ImGui::PushID(id++);
				IMS_SCOPE([] {ImGui::PopID(); });
				SAME_LINE();
				if (ImGui::Button(">")) {
					std::next_permutation(&p[0], &p[sz]);
				}
			}

			std::string label;
			static size_t s_last_pushed = ims_max;
			if (s_last_pushed >= sz)s_last_pushed = ims_max;

			static std::vector<line_helper> lh;
			lh.resize(sz);

			for (size_t i = 0; i < sz; ++i) {
				label.clear();
				fmt::format_to(std::back_inserter(label), "{}", p[i]);
				let& fg_color = ImGui::GetStyle().Colors[
					s_last_pushed != i ? ImGuiCol_Text : ImGuiCol_TextDisabled];
				ImGui::PushStyleColor(ImGuiCol_Text, fg_color);
				lh[i].begin();
				ImGui::PushID(id++);
				if (ImGui::Button(label.c_str())) {
					if (s_last_pushed == ims_max) {
						s_last_pushed = i;
					} else {//swap
						std::swap(p[i], p[s_last_pushed]);
						s_last_pushed = ims_max;
					}
				}
				ImGui::PopID();
				lh[i].end();
				ImGui::PopStyleColor();
			}
			continue;
		}

		size_t num_el;

		if (ctl_ptr.h.tt == ETYPE::set_interval &&
			(q.tt == ETYPE::number || q.tt == ETYPE::number_imm))
		{
			num_el = 1;
		} else if (	
			(ctl_ptr.h.tt == ETYPE::set_vector && q.tt == ETYPE::vector_imm) || 
			(ctl_ptr.h.tt == ETYPE::set_binary && 
				(q.tt == ETYPE::vector || q.tt == ETYPE::vector_imm)))
		{
			num_el = ctl_ptr.h.get_u24();
			if (num_el == 0)num_el = sr.get_dim();
			if (q.get_u24() != num_el) {
				continue;
			}
		}  else {
			continue;
		}

		///////////////////////////////////////////////
		//variable editor
		has_ctls = true;

		ImGui::BeginDisabled(cref.var_is_locked);
		IMS_SCOPE([] {ImGui::EndDisabled(); });

		static std::vector<line_helper> lh;
		lh.resize(num_el);

		let checkboxes = (di.t == ETYPE::distribution_int) &&
			(di.s == ESUBTYPE::dist_uniform) && (di.d[0] + 1 == di.d[1]);

		for (size_t i = 0; i < num_el; ++i) {

			ImGui::PushID(id++);
			IMS_SCOPE([] {ImGui::PopID(); });

			bool changed = false;

			if (ctl_ptr.h.tt == ETYPE::set_binary) {
				let& hx = sr.m_ops[q.get_offset() + i];
				bool v = hx.hdr.tt != ETYPE::empty;

				if (i == 0 && num_el > 1) {
					ImGui::NewLine();
				}

				let sl = i > 0;
				if (sl)lh[i].begin();
				IMS_SCOPE([&] {if (sl)lh[i].end(); });

				//ImGui::Text("%3zu", i); ImGui::SameLine();
				changed = ImGui::Checkbox("", &v);
				if (ImGui::IsItemActive() || ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%zu", i);
				}
				if (changed && w_ch.allow()) {

					//change in the GUI thread
					stop_build_then([xd, v, i, em]() {

						auto& b = xd->get_block();
						auto& q = b.m_ops[em.pos].hdr;

						b.m_ops[q.get_offset() + i].hdr.tt
							= v ? ETYPE::id : ETYPE::empty;

						b.m_flags.ready = false;

						ui_update_maps();
						do_rebuild_sync();
					});
				};
		
			} else {//vector of numbers
				///////////////////////////////////////////////
				//read the current value
				double dv;
				int64_t iv = 0, denominator = 0;
				bool is_int = false;

				ims_operator op = q;
				if (op.tt == ETYPE::vector_imm) {
					op.tt = ETYPE::number;
					op.set_offset(op.get_offset() + i);
				};

				bool res = sr.get_val(op, is_int, dv, iv, denominator);

				

				if (checkboxes) {
					if (i == 0 && num_el > 1) {
						ImGui::NewLine();
					}
				} else {
					if (i > 0) {
						ImGui::NewLine();
						ImGui::SameLine(control_pos);
					}
				}
				

				if (di.t == ETYPE::distribution_real) {
					if (!res) {
						dv = 0;
					} else if (is_int) {
						dv = double(iv) / denominator;
					}


					double mi;
					double ma;
				
					if (di.s == ESUBTYPE::dist_uniform) {
						ma = di.d[1];
						mi = di.d[0];
					} else {
						ma = -1;
						mi = 0;
					};
					
					changed = input_double2(dv, mi, ma);
					is_int = false;
				} else if (di.t == ETYPE::distribution_int) {
					if (!res) {
						iv = 0;
					} else if (is_int) {
						iv = iv / denominator;
					} else {
						iv = (int64_t)std::round(dv);
					}

					if (di.s == ESUBTYPE::dist_uniform) {
						let vmi = (int64_t)di.d[0];
						let vma = (int64_t)di.d[1];

						if (vma == vmi + 1) {
							bool bv = (iv == vma);

							assert(checkboxes);

							let sl = i > 0;
							if (sl)lh[i].begin();
							IMS_SCOPE([&] {if (sl)lh[i].end(); });

							changed = ImGui::Checkbox("", &bv);

							if (ImGui::IsItemActive() || ImGui::IsItemHovered()) {
								ImGui::SetTooltip("%zu", i);
							}
							if (changed) {
								iv = bv ? vma : vmi;
							};
						} else {

							assert(!checkboxes);
							let ws = 10.0 / ImGui::GetWindowWidth();
							changed = ImGui::DragScalar("", ImGuiDataType_S64, &iv,
								float((vma - vmi) * ws), &vmi, &vma);
						}

					} else {
						const int64_t step1 = 1;
						changed = ImGui::InputScalar("", ImGuiDataType_S64, &iv, &step1);
					};
					is_int = true;
				}

				if (com) {
					set_tooltip(com->c_str());
				}
				///////////////////////////////////////////////

				if (changed && w_ch.allow()) {

					//change in the GUI thread
					stop_build_then([xd, iv, dv, is_int, em, i]() {

						auto& b = xd->get_block();
						auto& q = b.m_ops[em.pos].hdr;

						size_t offs;

						if (q.tt == ETYPE::number_imm) {//->number
							q.tt = ETYPE::number;
							offs = b.m_ops.size();
							q.set_offset(offs);

							if (is_int) {
								q.ts = ESUBTYPE::integer;
							} else {
								q.ts = ESUBTYPE::real;
							}

							b.add(1);//q is spoiled here

						} else {

							b.convert_type_inplace(q, is_int);

							offs = q.u32 + i;

						}


						////////////////////////////////////////////////////////////
						//q is invalid here
#define q

						if (is_int) {
							b.m_ops[offs].i64 = iv;
						} else {
							b.m_ops[offs].f64 = dv;
						}

						b.m_flags.ready = false;
			
						ui_update_maps();
						do_rebuild_sync();
					});
				};//if
			}
		};//for i
	};
	ImGui::EndChild();
}

void ws_editor::on_load()
{
	m_source_editor.on_load();
}
