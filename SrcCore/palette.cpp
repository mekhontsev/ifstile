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
#include "palette.h"
#include "ims_num_traits.h"
#include "ims_random.h"

//sequentially reads integers from a character stream
namespace number_parser
{

	//loads the fragment to the end of the line
	template<typename Iter>
	void load_line(Iter& iter, const Iter& end, std::string& dst)
	{
		dst.clear();

		for (;;) {
			if (iter == end) {
				return;
			}

			let c = *iter++;

			if (c == '\n') {
				return;//ready
			};

			if (c != '\r') {
				dst += c;
			};
		}

	};


	static bool is_delim(char c)
	{
		const char delim[] = " ,;\t";
		for (auto d : delim)if (c == d)return true;
		return false;
	};

	static bool is_eol(char c)
	{
		return c == '\r' || c == '\n';
	};

	static bool is_comment(char c)
	{
		return c == '#';
	}

	struct comment_info
	{
		std::string same;//comment on the same line
		std::vector<std::string> other;//comments in the next lines
	};

	//skip spaces and comments
	//returns false if we reach the end of the line
	//can collect all comments
	template<typename Iter>
	static bool next_token(Iter& it, const Iter it_end,
		comment_info* com = nullptr)
	{
		bool same_line = true;
		if (com) {
			com->same.clear();
			com->other.clear();
		}
		while (it != it_end) {
			//skip spaces and separators
			for (;;) {
				if (is_eol(*it)) {
					same_line = false;
					++it;
				} else if (is_delim(*it)) {
					++it;
				} else {
					break;
				}
				if (it == it_end)return false;
			}

			if (!is_comment(*it)) {
				return true;//found something interesting
			}

			++it;

			if (!same_line && com) {
				com->other.emplace_back();
			}

			//skip the comment
			while (!is_eol(*it)) {
				if (com) {
					if (same_line) {
						com->same += *it;
					} else {
						com->other.back() += *it;
					}
				}
				++it;
				if (it == it_end)return false;
			};
		}
		return false;
	}



	//get a real number from the stream
	template<typename Number, typename Iter>
	bool parse(Number& dst, Iter& it, const Iter& it_end)
	{
		if (!next_token(it, it_end)) {
			return false;
		}

		std::string str;

		for (;
			it != it_end &&
			!is_delim(*it) &&
			!is_comment(*it) &&
			!is_eol(*it);
			++it)
		{
			str += *it;
		}

		assert(!str.empty());

		return boost::conversion::try_lexical_convert(str, dst);
	}


}



void palette::reset()
{
	constexpr size_t sz = 16;

	float arr[sz][3] =
	{
		{ 0.9375,0.9375,  0.9375,},
		{1, 1, 0.25},
		{ 0.25, 0.5, 1},
		{ 1, 0.25, 0.25},
		{ 0.25, 1, 0.25},
		{ 0.25, 1, 1},
		{ 0.75, 0.25, 1},
		{ 1, 0.5, 0.25},
		{ 0.75, 0.5, 1},
		{ 1, 0.5, 0.5},
		{ 0.25, 1, 0.5},
		{ 0.75, 1, 0.5},
		{ 1, 0.75, 0.5},
		{ 0.5, 0.5, 1},
		{ 0.25, 0.25, 1},
		{ 0.25, 0.75, 1},
	};

	clear();
	data.resize(sz);

	for (size_t i = 0; i < sz; ++i) {
		let& c = arr[i];
		auto& d = data[i];
		d.c[0] = c[0];
		d.c[1] = c[1];
		d.c[2] = c[2];
		d.c[3] = 1;
	};
}

void palette::sort()
{
	std::stable_sort(data.begin(), data.end(), [](let& e1, let& e2) {
		return e1.checked_p && !e2.checked_p;
	});
}

void palette::save_hex_rgb(std::ostream& s, size_t pad) const
{
	std::vector<uint8_t> arr;
	for (let& q : data) {
		for (size_t i = 0; i < 3; ++i) {
			arr.push_back(to8bit(q.c[i]));
		}
	}
	arr.resize(pad * 3, 0);
	boost::algorithm::hex(arr.begin(), arr.end(),
		std::ostream_iterator<uint8_t>{s, ""});
}

bool palette::load_gimp(std::istream& file)
{
	auto iter = std::istreambuf_iterator<char>(file);
	let end = std::istreambuf_iterator<char>();

	clear();

	std::string buf;

	//skip empty lines
	for (;;) {
		if (iter == end)return false;
		number_parser::load_line(iter, end, buf);
		boost::algorithm::trim(buf);
		if (!buf.empty()) {
			if (buf == "GIMP Palette") {
				break;
			}
			return false;
		}
	}



	while (iter != end) {
		number_parser::load_line(iter, end, buf);
		boost::algorithm::trim(buf);

		if (buf == "#") {
			break;
		}

		if (buf.substr(0, 5) == "Name:") {
			name = buf.substr(5, buf.length() - 5);
			boost::algorithm::trim(name);
		};

	}

	while (iter != end) {
		uint16_t r = 0, g = 0, b = 0, a=0;
		bool res =
			number_parser::parse(r, iter, end) &&
			number_parser::parse(g, iter, end) &&
			number_parser::parse(b, iter, end);

		if (!res) {
			break;//read as far as we could
		}

		//format extension - alpha support
		if (!number_parser::parse(a, iter, end)) {
			a = 255;
		}

		number_parser::load_line(iter, end, buf);
		boost::algorithm::trim(buf);
		data.push_back({ { from_u(r),from_u(g),from_u(b),1 } ,buf,true });
	}
	return true;
}


void palette::save_gimp(std::ostream& s) const
{
	s << "GIMP Palette" << "\r\n";
	s << "Name: " << (name.empty() ? "IFStile" : name.c_str()) << "\r\n";
	s << "#" << "\r\n";
	for (let& q : data) {
		for (let& c : q.c) {
			let u = to8bit(c);
			if (u < 10)s << " ";
			if (u < 100)s << " ";
			s << unsigned(u) << " ";
		}
		if (!q.name.empty()) {
			s << q.name << " ";
		}
		s << "\r\n";
	}
}


void palette::resize(size_t sz)
{

	if (sz <= data.size()) {
		data.resize(sz);
	} else {
		update(sz);
	}
}

float palette::get_max_brighness() const
{
	float m = 0;
	for (let& q : data) {
		m = std::max(m, q.get_max());
	}
	return m;
}

float palette::get_min_brighness() const
{
	float m = 1;
	for (let& q : data) {
		m = std::min(m, q.get_min());
	}
	return m;
}

void palette::randomize(size_t from)
{
	auto min_b = std::max(get_min_brighness(), 0.0f);
	auto max_b = std::min(get_max_brighness(), 1.0f);
	if (max_b <= min_b) {
		min_b = 0;
		max_b = 1;
	}

	auto& rng = ims_random::getR().rng;
	std::uniform_real_distribution<float> distr(min_b, max_b);

	for (size_t i = from; i < data.size(); ++i) {
		auto& q = data[i];
		if (!q.checked_p)continue;

		let idx0 = rng() % 3;
		let idx1 = (idx0 + 1 + (rng() % 2)) % 3;
		let idx2 = 3 - (idx0 + idx1);

		q.c[idx0] = min_b;
		q.c[idx1] = max_b;
		q.c[idx2] = distr(rng);
	};
}

// Convert hsv floats ([0-1],[0-1],[0-1]) to rgb floats ([0-1],[0-1],[0-1]), from Foley & van Dam p593
// also http://en.wikipedia.org/wiki/HSL_and_HSV
static void ColorConvertHSVtoRGB(
	float h, float s, float v, 
	float& out_r, float& out_g, float& out_b)
{
	if (s == 0.0f)
	{
		// gray
		out_r = out_g = out_b = v;
		return;
	}

	h = fmodf(h, 1.0f) / (60.0f / 360.0f);
	int   i = (int)h;
	float f = h - (float)i;
	float p = v * (1.0f - s);
	float q = v * (1.0f - s * f);
	float t = v * (1.0f - s * (1.0f - f));

	switch (i)
	{
	case 0: out_r = v; out_g = t; out_b = p; break;
	case 1: out_r = q; out_g = v; out_b = p; break;
	case 2: out_r = p; out_g = v; out_b = t; break;
	case 3: out_r = p; out_g = q; out_b = v; break;
	case 4: out_r = t; out_g = p; out_b = v; break;
	case 5: default: out_r = v; out_g = p; out_b = q; break;
	}
}

static std::uniform_real_distribution<float> get_dist(float v0, float v1)
{
	if (v0 > v1)std::swap(v0, v1);
	return std::uniform_real_distribution<float>(v0, v1);
}

void palette::randomize(
	const float* H, 
	const float* S, 
	const float* V, 
	const float* A)
{
	auto dH = get_dist(H[0], H[1]);
	auto dS = get_dist(S[0], S[1]);
	auto dV = get_dist(V[0], V[1]);
	auto dA = get_dist(A[0], A[1]);

	auto& rng = ims_random::getR().rng;

	for (size_t i = 0; i < data.size(); ++i) {
		auto& q = data[i];
		if (!q.checked_p)continue;

		let fH = dH(rng);
		let fS = dS(rng);
		let fV = dV(rng);
		let fA = dA(rng);

		ColorConvertHSVtoRGB(fH, fS, fV, q.c[0], q.c[1], q.c[2]);
		q.c[3] = fA;
	}
}

void palette::update(size_t sz)
{
	size_t old_sz = data.size();
	if (sz <= old_sz)return;

	if (sz > max_palette)sz = max_palette;
	data.reserve(max_palette);

	entry e;
	if (old_sz > 0)	e = data[0];
	else e.c[3] = 1;//opaque
	data.resize(sz, e);

	randomize(old_sz);
}

//x from 0 to the palette size, i.e. color index
void palette::interpolate(color& dst, double x) const
{
	if (x < 0)x = 0;

	let fx = floor(x);
	let dx = x - fx;

	let ix = (size_t)fx;

	let i1 = adjust_direct(ix);
	let i2 = adjust_direct(ix + 1);

	let& c1 = data[i1].c;
	let& c2 = data[i2].c;


	let k1 = 1 - dx;
	let k2 = dx;

	let f1 = k1 * c1[3];
	let f2 = k2 * c2[3];

	let s2 = f1 + f2;
	if (s2 == 0) {
		dst[0] = dst[1] = dst[2] = 0;
	} else {
		dst[0] = float((c1[0] * f1 + c2[0] * f2) / s2);
		dst[1] = float((c1[1] * f1 + c2[1] * f2) / s2);
		dst[2] = float((c1[2] * f1 + c2[2] * f2) / s2);
	}

	
	dst[3] = float((c1[3] * k1 + c2[3] * k2));
}

size_t palette::adjust(size_t idx, size_t sz)
{
	if (sz == 1)return 0;
	if (idx < sz)return idx;
	return 1 + ((idx - sz) % (sz - 1));
};


size_t palette::adjust_direct(size_t idx) const
{
	let sz = data.size();
	return idx % sz;
}

float palette::entry::get_max() const
{
	float m = c[0];
	for (size_t j = 1; j < 3; ++j) {
		m = std::max(m, c[j]);
	}
	return m;
}

float palette::entry::get_min() const
{
	float m = c[0];
	for (size_t j = 1; j < 3; ++j) {
		m = std::min(m, c[j]);
	}
	return m;
}

