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


struct draw_task
{
	bool
		paint_2d_std : 1,
		paint_2d_ext : 1,
		paint_3d : 1,
		build_2d_std : 1,
		build_2d_ext : 1,
		build_3d : 1,
		ssao_3d : 1,
		fit : 1,//automatically means rebuild
		use_low_res : 1;


	bool changed() const
	{
		return
			paint_2d_std || paint_2d_ext || paint_3d ||
			build_2d_std || build_2d_ext || build_3d || fit;
	};

	draw_task(bool v = false) { set_all(v); };

	bool is_need_build() const
	{
		return build_2d_ext || build_2d_std || build_3d;
	};

	void rebuild()
	{
		build_2d_std = true;
		build_2d_ext = true;
		build_3d = true;
	}

	void set_all(bool v)
	{
		paint_2d_std = v;
		paint_3d = v;
		paint_2d_ext = v;
		build_2d_std = v;
		build_3d = v;
		build_2d_ext = v;
		ssao_3d = v;
		use_low_res = false;
		fit = false;
	};
};