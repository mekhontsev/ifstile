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
#include "conbuf.h"

void ims_num_traits_init_all();


ims_static ims_conbuf g_conbuf;
void ext_console_sync(std::string& str)
{
	g_conbuf.sync_string(str);
}
void ext_console_clear()
{
	g_conbuf.clear();
};

bool ims_need_stop()
{
	return false;
}

int main_utf8(int argc, char** argv)
{
	ims_num_traits_init_all();
	g_conbuf.redirect(std::cout);
	g_conbuf.redirect(std::cerr);
	printf("Running main() from %s\n", __FILE__);
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
