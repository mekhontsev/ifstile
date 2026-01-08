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


struct exr_data
{
	void init(size_t width, size_t height, size_t num_chan);
	void set_num_chan(size_t nc);
	void set_channel(size_t idx, float* pixels, const char* chan_name);
	void set_channel(size_t idx, uint8_t* pixels, const char* chan_name);
	bool save(std::ostream& fs);

private:

	struct channel 
	{
		std::string name;
		void* image=nullptr;
		bool is_float=false;
	};

	size_t w = 0, h = 0;

	std::vector<channel> m_img;	

};

