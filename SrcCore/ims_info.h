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

#include "js_engine.h"
#include "ifs_list.h"
#include "pool_ptr.h"

struct oper_block;
struct block_class;
struct read_state;
struct ast_stack;

//current document
struct ims_info: public boost::noncopyable
{
	ifs_list m_list;
	js_engine m_js_engine;

	std::string m_js_src;
	std::string m_js_description;
	std::string m_js_filename;

	//Changes happened, saving required
	bool m_need_save = false;

	////////////////////////////////////////////////////////////////////////////

	//at the time of the call, parent must already be set
	static void link_refs_for_block(
		const ims_info& nfo, oper_block* b, ast_stack& ai);

	//sets all parents and all references inside blocks to the tail of the list
	bool link_refs(const size_t idx_from);

	std::string create_from_constructor(const ims_val* v);

	pool_ptr m_constructor_dialog;

	[[nodiscard]]
	bool process_js(read_state& rs);

	void print_js(std::ostream& str) const;

};
