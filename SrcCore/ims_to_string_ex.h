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
struct ims_graph_base;
struct palette;
struct ims_graph;
struct edge_map;

template<typename Real>
struct screen_disk;

template<typename Real>
struct camera;

void ims_to_fractracer(
	std::ostream& str,
	size_t dim,
	const ims_graph_base& gm,
	const camera<double>& cam,
	size_t ver,
	std::span<const edge_map> ri,
	const palette& pal);

struct  flame_edge;

void ims_to_flame(
	std::ostream& str,
	size_t dim,
	std::string_view name,
	std::span<const flame_edge> fedegs,
	const size_t ver,
	std::span<const edge_map> ri,
	const screen_disk<double>& sd,
	
	const palette& pal,
	const std::array<float, 4>& background
);

#ifndef NDEBUG
void ims_to_x3d(const std::string& path, const ifs_list& lst);
#endif // NDEBUG

