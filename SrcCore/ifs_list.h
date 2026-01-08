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
#include "ims_identifiers.h"
#include "oper_block.h"

//all information about the current file
struct ifs_list : public boost::noncopyable
{
	ims_identifiers m_idf;

	//all live blocks
	std::vector<block_id_t> m_blocks;

	struct data
	{
		//can be nullptr - on load, or if not found or deleted
		std::unique_ptr<oper_block> b;

		//index m_idf.idx2m_unknown
		size_t m_str_id = ims_max;

		void clear();
	};

	//all blocks indexed by id
	//the array may contain empty entries corresponding to m_free_blocks
	std::vector<data> m_id2data;

	
	//all dead (deleted id) - holes m_id2data
	std::vector<block_id_t> m_free_blocks;

	////////////////////////////////////////////////////////////////////////////

	//m_id2data[ret] will be filled somewhere else
	//let's leave m_blocks alone, let someone else do it
	[[nodiscard]]
	block_id_t alloc_block_entry();

	//don't touch m_blocks, let someone else do it.
	//clear m_id2data (delete the block itself)
	void release_block_entry(block_id_t id);

	////////////////////////////////////////////////////////////////////////////
	bool empty() const { return m_blocks.empty(); };
	size_t size() const { return m_blocks.size(); };
	//returns or creates a block id via string str_id
	block_id_t insert_by_str_id(std::string_view str_id);

	//places a block
	block_id_t move_block(
		std::unique_ptr<oper_block>& src, 
		std::string_view str_id);

	
	oper_block* get_block(block_id_t id) const;
	oper_block* get_block_from_unk(size_t unk_id) const;

	std::string_view get_str(block_id_t id) const;

	//create an empty block with the given id, replacing the existing one
	oper_block* add_block(std::string_view str_id);

	oper_block* find_block(std::string_view str_id) const;

	//O(n)
	oper_block* find_block2(
		std::string_view str_id, 
		std::string_view name) const;

	//get via live index m_blocks
	oper_block* get_block_by_idx(size_t idx) const;
	
};
