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
#include "render_tester.h"
#include "ifs_renderer.h"


TEST(testRender, RenderSquare)
{
	std::string aifs{ R"(
@G4
$a=h
$dim=2
$subspace=[s, 0]
s=$companion([1,0])
r=$companion([-1,0])
&R=$semigroup([s,r])
&T=R*$vector()
h0=R
h1=T
h2=T
h3=T
A=2^-1*(h0|h1|h2|h3)*A

@:G4
h0=1
h1=1*[1,0]
h2=1*[0,1]
h3=1*[1,1]

)" };

	ifs_renderer r;

	EXPECT_TRUE(r.init(aifs));

};
