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
#include "samples.h"
#include "ims_file.h"
#include "embed/examples.h"

samples::samples()
{
#define ME(title, arr) {title, "", embed::arr, sizeof(embed::arr)}

	m_samples =
	{
		{ "Recent", {} },

		{ "2D Tiles", {
		ME("Square", square),
		ME("Ammann_A3", ammann_a3),
		ME("Ammannn_Beenker", ammannn_beenker),
		ME("IR3-211111-90", ir3_211111_90),
		ME("IR4-31111111-90", ir4_31111111_90),
		ME("IR4-32111-60", ir4_32111_60),
		ME("IR5-32221111-90", ir5_32221111_90),
		ME("Misc", misc),
		ME("x^2+2", x_2_2),
		ME("x^2-2x+2_Twin_Dragon", x_2_2x_2_twin_dragon),
		ME("x^2-3x+3_Koch_Snowflake", x_2_3x_3_koch_snowflake),
		ME("x^2-4x+5_Pinwheel", x_2_4x_5_pinwheel),
		ME("x^2-5x+7_Gosper_Island", x_2_5x_7_gosper_island),
		ME("x^2-x+2_Tame_Twin_Dragon", x_2_x_2_tame_twin_dragon),
		ME("x^3+2x-1", x_3_2x_1),
		ME("x^3+3x-1", x_3_3x_1),
		ME("x^3+x^2-1", x_3_x_2_1),
		ME("x^3+x-1", x_3_x_1),
		ME("x^3-2x^2+3x-1", x_3_2x_2_3x_1),
		ME("x^3-2x^2+x+1", x_3_2x_2_x_1),
		ME("x^3-3x^2+4x-1", x_3_3x_2_4x_1),
		ME("x^3-x^2+2x+1[1]", x_3_x_2_2x_1_1_),
		ME("x^3-x^2+2x-1[2]", x_3_x_2_2x_1_2_),
		ME("x^3-x^2+x+1", x_3_x_2_x_1),
		ME("x^4+x^2-1_Golden", x_4_x_2_1_golden),
		ME("x^4+x+1", x_4_x_1),
		ME("x^4-2x^3-x^2+2x+1_Robinson", x_4_2x_3_x_2_2x_1_robinson),
		ME("x^4-2x^3-x^2+2x+1_Robinson_90", x_4_2x_3_x_2_2x_1_robinson_90),
		ME("x^4-x^3+1", x_4_x_3_1),
		ME("[3a.b-2a.b][x+1][C12]", _3a_b_2a_b__x_1__c12_),
		} },

		{ "3D Tiles", {
		ME("x^3-2", x_3_2),
		ME("x^3-N", x_3_n),
		ME("Cube", cube),
		ME("Tesseract", tesseract),
		ME("Quaquaversal", quaquaversal),
		}},

		{ "Examples", {
		ME("Barnsley fern",barnsley_fern),
		ME("Pentadentrite",pentadentrite),
		ME("IR3",ir3),
		ME("PedalTriangle",pedaltriangle),
		ME("Cantor set",cantor_set),
		ME("Segment",segment),
		ME("Cantorval",cantorval),
		ME("Jerusalem cross",jerusalem_cross),
		ME("4Gen",_4gen),
		ME("[3D]_Simple",_3d__simple),
		ME("[3D]_Trees",_3d__trees),
		ME("[3D]_SelfSim2",_3d__selfsim2),
		ME("[3D]_Sierpinski",_3d__sierpinski),
		ME("[3D]_Various",_3d__various),
		}},

		{ "Animation", {
		ME("Animation",animation),
		ME("[3D]_Animation",_3d__animation),
		}},

		{ "JavaScript", {
		ME("basic.js",basic_js),
		ME("disk_like.js", disk_like_js),
		ME("julia.js",julia_js),
		ME("trees.js",trees_js),
		}},
	};
}

void samples::add_recent(std::string_view filename, bool to_front)
{
	auto path = ims_file::adjust(filename);

	std::string name;

	let pos= path.find_last_of('/');

	if (pos == std::string::npos) {
		return;//only with full path
	}
	
	name = path.substr(pos + 1);
	

	////////////////////
	
	if (name.size()>5 && name.substr(name.size() - 5, 5) == ".aifs") {
		name = name.substr(0, name.size() - 5);
	}

	if (pos != std::string::npos) {
		name += ": ";
		name += path.substr(0, pos);
	}
	
	////////////////////

	auto& rec = get_recent().smp;

	if (!to_front) {
		//easy way
		rec.push_back({ name ,path });
		return;
	}

	remove_recent(path);
	
	rec.insert(rec.begin(), { name ,path });	

	if (rec.size() > s_max_recent) {
		rec.resize(s_max_recent);
	}
}


void samples::remove_recent(std::string_view filename)
{
	ims_erase(get_recent().smp, [filename](let& e) {
		return e.path == filename;
	});
}
