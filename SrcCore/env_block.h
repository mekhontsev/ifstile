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

struct env_block_data
{
	struct columns* cols;
	struct finder* fparams;
	struct render_params* rparams;

	static bool s_use_fparams;
	static bool s_use_rparams;

};

struct oper_block;
struct ims_identifiers;

void set_env_block(ims_identifiers& idf, oper_block& b, const env_block_data& ebd);
bool load_env_block(const oper_block* xb, env_block_data& ebd);