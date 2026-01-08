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
#include "ui_components.h"
#include "gui.h"
#include "ims_window_drag.h"
#include "build_data.h"
#include "oper_block.h"
#include "block_class.h"
#include "ims_keywords.h"
#include "block_graph.h"

const char* ws_components::get_title()
{
	return "Components";
}


void ws_components::show()
{
	auto* bd = get_global_bd();
	if (!bd || !bd->m_bi.exists())return;

	if (bd->m_bi.m_id8 == 0)return;


	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
		check_navi_ex(true);
	}

	//assert(!bi->m_im.di.empty());

	let* sr = get_global_block();
	if (!sr)return;


	let* cl = sr->get_class();
	if (!cl)return;

	let* g = sr->get_graph();


	int tflags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_Reorderable |
		ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable |
		ImGuiTableFlags_Hideable | ImGuiTableFlags_NoSavedSettings;

	constexpr std::array sl{ "Name","DIM","DIM type","Strong ID","Depth","Edges" };
	bool bv = ImGui::BeginTable("SetsList", (int)std::size(sl), tflags);
	if (!bv)return;

	ims_window_drag w_ch(true);

	enum class ECOMP_COLS :uint8_t
	{
		Name,
		DIM,
		DIM_type,
		Strong_ID,
		Depth,
		Edges,
		NUM_COLS
	};


	static_assert((size_t)ECOMP_COLS::NUM_COLS == sl.size());


	ImGui::TableSetupScrollFreeze(0, 1);
	for (let* c : sl)ImGui::TableSetupColumn(c);
	ImGui::TableHeadersRow();

	size_t root_ref = bd->get_block().get_active_ref();
	let& fg = bd->m_bi.get_fg();

	auto* sorts_specs = ImGui::TableGetSortSpecs();

	if (m_brefs.init8(*bd) && sorts_specs) {
		sorts_specs->SpecsDirty = false;
	}

	auto& ar = m_brefs.m_all_refs;

	if (sorts_specs && sorts_specs->SpecsDirty) {

		for (int n = 0; n < sorts_specs->SpecsCount; n++)
		{
			let* sort_spec = &sorts_specs->Specs[n];

			let asc = sort_spec->SortDirection == ImGuiSortDirection_Ascending;

			switch ((ECOMP_COLS)sort_spec->ColumnIndex) {
			case ECOMP_COLS::Name:
				std::stable_sort(ar.begin(), ar.end(),
					[&](let i0, let i1)
					{
						let& v0 = cl->get_var_name(i0);
						let& v1 = cl->get_var_name(i1);

						return asc ? v0 < v1 : v0 > v1;
					});
				break;
			case ECOMP_COLS::DIM:
				std::stable_sort(ar.begin(), ar.end(),
					[&](let i0, let i1)
					{
						let& d0 = bd->m_bi.m_im.di[fg.m_ver2com[g->ref2fg(i0)]].H;
						let& d1 = bd->m_bi.m_im.di[fg.m_ver2com[g->ref2fg(i1)]].H;

						return asc ? d0 < d1 : d0 > d1;
					});
				break;
			case ECOMP_COLS::DIM_type:
				std::stable_sort(ar.begin(), ar.end(),
					[&](let i0, let i1)
					{
						let& d0 = bd->m_bi.m_im.di[fg.m_ver2com[g->ref2fg(i0)]].DR;
						let& d1 = bd->m_bi.m_im.di[fg.m_ver2com[g->ref2fg(i1)]].DR;

						return asc ? d0 < d1 : d0 > d1;
					});
				break;
			case ECOMP_COLS::Strong_ID:
				std::stable_sort(ar.begin(), ar.end(),
					[&](let i0, let i1)
					{
						let& c0 = fg.m_ver2com[g->ref2fg(i0)];
						let& c1 = fg.m_ver2com[g->ref2fg(i1)];

						return asc ? c0 < c1 : c0 > c1;
					});
				break;
			case ECOMP_COLS::Depth:
				std::stable_sort(ar.begin(), ar.end(),
					[&](let i0, let i1)
					{
						let& c0 = fg.m_comp[fg.m_ver2com[g->ref2fg(i0)]].depth;
						let& c1 = fg.m_comp[fg.m_ver2com[g->ref2fg(i1)]].depth;

						return asc ? c0 < c1 : c0 > c1;
					});
				break;
			case ECOMP_COLS::Edges:
				std::stable_sort(ar.begin(), ar.end(),
					[&](let i0, let i1)
					{
						let& c0 = fg.m_vers[g->ref2fg(i0)].sz;
						let& c1 = fg.m_vers[g->ref2fg(i1)].sz;

						return asc ? c0 < c1 : c0 > c1;
					});
				break;
			default:
				break;

			}

		}
		sorts_specs->SpecsDirty = false;
	}


	ImGuiListClipper clipper;
	clipper.Begin((int)ar.size());


	while (clipper.Step()) {
		for (int j = clipper.DisplayStart; j < clipper.DisplayEnd; j++) {
			let i = ar[j];
		

			ImGui::TableNextRow();
			
			////////////////////////////////////////////////////////////////
			ImGui::TableNextColumn();

			ImGui::PushID((int)i);
			
			m_var_name = cl->get_var_name(i);
			if (m_var_name.empty()) {
				m_var_name = ims_keywords::autoprefix;
				m_var_name += std::to_string(i);
			}
			
			bool b = ImGui::Selectable(m_var_name.c_str(),
				i == root_ref,
				ImGuiSelectableFlags_SpanAllColumns);

			ImGui::PopID();

			let* com = sr->get_comment(i);
			if (com)set_tooltip(com->c_str());
			

			if (b && w_ch.allow()) {
				set_new_ver_ref(i);
			}

			////////////////////////////////////////////////////////////////
			let vr = g->ref2fg(i);

			if (vr == ims_max || fg.is_ver_empty(vr)) {
				continue;
			}

			let cidx = fg.m_ver2com[vr];

			let& c = fg.m_comp[cidx];

			assert(cidx < bd->m_bi.m_im.di.size());

			let& di = bd->m_bi.m_im.di[cidx];
		
			const char* dtype = "-";
			switch (di.DR)
			{
			case dim_relations::own:	dtype = "+";	break;
			case dim_relations::dep:	dtype = ".";	break;
			case dim_relations::equ:	dtype = "oo";	break;
			};

			////////////////////////////////////////////////////////////////
			ImGui::TableNextColumn();
			ImGui::Text("%.16f", di.H);
			////////////////////////////////////////////////////////////////
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(dtype);
			////////////////////////////////////////////////////////////////
			ImGui::TableNextColumn();
			ImGui::Text("%zu", cidx);
			////////////////////////////////////////////////////////////////
			ImGui::TableNextColumn();
			ImGui::Text("%zu", c.depth);
			////////////////////////////////////////////////////////////////
			ImGui::TableNextColumn();
			ImGui::Text("%zu", fg.m_vers[vr].sz);
		}
	}
	ImGui::EndTable();
}
