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


namespace ims_keywords
{
static constexpr char const

*name = "$n",
*attrib = "$a",
*dim = "$dim",
*timestamp = "$ts",
*empty = "$e",
*marker = "$",
*parent = "$p",
*str_id = "$id",
*subspace = "$subspace",
*camera = "$camera",
*convert_to = "$convert_to",
*section = "$section",
*root = "$root",
*palette = "$palette",
*background = "$background",
*time = "$time",
*light = "$light",
*colorize = "$colorize",
*condition = "if",
*end = "end",
*nlc = "\n",
*js_delimeter = "@@",
*js_init = "$init",
*js_info = "$info",
*js_export_blocks = "$aifs",

*autoprefix = "_",
*version = "version",//ignored

subst = '&',
builtin = '$',
block = '@',
base = ':',
comment = '#',

*search_params_block = "IFStile";
};
