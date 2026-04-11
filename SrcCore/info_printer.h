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
struct block_info;
struct eval_context;

void print_ifs_data(const oper_block& sr);
void print_ifs_def(const oper_block& sr);
void print_normal_maps(const oper_block& sr, eval_context& ec);
void print_ifs_eval(const oper_block& sr, eval_context& ec);
bool print_dimensions(std::ostream& sout, const block_info& bi);
void print_balls(const oper_block& sr, const block_info* bi);
void print_diams(const oper_block& sr, const block_info* bi);
void print_measure(const oper_block& sr, const block_info* bi);
void print_ifs_proj(const oper_block& sr, const block_info& bi);
void print_components(const oper_block& sr, const block_info& bi);
void print_ast(const oper_block& sr, const block_info& bi);
void print_subspaces(const oper_block& sr, const block_info* bi);
