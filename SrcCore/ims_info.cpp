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

#include "ims_info.h"
#include "block_class.h"
#include "oper_block.h"
#include "ifs_data_text.h"
#include "ast_stack.h"
#include "js_engine.h"

#if 0
void intrusive_ptr_release(var_subs_info::V* p)
{
	var_subs_info::nfo(p).release(p);
}


void ims_info::release(var_subs_info::V* p)
{
	m_subs_info.release(p);
}
#endif

void ims_info::link_refs_for_block(
	const ims_info& nfo, oper_block* b,  ast_stack& ai)
{
	//the class must always be
	auto* cl = b->get_class();
	if (!cl) {
		//TODO: almost never uses ims_info
		cl = &b->set_new_class(&nfo);
	}
	
	//create variables and graphs
	size_t num = 0;
	size_t start_unnamed_ref = 0;

	b->m_subspace.set_undef();

	b->m_flags.only_view = (b->m_js_init == ims_max);

	//deal with block variables
	for(let& q: *b){

		auto& vh = q.get_vh(*b);
		IMS_SCOPE([&] {++num;});
		if (vh.is_builtin()) {
			if (vh.is_builtin(builtin_ids::subspace)) {
				b->m_subspace = { b->m_ops[q.pos5].hdr, b };
			}
			continue;//does not require correction
		}

		b->m_flags.only_view = false;

		size_t idx;
		bool is_subst;

		if (num >= b->m_named_vars) {//nameless
			if (num == b->m_named_vars) {
				cl = &b->create_own_class();
				start_unnamed_ref = cl->m_refs.size();
			}


			idx = cl->add_var(ims_max);
			
			is_subst = false;

		}else  {
			is_subst = vh.is_subs();
			let unk_idx = vh.get_ref();
			idx = cl->find_var(unk_idx);
			if (idx == ims_max) {
				//the current class doesn't have one yet - let's create a new graph
				cl = &b->create_own_class();


				idx = cl->add_var(unk_idx);
			}
		}

	
		vh.ref8 = var_header::pack(idx, is_subst);
	}

	//fill in the references inside the bytecode
	for (let& q : *b) {
		ai.reset3({ b, &b->m_ops[q.pos5].hdr });
		for (auto& p : ai) {

			if (p.h->tt != ETYPE::reference)continue;

			let unk_id = p.h->get_offset();

			if (p.h->ts == ESUBTYPE::ref_unknown) {

				auto graph_ref = cl->find_var(unk_id);
				if (graph_ref != ims_max) {
					p.h->set_reference(graph_ref);
				}else {
					p.h->tt = ETYPE::unk_reference;
					p.h->set_offset(unk_id);
				}
			}else if (p.h->ts == ESUBTYPE::ref_js) {
				p.h->set_reference(unk_id + start_unnamed_ref);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	let* p = b->get_parent();
	if (p) {
		if (!b->m_flags.has_dim) {
			b->m_dim2 = p->m_dim2;
		}
		if (!b->m_subspace.is_def()) {
			b->m_subspace = p->m_subspace;
		}
	}
	////////////////////////////////////////////////////////////////////////////
	//adjust description
	auto* s = b->m_src2.get();
	
	if (s) {
		auto& a = s->ref2comments;
		assert(a.empty());
		for (auto& ud : s->unk2description) {

			auto it = cl->m_unk2var.find(ud.first);
			if (it != cl->m_unk2var.end()) {
				let ref = it->second;
				if (a.size() <= ref)a.resize(ref + 1);
				a[ref] = ud.second;
			}
		}
		s->unk2description.clear();
	}
}

bool ims_info::link_refs(const size_t idx_from)
{
	ifs_list& lst = m_list;

	for (size_t i = idx_from; i < lst.m_blocks.size(); ++i) {
		auto& b = *lst.get_block_by_idx(i);

		//fill in parents
		if (b.m_parent_id != block_id_max) {
			let& d = lst.m_id2data[b.m_parent_id];
			if (!d.b) {
				let& str = lst.m_idf.get_str_from_unk(d.m_str_id);
				ims_error("Invalid parent: {}", str);
				return false;
			}
			b.set_parent(d.b.get());
		}

		//check the converter
		if (b.is_converter()) {
			let& d = lst.m_id2data[b.m_conv_id];
			if (!d.b) {
				let& str = lst.m_idf.get_str_from_unk(d.m_str_id);
				ims_error("Invalid $convert_to: {}", str);
				return false;
			}
		}
	}

	//consider old elements as processed
	for (size_t i = 0; i < lst.m_blocks.size(); ++i) {
		lst.get_block_by_idx(i)->m_flags.marked = (i < idx_from);
	}

	std::vector<oper_block*> parr;

	ast_stack ai;

	//process the names of block variables and create graphs
	for (size_t i = idx_from; i < lst.m_blocks.size(); ++i) {

		parr.clear();
		for (auto* p = lst.get_block_by_idx(i);
			p && !p->m_flags.marked;
			p = (oper_block*)p->get_parent())
		{
			parr.emplace_back(p);
			if (idx_from + parr.size() > lst.m_blocks.size()) {
				ims_error("Recursive parent:  {}", p->str_id4());
				return false;
			}
		}
		
		for (auto* b : boost::adaptors::reverse(parr)) {

			if (b->m_flags.marked)continue;
			b->m_flags.marked = true;//completed
			link_refs_for_block(*this, b, ai);
		}
	}

	return true;
}

std::string ims_info::create_from_constructor(
	const ims_val* v)
{
	read_state rs;

	size_t num_blocks_before = m_list.m_blocks.size();

	let err_msg = m_js_engine.create_from_constructor(v, rs, m_list);
	if (!err_msg.empty()) {
		return err_msg;
	}

	if (!link_refs(num_blocks_before)) {
		return "Could not link references.";
	}

	for (size_t i = num_blocks_before; i < m_list.m_blocks.size(); ++i) {
		m_list.get_block_by_idx(i)->m_flags.from_js = false;
	}

	return "";
}

////////////////////////////////////////////////////////////////////////////////



bool ims_info::process_js(read_state& rs)
{
	m_constructor_dialog.reset();
	if (m_js_src.empty()) {
		return true;
	}
	return m_js_engine.get_blocks_from_js(
		m_js_filename,
		m_js_src,
		m_js_description,
		rs, 
		m_list,
		m_constructor_dialog);
}

static std::string remove_fragments(
	std::string_view str,
	std::string_view start_marker,
	std::string_view end_marker)
{
	std::string result;
	//Capacity reservation helps to avoid reallocations during append operations
	result.reserve(str.size());

	size_t current_pos = 0;
	while (current_pos < str.size()) {
		//Find the next occurrence of the opening marker
		size_t start_pos = str.find(start_marker, current_pos);

		if (start_pos == std::string_view::npos) {
			//No more fragments to remove, append remaining tail
			result.append(str.substr(current_pos));
			break;
		}

		//Add text found BEFORE the start marker to the result
		result.append(str.substr(current_pos, start_pos - current_pos));

		//Search for the closing marker strictly after the current opening marker
		size_t end_pos = str.find(end_marker, start_pos + start_marker.length());

		if (end_pos != std::string_view::npos) {
			//Successfully found a pair; move current_pos past the closing marker
			//This correctly handles back-to-back markers like "[][ ]"
			current_pos = end_pos + end_marker.length();
			//Remove line breaks just after the end_marker 
			for (; current_pos < str.size(); ++current_pos) {
				if (str[current_pos] != '\r' && str[current_pos] != '\n')break;
			};
		} else {
			//No closing marker found: append the rest (including the unmatched start_marker)
			result.append(str.substr(start_pos));
			break;
		}
	}

	return result;
}

#include "ims_keywords.h"
void ims_info::print_js(std::ostream& str) const
{
	auto js_for_save = remove_fragments(m_js_src,
		"//AIFS_IGNORE_BEGIN", "//AIFS_IGNORE_END");

	auto script = boost::algorithm::trim_copy_if(
		std::string_view{ js_for_save }, boost::algorithm::is_any_of(" \t\r\n"));
	
	if (script.empty())return;

	str << script << ims_keywords::nlc << ims_keywords::js_delimeter
		<< ims_keywords::nlc << ims_keywords::nlc;
}
