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
#include "aifs_printer.h"
#include "ifs_list.h"
#include "ims_operator.h"
#include "oper_block.h"
#include "ims_keywords.h"
#include "block_class.h"
#include "ast_stack.h"


void print_operator(
	const ifs_list& lst,
	std::ostream& str,
	const operator_ptr& ptr,
	const ETYPE par_type,
	const char* fmt);


using ims_keywords::nlc;


static void write_sym(std::ostream& str, char c)
{
	str.write(&c, sizeof(c));
};


static void write_block_src(std::ostream& str, const oper_block& b, oper_block_flags f)
{
	let& s = *b.m_src2;
	//the zero string with id and parent id was already written before
	constexpr size_t sl = 1;
	for (size_t i = sl; i < s.lines.size(); ++i) {

		if (i == s.line_name || (i == sl && s.line_name == ims_max)) {
			if (!b.m_name.empty()) {
				str << ims_keywords::name << "=" << b.m_name << nlc;
			}
		}
		if (i == s.line_attr || (i == sl && s.line_attr == ims_max)) {
			if (f.checked || f.hidden) {
				str << ims_keywords::attrib << "=";
				if (f.hidden)str << "h";
				if (f.checked)str << "c";
				str << nlc;
			}
		}

		if (i != s.line_name && i != s.line_attr) {
			str << s.lines[i] << nlc;
		}
	}

};


static void write_block_t(std::ostream& str, const oper_block& b, oper_block_flags f)
{
	let* g = b.get_class();
	let& lst = b.get_list();


	if (f.checked || f.hidden) {
		str << ims_keywords::attrib << "=";
		if (f.hidden)str << "h";
		if (f.checked)str << "c";
		str << nlc;
	}

	if (!b.m_name.empty()) {
		str << ims_keywords::name << "=" << b.m_name << nlc;
	}

	if (b.has_own_dim()) {
		str << ims_keywords::dim << "=" << b.get_dim() << nlc;
	}

	let js_init = b.get_js_init_identifier();
	if (js_init != ims_max) {
		str << ims_keywords::js_init << "=" << lst.m_idf.get_str_from_unk(js_init) << nlc;
	}

	if (b.is_converter()) {
		str << ims_keywords::convert_to << "=" << lst.get_str(b.m_conv_id) << nlc;
	}

	if (f.has_timestamp && b.m_timestamp > 0) {
		str << ims_keywords::timestamp << "=" << b.m_timestamp << nlc;
	}

	for (let& q : b) {

		const char* prec = nullptr;

		if (q.is_builtin()) {
			let bid = q.get_builtin();

			if (bid == builtin_ids::palette || bid == builtin_ids::background) {
				prec = "{:.3f}";
			}

			str << get_builtin_name(bid);
		} else {
			
			if (q.is_subs()) {
				write_sym(str, ims_keywords::subst);
			}

			let& var_name = g->get_var_name(q.gr());
			if (var_name.empty()) {
				str << ims_keywords::autoprefix << q.gr();
			} else {
				str << var_name;
			}
		}

		str << "=";

		//right side
		print_operator(lst, str, b.get_ptr(q.pos5), ETYPE::min_priority, prec);

		str << nlc;
	};
};

void ims_write_block(
	std::ostream& dst,
	const oper_block* b)
{
	write_sym(dst, ims_keywords::block);
	dst << b->str_id4();


	if (b->m_parent_id != block_id_max) {
		write_sym(dst, ims_keywords::base);
		dst << b->get_list().get_str(b->m_parent_id);
	}

	dst << nlc;

	if (b->m_src2 && !b->m_src2->lines.empty()) {
#ifndef NDEBUG
		write_block_t(dst, *b, b->m_flags);
#else
		write_block_src(dst, *b, b->m_flags);
#endif // !NDEBUG

		
	} else {
		write_block_t(dst, *b, b->m_flags);
	}
	dst << nlc;
}


////////////////////////////////////////////////////////////////////////////////
std::string_view aifs_printer::get_temp_id(const oper_block* b) const
{
	let id = b->str_id4();
	if (!id.empty()) {
		return id;
	}
	auto it = uds_need_id.find(b);
	if (it == uds_need_id.end()) {
		return {};
	}
	return it->second;
}

void aifs_printer::write_block(
	std::ostream& dst, 
	const oper_block* b, 
	oper_block_flags f, 
	bool ignore_priv)
{
	write_sym(dst, ims_keywords::block);
	dst << get_temp_id(b);

	let* dp = b->get_parent();



	if (dp) {
		if (ignore_priv) dp = dp->elevate_priv();
		if (dp) {
			write_sym(dst, ims_keywords::base);
			let id = get_temp_id(dp);
			assert(!id.empty());
			dst << id;
		}
	}
	dst << nlc;

	if (b->m_src2 && !b->m_src2->lines.empty()) {
#ifndef NDEBUG
		write_block_t(dst, *b, f);
#else
		write_block_src(dst, *b, f);
#endif
	} else {
		write_block_t(dst, *b, f);
	}
	dst << nlc;
}



////////////////////////////////////////////////////////////////////////////////


void aifs_printer::clear()
{
	uds.clear();
	arr.clear();

	uds_need_id.clear();
	arr_need_id.clear();
}

void aifs_printer::add_block(const oper_block* p)
{
	assert(p);
	if (uds.emplace(p).second) {
		arr.emplace_back(p);
	}
}

void aifs_printer::add_depends(const ifs_list& lst, ast_stack& ai, bool ignore_js)
{
	
	//the array will grow longer as you iterate
	//cannot be replaced with range-based!
	for (size_t i = 0; i < arr.size(); ++i) {
		auto* b = const_cast<oper_block*>(arr[i]);

		let* dp = b->get_parent();
		if (dp) {
			if (ignore_js) {
				dp = dp->elevate_priv();
				if (!dp || dp->m_flags.from_js)continue;
			}
			add_dep_block(dp);
		}

		if (b->is_converter()) {
			add_dep_block(lst.get_block(b->m_conv_id));
		}

		for (let& q : *b) {
			ai.reset3({ b, &b->m_ops[q.pos5].hdr });
			for (auto& x : ai) {
				if (x.h->tt != ETYPE::identifier)continue;
				let* bx = lst.get_block_from_unk(x.h->get_unk_id());
				if (bx)add_dep_block(bx);
			}
		}
	}
}

size_t aifs_printer::prepare(
	const ifs_list& lst,
	std::span<const oper_block*> arr2, 
	bool only_checked, 
	bool ignore_js)
{
	//instead of empty inherited blocks, saves their parent ones
	for (let* b : arr2) {
		
		auto* nb = b->elevate_empty();//happens when we print to the console @IFStile
		if (!nb)continue;
		
		if (ignore_js && b->m_flags.from_js) {
			continue;//print those that came from js only in console mode
		}
		add_block(nb);
	}

	if (arr2.empty()) {//the whole list or only the selected ones
		for (let id : lst.m_blocks) {
			let* b = lst.get_block(id);
			if (only_checked && !b->m_flags.checked)continue;

			if (ignore_js && b->m_flags.from_js) {
				continue;//print those that came from js only in console mode
			}
			add_block(b);
		}
	}

	let arr_initial_size = arr.size();//to support hide_other

	//will save with all dependencies
	
	ast_stack ai;
	add_depends(lst, ai, ignore_js);


	size_t uni_idx = 0;
	for (let* b : arr_need_id) {
		uds_need_id[b] = lst.m_idf.gen_unique_block_id("UN", &uni_idx);
		++uni_idx;
	}

	return arr_initial_size;
}

size_t aifs_printer::ims_to_text(
	std::ostream& str, 
	const ifs_list& lst,
	std::span<const oper_block*> arr2, 
	bool only_checked, 
	bool hide_other, 
	bool ignore_js)
{

	let arr_initial_size = prepare(lst, arr2, only_checked, ignore_js);

	let do_uncheck = arr2.empty() && only_checked;//remove checked

	for (size_t i = 0; i < arr.size(); ++i) {
		let* b = arr[i];

		auto f = b->m_flags;
		bool do_hide = hide_other && i > arr_initial_size;
		if (do_uncheck)f.checked = false;
		if (do_hide)f.hidden = true;

		write_block(str, b, f, ignore_js);
	}

	return arr.size();
}

void aifs_printer::add_dep_block(const oper_block* b)
{
	add_block(b);

	if (b->str_id4().empty()) {
		assert(b);
		if (uds_need_id.emplace(b, std::string()).second) {
			arr_need_id.emplace_back(b);
		}
	}
}
