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

struct oper_block;
struct oper_block_flags;
struct ifs_list;
struct ast_stack;


void ims_write_block(std::ostream& str, const oper_block* b);


struct depends_enumerator_base
{

};

struct aifs_printer : public depends_enumerator_base
{
#ifdef NDEBUG
	template<typename T1, typename T2>
	using umap = ankerl::unordered_dense::map<T1, T2>;

	template<typename T>
	using uset = ankerl::unordered_dense::set<T>;
#else
	template<typename T1, typename T2>
	using umap = std::unordered_map<T1, T2>;

	template<typename T>
	using uset = std::unordered_set<T>;
#endif

	//don't have a string ID, but someone is referencing them
	umap<const oper_block*, std::string> uds_need_id;

	std::vector<const oper_block*> arr_need_id;
	std::string_view get_temp_id(const oper_block* b) const;

	//extended version of ims_write_block
	void write_block(
		std::ostream& dst,
		const oper_block* b,
		oper_block_flags f,
		bool ignore_priv);

	void clear();

	//ignores duplicates
	void add_block(const oper_block* p);


	//all added blocks
	uset<const oper_block*> uds;

	//all blocks, without duplication, in the order of addition
	std::vector<const oper_block*> arr;

	size_t prepare(
		const ifs_list& lst,
		std::span<const oper_block*> arr2,
		bool only_checked,
		bool ignore_js);


	//returns the number of saved
	size_t ims_to_text(
		std::ostream& str,
		const ifs_list& lst,
		//only if we're saving a specific list
		//if it's empty, we save everything from ifs_list
		std::span<const oper_block*> arr,
		//true to save only selected ones
		bool only_checked,
		//true to hide all but arr
		bool hide_other,
		//ignore temporary parent values from JS, i.e., elevate_priv them
		//necessary, for example, for javascript->disk_like when printing NOT to the console
		//almost always true except, for example, when printing to the console
		bool ignore_js
	);

private:

	void add_depends(const ifs_list& lst, ast_stack& ai, bool ignore_priv);
	void add_dep_block(const oper_block* b);
};
