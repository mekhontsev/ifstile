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

struct background 
{
	//the first 3 parameters are color, the fourth is fog density
	std::array<float, 4> data = {0,0,0,0};
	void reset() { data[0] = data[1] = data[2] = data[3] = 0; };

	float& get_fog() { return data[3]; }
	float get_fog() const { return data[3]; }
};

struct palette 
{
	using color = std::array<float, 4>;
	std::string name;

	struct entry
	{
		color c;
		std::string name;
		bool checked_p=true;
		float get_max() const;
		float get_min() const;
	};

	std::vector<entry> data;

	////////////////////////////////////////////////////////////////////////////


	
	void save_gimp(std::ostream& s) const;


	void reset();

	void sort();

	
	void save_hex_rgb(std::ostream& s, size_t pad) const;

	
	bool load_gimp(std::istream& file);

	void clear() 
	{
		data.clear();
		name.clear();
	}

	bool empty() const
	{
		return data.empty();
	}


	template<typename U>
	static float from_u(U c)
	{
		return float(c) / 255;
	}

	template<typename Real>
	static uint8_t to8bit(Real c)
	{
		c = std::round(c * 255);
		if (c < 0)c = 0;
		if (c > 255)c = 255;
		return static_cast<uint8_t>(c);
	}

	static const size_t max_palette = 65536;

	//maximum brightness
	float get_max_brighness() const;
	float get_min_brighness() const;

	void resize(size_t sz);
	void randomize(size_t from);
	
	void randomize(
		const float* H, 
		const float* S, 
		const float* V, 
		const float* A);

	void update(size_t sz);

	void interpolate(color& dst, double x) const;

	static size_t adjust(size_t idx, size_t pal_size);
	size_t adjust(size_t idx) const { return adjust(idx, data.size()); };
	

	size_t adjust_direct(size_t idx) const;

	entry& get(size_t idx){	return data[adjust(idx)];};
	const entry& get(size_t idx) const  { return data[adjust(idx)]; };


};

struct colorize_params
{
	using additional=std::vector<double>;

	enum EPAR: size_t
	{
		e_edge = 0,			//+depth
		e_vertex = 1,		//+depth
		e_field_lines = 2,	//+power
		e_equipotential = 3,//+power + magnitude
		e_numpar = 4,
	};

	EPAR type = e_numpar;
	double shift = 0;//cyclic shift of the palette
	additional params;

	double get_depth() const
	{
		if (!is_tiling())return 1;
		let& p = params[0];
		return p > 0 ? p : 0;
	}

	static void set_default(EPAR def, additional& dst)
	{
		switch (def) {
		case e_edge:
			dst.resize(1);
			dst[0] = 0.1;
			break;
		case e_vertex:
			dst.resize(1);
			dst[0] = 0.1;
			break;
		case e_field_lines:
			dst.resize(1);
			dst[0] = 3;
			break;
		case e_equipotential:
			dst.resize(2);
			dst[0] = 3;
			dst[1] = 20;
			break;
		default:
			dst.resize(0);
			assert(false);
			return;
		}
	};

	static bool is_field(EPAR t)
	{
		return t == e_field_lines || t == e_equipotential;
	};

	static bool is_tiling(EPAR t)
	{
		return t == e_vertex || t == e_edge;
	};

	bool is_field() const
	{
		return is_field(type);
	};

	bool is_tiling() const
	{
		return is_tiling(type);
	};

	

	void reset()
	{
		type = e_edge;
		set_default(type, params);
		shift = 0;
	};


	void clear()
	{
		type = e_numpar;
		shift = 0;
		params.clear();
	};

	bool empty() const
	{
		return type == e_numpar;
	}
};