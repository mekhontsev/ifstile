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
#include "parser_state.h"


struct block_class;
struct ifs_list;

bool is_var_id_sym(char c);

void WARN_DUP_ID(std::string_view id);

struct parsed_var
{
	std::string name;

	std::string val;

	size_t js_val = ims_max;//index in js_aifs_block::m_js_vars

	//unique identifier within the block
	size_t index7 = ims_max;

	bool is_subst = false;
	bool from_js = false;

	//on which line does the definition begin
	size_t line7 = ims_max;
};

//indices of all variables in the graph - to search for duplicates
struct var_idx
{
	var_idx(size_t i, size_t l) :idx((uint32_t)i), line((uint32_t)l) {};

	bool operator<(const var_idx& v) const {
		if (idx < v.idx)return true;
		if (idx > v.idx)return false;
		return line < v.line; //to ensure stable sorting
	};
	bool operator==(const var_idx& v) const { return idx == v.idx; };

	uint32_t idx;
	uint32_t line;
};


struct read_state
{

	parser_state m_pfo;

	std::string m_id;

	std::string m_convert_to;

	std::vector<parsed_var> m_vars;
	std::vector<var_idx> m_vars_duplicates;

	std::vector<std::string_view> m_source_view;


	std::vector<std::string> m_source_lines;
	size_t m_source_num_lines;//how much is relevant in m_source_lines
	

	void clear() 
	{
		m_id.clear();
		m_convert_to.clear();
		m_vars.clear();
		m_vars_duplicates.clear();
		m_source_view.clear();

		m_source_num_lines = 0;//don't touch m_source_lines
	}

	//returns false if an error occurs
	[[nodiscard]]
	bool process_string_vars(
		ifs_list& lst,
		oper_block& b,
		size_t& line_name,
		size_t& line_attr);	

	//returns an error message or an empty string
	std::string parse_expression_as_aifs(
		ims_identifiers& unk,
		const std::string str, 
		oper_block& b, 
		size_t offset);

	//read id
	//id is any sequence of Latin letters and numbers, underscores, and dollars
	static void read_id(std::string& dst, std::string_view& s);//s changes

	//remove spaces at the beginning
	static void skip_spaces(std::string_view& s);//s changes


	//remove spaces at the beginning and end
	static void trim_spaces(std::string_view& s);//s changes


	
	bool pre_process_variable(
		ifs_list& lst,
		parsed_var& q,
		oper_block& b,
		size_t& line_name,
		size_t& line_attr);

	//fills the index of each variable
	//process some built-in variables, like $dim
	bool process_variable(
		ifs_list& lst,
		parsed_var& q, 
		oper_block& b);

	struct parse_result
	{
		enum {
			e_continue,
			e_ignore,
			e_completed,
		} status = e_continue;

		bool keep_source = false;//save the text in the oper_block

		std::string block_id;
		std::string parent_id;
	};

	
	 bool parse_block(
		ifs_list& lst,
		parse_result& res,
		size_t first_line
	);


};

//as a result, each JS line is guaranteed to end with \n
//returns the string with which JS begins or 0
size_t js_from_stream(
	std::string& dst, 
	size_t& cur_line, 
	std::istreambuf_iterator<char>& it_beg, 
	const std::istreambuf_iterator<char>& it_end);


//read and add the next block from the stream to the list
//returns false in case of a critical error
[[nodiscard]]
bool aifs_from_stream_ex(
	ifs_list& lst,
	size_t& cur_line,
	read_state& rs,
	std::istreambuf_iterator<char>& it,
	const std::istreambuf_iterator<char>& end);
