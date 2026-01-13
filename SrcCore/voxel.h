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


//bit_depth - number of bits to store one coordinate
template <uint32_t bit_depth>
struct voxel_t
{
	enum {
		//bit_depth=10,
		bit_shift=1<<bit_depth,
	};

	unsigned x1: bit_depth;//0=[0,1/31], 1=[1/16,2/32],30=[30/31,1], 31-infinity
	unsigned x2: bit_depth;//the same as x1, only in the decreasing direction
	unsigned y1: bit_depth;
	unsigned y2: bit_depth;
	unsigned z1: bit_depth;
	unsigned z2: bit_depth;
	

	void clear()
	{
		x1=x2=y1=y2=z1=z2=bit_shift-1;//1 in all directions
	}

	static double get_dist(unsigned c){
		//if (c==31)return 1;
		//return (c+1)*(1.0/32);
		return c*(1.0/(bit_shift-1));
	};

	double xp(){return  get_dist(x1);};
	double xm(){return 1-get_dist(x2);};
	double yp(){return  get_dist(y1);};
	double ym(){return 1-get_dist(y2);};
	double zp(){return  get_dist(z1);};
	double zm(){return 1-get_dist(z2);};

	
	//update the code based on the old code and the new distance
	//returns the new code (the minimum of the old code and the one calculated based on dist)
	inline static unsigned update(unsigned c, double dist, bool& updated)
	{
		unsigned nc=unsigned(dist*(bit_shift-1));
		if (nc>(bit_shift-1)){
			nc=bit_shift-1;
		}

		//return the minimum
		if (nc<c){
			updated=true;
			return nc;
		}

		return c;//not updated
	};

	inline static unsigned get(double dist)
	{
		unsigned nc=unsigned(dist*(bit_shift-1));
		if (nc>(bit_shift-1)){
			nc=bit_shift-1;
		}
		return nc;
	};
};



