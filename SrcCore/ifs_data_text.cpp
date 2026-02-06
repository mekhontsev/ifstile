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

#include "ims_keywords.h"
#include "oper_block.h"
#include "parser_state.h"
#include "ifs_data_text.h"

#include "error_helper.h"
#include "ifs_list.h"

namespace new_parser
{


static bool is_space(char c)
{
	return c == ' ' || c == '\t' || c == ';';
};


static bool is_comment(char c)
{
	return c == ims_keywords::comment;
}

static bool is_alpha(char c)
{
	return 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z';
}

static bool is_digit(char c)
{
	return '0' <= c && c <= '9';
}

#if 0

static bool is_eol(char c)
{
	return c == '\r' || c == '\n';
};

static bool is_delim(char c)
{
	return c == ',' || is_space(c);
};

template<typename SkipFunc>
static bool skip(
	std::istreambuf_iterator<char>& it, 
	const std::istreambuf_iterator<char>& it_end, 
	SkipFunc* F)
{
	for (;;) {
		if (it == it_end) {
			return false;
		}
		if (!F(*it)) {
			break;
		}
		it++;
	}
	return true;
}

static bool skip_unused(
	std::istreambuf_iterator<char>& it, 
	const std::istreambuf_iterator<char>& it_end)
{
	for (;;) {
		if (it == it_end) {
			return false;
		}
		if (*it > ' ') {
			break;
		}
		it++;
	}
	return true;
}

//get a real number from the stream
template<typename Number>
bool parse(
	Number& dst, 
	std::istreambuf_iterator<char>& it,
	const std::istreambuf_iterator<char>& it_end)
{
	std::string str;


	for (; it != it_end && !is_delim(*it) &&
		!is_comment(*it) && !is_eol(*it); ++it) {
		str += *it;
	}
	if (str.empty()) {
		return false;
	}

	return boost::conversion::try_lexical_convert(str, dst);
}

//does not go to a new line
static bool read_ifs_comment(
	std::string& str, 
	std::istreambuf_iterator<char>& it, 
	const std::istreambuf_iterator<char>& it_end)
{
	str.clear();

	skip(it, it_end, is_space);

	if (it == it_end)return false;
	if (*it != ';')return false;
	++it;

	for (;;) {
		if (it == it_end || is_eol(*it)) {
			break;
		}
		str += *it++;
	}
	return true;
};
#endif

};//namespace

bool is_var_id_sym(char c)
{
	return new_parser::is_digit(c) || new_parser::is_alpha(c) ||
		c == '_' || c == '$' || c == '&';
}



void WARN_DUP_ID(std::string_view id)
{
	//ignore this block and continue 
	//useful for loading from the clipboard
	ims_warning("Duplicate block ignored, id  =", id);
}

//reads a block as an array of dst strings
//returns false on error
//fills dst_size - how many elements are currently in dst
//skips everything between blocks - even comments
//lines_counter - increments at each line completion
//first_line - on which lines_counter the block was found
static bool read_next_block(
	std::vector<std::string>& dst,
	size_t& dst_size,
	size_t& lines_counter,
	size_t& first_line,
	std::istreambuf_iterator<char>& it, 
	const std::istreambuf_iterator<char>& end)
{
	error_helper::line ehl(first_line);

	bool block_found = false;
	bool directive_found = false;

	dst_size = 0;

	//found the next block
	bool complete = false;

	while (it != end) {//by lines

		if (block_found || dst_size == 0) {//allocate space for a new line
			++dst_size;
			if (dst.size() < dst_size) {
				dst.resize(dst_size);
			}
			dst[dst_size - 1].clear();
		}

		bool com = false;//skip the entire line to the end because of the comment

		//line reading loop
		for (; it != end; ++it) {

			bool next_line_found = false;

			switch (*it) {
			case '\r':
				++it;
				next_line_found = true;
				if (it != end && *it == '\n') {
					++it;
				}
				++lines_counter;
				break;
			case '\n':
				++it;
				next_line_found = true;
				++lines_counter;
				break;
			case ims_keywords::comment:
				com = true;
				break;
			case ims_keywords::block:
				if (!com) {
					if (block_found) {//support for @@ directives
						if (dst_size > 1) {
							complete = true;//found the next block, finishing
						} else {
							directive_found = true;//continue to read
						}
					}
					else {
						block_found = true;
						first_line = lines_counter;
					}
					break;
				}
				break;
			}

			if (next_line_found || complete) {
				break;
			}

			if (block_found) {
				dst[dst_size - 1].push_back(*it);
			}else if (!com) {
				if (!new_parser::is_space(*it)) {
					ims_error("Invalid symbol: {}", *it);
					return false;
				}
			}
		}

		if (complete || directive_found)break;
	}

	//remove spaces
	for (; dst_size > 0; --dst_size) {
		for (let c : dst[dst_size - 1]) {
			if (!new_parser::is_space(c)) {
				return true;
			}
		}
	}

	return true;

};

size_t js_from_stream(
	std::string& dst,
	size_t& lines_counter,
	std::istreambuf_iterator<char>& it,
	const std::istreambuf_iterator<char>& end) 
{
	bool js_found = false;

	std::string line;

	size_t line_start = 0;

	dst.clear();

	while (it != end) {//by lines
		
		line.clear();

		//line reading loop
		for (; it != end; ++it) {
			if (*it == '\r') {
				++it;
				if (it != end && *it == '\n') {
					++it;
				}
				++lines_counter;
				break;
			}
			if (*it == '\n') {
				++it;
				++lines_counter;
				break;
			}


			if (!js_found) {
				//JS search mode
				if (*it == ' ' || *it == '\t') {
					continue;
				}
				if (*it == ims_keywords::comment || *it == ims_keywords::block) {
					return 0;//still haven't found JS
				}
				//found something non-trivial
				line_start = lines_counter;
				js_found = true;	
			}

			line.push_back(*it);
		}

		if (js_found) {
			if (line == ims_keywords::js_delimeter) {//JS end indicator
				return line_start;
			}

			dst += line;
			dst += ims_keywords::nlc;
		}
	}

	return line_start;
	
}


bool aifs_from_stream_ex(
	ifs_list& lst,
	size_t& cur_line,
	read_state& rs,
	std::istreambuf_iterator<char>& it_beg,
	const std::istreambuf_iterator<char>& it_end) 
{
	size_t first_line = 0;

	if (!read_next_block(
		rs.m_source_lines,
		rs.m_source_num_lines,
		cur_line,
		first_line,
		it_beg,
		it_end))
	{
		return false;//critical error - failed to load file
	}

	if (rs.m_source_num_lines == 0) {
		return true;//completed
	};

	read_state::parse_result res;

	if (!rs.parse_block(lst, res, first_line)) {
		return false;//critical error - failed to load file
	}

	if (res.status != read_state::parse_result::e_continue) {
		if (res.status == read_state::parse_result::e_completed) {
			rs.m_source_num_lines = 0;
		} else {
			assert(res.status == read_state::parse_result::e_ignore);
		}
		return true;
	}

	auto* b = lst.add_block(rs.m_id);
	b->m_line8 = first_line;

	if (!res.parent_id.empty()) {
		b->m_parent_id = lst.insert_by_str_id(res.parent_id);
	}

	size_t line_name = ims_max;
	size_t line_attr = ims_max;

	if (!rs.process_string_vars(lst, *b, line_name, line_attr)) {
		return false;
	}

	if (res.keep_source ||
		b->m_parent_id == block_id_max ||
		b->m_ops.empty())
	{
		auto& s = b->m_src2;
		s.reset(new oper_source);
		s->lines.resize(rs.m_source_num_lines);
		s->line_name = line_name;
		s->line_attr = line_attr;

		//copy the source code
		let& it = rs.m_source_lines.begin();
		std::copy(it, it + rs.m_source_num_lines, s->lines.begin());

		//search for a block comment
		bool md_found = false;
		for (size_t i = 1; i < s->lines.size(); ++i) {
			std::string_view v(s->lines[i]);
			while (!v.empty() && (v.front() == ' ' || v.front() == '\t')) {
				v.remove_prefix(1);
			}
			if (v.empty()) {
				if (!md_found)continue;
				if (md_found)break;
			} else if (v.front() == ims_keywords::comment) {
				v.remove_prefix(1);
				s->md += v;
				s->md += "\n";
				md_found = true;
			} else {
				break;
			}
		}

		//set variables comments
		auto& u2d = s->unk2description;

		for (let& v : rs.m_vars) {
			if (ref_is_builtin(v.index7))continue;

			std::string com;

			//search for the comment above
			size_t idx = v.line7 - 1;
			for (; idx > 0; --idx) {
				let& q = s->lines[idx];
				if (q.empty() || q.front() != ims_keywords::comment)break;
			}
			for (size_t i = idx + 1; i < v.line7; ++i) {
				com += s->lines[i].substr(1);
				if (i + 1 < v.line7)com += "\n";
			}

			//search in the same line
			let& q = s->lines[v.line7];
			let pos = q.find(ims_keywords::comment);
			if (pos != std::string::npos) {
				if (!com.empty())com += "\n";
				com.append(q.begin() + pos + 1, q.end());
			}

			boost::algorithm::trim(com);
			if (!com.empty()) {
				u2d.emplace_back(
					lst.m_idf.get_unk_id(v.name),
					std::move(com));
			}
		}
	}

	return true;//continue to parse
}


void read_state::read_id(std::string& dst, std::string_view& s) //s changes
{
	dst.clear();
	while (!s.empty()) {
		let c = s.front();
		if (!is_var_id_sym(c))break;
		s.remove_prefix(1);
		dst += c;
	}
}

void read_state::skip_spaces(std::string_view& s) //s changes
{
	while (!s.empty() && new_parser::is_space(s.front())) {
		s.remove_prefix(1);
	}
}

void read_state::trim_spaces(std::string_view& s) //s changes
{
	skip_spaces(s);
	while (!s.empty() && new_parser::is_space(s.back())) {
		s.remove_suffix(1);
	}
}

////////////////////////////////////////////////////////////////////////////////

static bool parse_subspace_old_format(
	std::string& id,
	std::vector<size_t>& sbt,
	std::string_view qv)
{
	read_state::read_id(id, qv);
	if (id.empty()) {
		return false;
	}

	read_state::trim_spaces(qv);

	while (!qv.empty()) {
		let p = qv.find_first_of(" ");

		std::string_view ns;

		if (p != qv.npos) {
			ns = qv.substr(0, p);
			qv = qv.substr(p + 1);
			read_state::skip_spaces(qv);
		}
		else {
			ns = qv;
			qv = std::string_view();
		}

		if (!ns.empty()) {
			size_t num;
			if (!boost::conversion::try_lexical_convert(ns, num)) {
				return false;
			}
			sbt.emplace_back(num);
		}
	}

	if (sbt.empty()) {
		return false;
	}

	return true;
}

bool read_state::pre_process_variable(
	ifs_list& lst, 
	parsed_var& q, 
	oper_block& b, 
	size_t& line_name, 
	size_t& line_attr)
{
	let& n = q.name;

	assert(!n.empty());

	q.is_subst = !n.empty() && n[0] == ims_keywords::subst;
	bool is_builtin = !n.empty() && n[0] == ims_keywords::builtin;

	if (q.is_subst && !n.empty()) {
		q.name.erase(0, 1);
	}

	////////////////////////////////////////////////////////////////////////////

	if (!is_builtin) {//regular variables
		q.index7 = lst.m_idf.get_unk_id(n);
	} else {//$builtins
		let bid = get_builtin_id(n);

		if (bid != builtin_ids::num_ids) {//$subspace, $camera, etc.
			q.index7 = builtin2ref(bid);
			assert(q.index7 != ims_max);

			if (bid == builtin_ids::subspace) {
				std::string sbt_id;
				std::vector<size_t> sbt_cells;
				if (parse_subspace_old_format(sbt_id, sbt_cells, q.val)) {
					q.val = "[";
					q.val += sbt_id;
					for (let c : sbt_cells) {
						q.val += ", ";
						q.val += std::to_string(c);
					}
					q.val += "]";
				}
			}
		} else {//very special variables

			q.index7 = ims_max;

			if (n == ims_keywords::name) {
				b.m_name = q.val;
				line_name = q.line7;
			} else if (n == ims_keywords::parent) {
				//JS only
			} else if (n == ims_keywords::js_init) {
				//JS only
			} else if (n == ims_keywords::js_info) {
				//JS only
			} else if (n == ims_keywords::str_id) {
				//JS only
			} else if (n == ims_keywords::time) {
				//ignore
			} else if (n == ims_keywords::attrib) {
				for (let& c : q.val) {
					switch (c) {
					case 'h': b.m_flags.hidden = true; break;
					case 'c': b.m_flags.checked = true; break;
					}
				}
				line_attr = q.line7;
			} else if (n == ims_keywords::timestamp) {

				uint64_t ts = 0;

				if (!boost::conversion::try_lexical_convert(q.val, ts)) {
					ims_error("Invalid timestamp: {}", q.val);
					return false;
				}

				if (ts > 0) {
					b.m_timestamp = ts;
					b.m_flags.has_timestamp = true;
				}
			} else if (n == ims_keywords::convert_to) {
				//classify it later
				b.m_conv_id = lst.insert_by_str_id(q.val);
			} else if (n == ims_keywords::dim) {

				if (!q.val.empty()) {//otherwise it would have already been processed before in JS

					size_t dim = 0;

					if (!boost::conversion::try_lexical_convert(q.val, dim)) {
						ims_error("Invalid dimension: {}", q.val);
						return false;
					}

					if (dim > 100) {
						ims_error("Invalid dimension: {}", dim);
						return false;
					}
					b.m_flags.has_dim = true;
					b.m_dim2 = dim;
				}
			} else {
				ims_error("Invalid var name:  {}", q.name);
				return false;//unknown identifier
			}
		}
	}

	return true;
}



bool read_state::process_variable(
	ifs_list& lst,
	parsed_var& q,
	oper_block& b)
{

	if (q.index7 == ims_max) {
		//special identifiers ($dim, etc.).
		//as well as additional unnamed variables from JS
		return true;
	}

	if (q.val.empty()) {
		return true;//processed as JS
	}

	let offset = b.add_var(m_pfo.m_pos2, q.index7, q.is_subst);

	let err_msg = parse_expression_as_aifs(lst.m_idf,
		q.val, b, offset);

	if (!err_msg.empty()) {
		ims_error("{}", m_pfo.err.str());
		return false;
	};


	return true;
}


bool read_state::parse_block(
	ifs_list& lst,
	read_state::parse_result& ret,
	size_t first_line)
{
	error_helper::line elh(first_line);

	m_source_view.resize(m_source_num_lines);
	for (size_t i = 0; i < m_source_view.size(); ++i) {

		auto& s = m_source_view[i];
		s = m_source_lines[i];

		for (size_t j = 0; j < s.size(); ++j) {
			if (new_parser::is_comment(s[j])) {
				s = s.substr(0, j);
				ret.keep_source = true;
				break;
			}
		}

		trim_spaces(s);
	}

	//block header line
	auto& hdr = m_source_view.front();
	assert(hdr.front() == ims_keywords::block);//by definition
	hdr.remove_prefix(1);

	bool is_directive = !hdr.empty() && hdr.front() == ims_keywords::block;
	if (is_directive)hdr.remove_prefix(1);

	read_id(m_id, hdr);

	skip_spaces(hdr);

	//reading the directive
	if (is_directive) {

		if (m_id == ims_keywords::version) {
			//obsolete, let's continue, no error
			ret.status = read_state::parse_result::e_ignore;
			return true;
		}

		if (m_id == ims_keywords::end) {
			//finished parsing the block, no error
			ret.status = read_state::parse_result::e_completed;
			return true;
		}

		
		ims_error("Invalid directive: {}", m_id);
		return false;//critical error
	}

	if (lst.find_block(m_id)){
		//completely ignore the block... don't even check for errors
		ret.status = read_state::parse_result::e_ignore;
		WARN_DUP_ID(m_id);
		return true;
	};



	////////////////////////////////////////////////////////////////////////
	//looking for parents - there can't be any errors here
	ret.parent_id.clear();
	if (!hdr.empty() && hdr.front() == ':') {
		hdr.remove_prefix(1);
		skip_spaces(hdr);
		read_id(ret.parent_id, hdr);
	}

	skip_spaces(hdr);
	if (!hdr.empty()) {
		ims_error("invalid block header");
		return false;
	}

	m_vars.clear();
	////////////////////////////////////////////////////////////////////////

	size_t line = 1;//next after the header

	while (line < m_source_view.size()) {
		auto& h = m_source_view[line];
		if (h.empty()) {
			++line;
			continue;
		}

		error_helper::line elh2(first_line + line);

		let p = h.find_first_of('=');
		if (p == h.npos) {
			ims_error("unrecognized variable");
			return false;
		}

		//everything from the beginning of the line to p should be the id variable
		std::string_view sv = h.substr(0, p);
		trim_spaces(sv);

		parsed_var v;
		v.line7 = line;
	

		read_id(v.name, sv);

		if (!sv.empty() || v.name.empty()) {
			ims_error("invalid variable name");
			return false;
		}

	
		h = h.substr(p + 1);
		skip_spaces(h);

		//look for the end of the variable body
		//either ';' or a string containing '='
	
		for (; line < m_source_view.size(); ++line) {
			auto& q = m_source_view[line];
			if (q.empty()) {
				continue;
			}
			

			let spos = q.find_first_of(';');
			let epos = q.find_first_of('=');

			if (epos < spos) {
				break;
			}

			//do not allow concatenation of identifiers from different lines
			if (!v.val.empty() &&
				is_var_id_sym(v.val.back()) &&
				is_var_id_sym(q.front()))
			{
				break;
			}


			if (spos != q.npos) {
				v.val.append(q.data(), spos);
				q = q.substr(spos + 1);
				break;
			}

			//add the entire line to the body
			if (!v.val.empty())v.val.push_back('\n');//multi-line support
			v.val.append(q.data(), q.size());

		};

		if (v.val.empty()) {
			ims_error("undefined variable {}", v.name);
			return false;
		}

		m_vars.emplace_back(std::move(v));
	}

	
	ret.status = read_state::parse_result::e_continue;
	return true;
}





static std::string_view get_var_name(size_t idx, ims_identifiers& idf)
{
	if (ref_is_builtin(idx)) {
		return get_builtin_name(ref2builtin(idx));	
	}else {
		return idf.get_str_from_unk(idx);
	}
}

bool read_state::process_string_vars(
	ifs_list& lst,
	oper_block& b,
	size_t& line_name,
	size_t& line_attr)
{

	m_vars_duplicates.clear();

	m_pfo.m_pos2 = 0;
	assert(b.m_ops.empty());

	//here, in particular, all variable names are indexed
	for (auto& q : m_vars) {
		if (q.name.empty()) {
			continue;//handle it in JS
		}

		if (!pre_process_variable(lst, q, b, line_name, line_attr)) {
			return false;
		}
	}

	//parse definitions
	for (auto& q : m_vars) {
		if (q.name.empty()) {
			continue;//handle it in JS
		}

		if (!process_variable(lst, q, b)) {
			return false;
		}

		if (q.index7 != ims_max) {
			m_vars_duplicates.emplace_back(q.index7, q.line7);
		}
	}

	std::sort(m_vars_duplicates.begin(), m_vars_duplicates.end());
	auto it = std::adjacent_find(m_vars_duplicates.begin(), m_vars_duplicates.end());
	if (it != m_vars_duplicates.end()) {
		++it;//first duplicate
		ims_error("Duplicate declaration of {}", 
			get_var_name(it->idx, lst.m_idf));
		return false;
	}


	return true;
}
std::string read_state::parse_expression_as_aifs(
	ims_identifiers& unk,
	const std::string str, 
	oper_block& b, 
	size_t offset)
{
	m_pfo.err.str("");
	m_pfo.unk = &unk;
	

	std::string ret_err;

	try {
		if (!m_pfo.parse7(str, b, offset)) {
			ret_err = m_pfo.err.str();
			if (ret_err.empty())ret_err = "Unknown error";
		}
	}
	catch (const std::exception& e) {
		ret_err = e.what();
		if (ret_err.empty())ret_err = "Unknown error";
		
	}

	return ret_err;
}

