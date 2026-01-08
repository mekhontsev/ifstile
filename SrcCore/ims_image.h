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


//It is forbidden to write code that copies images
//Pixel must be a trivial type
template<typename Pixel>
struct ims_image: public boost::noncopyable
{

	size_t w() const { return m_w; };
	size_t h() const { return m_h; };

	~ims_image() { free_image(); };

	void free_image()
	{
		static_assert(std::is_trivial<Pixel>::value);

		if (!m_data)return;

		std::free(m_data);

		m_data = nullptr;
		m_w = m_h = m_capacity = 0;
	}

	
	//can reset image dimensions
	void reserve(size_t sz) noexcept
	{
		if (sz <= m_capacity) {
			return;
		}

		let old_capacity = m_capacity;

		free_image();

		//increase the size exponentially
		sz = std::max(old_capacity * 3 / 2, sz);

		let sz_bytes = sz * sizeof(Pixel);

		m_data = (Pixel*)std::malloc(sz_bytes);

		if (m_data) {
			m_capacity = sz;
		}else {
			ims_error("Failed to allocate {} bytes", sz_bytes);
			m_capacity = 0;
		}
	}
	

	void recreate(size_t w, size_t h)
	{
		reserve(w * h);

		if (m_data) {
			m_w = w;
			m_h = h;
		} else {
			m_w = 0;
			m_h = 0;
		}
	}


	bool empty() const { return m_w == 0 || m_h == 0; };

	Pixel* data() { return m_data; };
	const Pixel* data() const { return m_data; };


	Pixel& get_pixel(size_t x, size_t y)
	{
		return m_data[get_index(x, y)];
	}

	Pixel& operator() (size_t x, size_t y)
	{
		return get_pixel(x,y);
	}


	const Pixel& get_pixel(size_t x, size_t y) const
	{
		return m_data[get_index(x, y)];
	}

	const Pixel& operator()(size_t x, size_t y) const
	{
		return get_pixel(x,y);
	}

	Pixel& operator()(size_t idx) 
	{
		assert(idx < m_capacity);
		return m_data[idx];
	}

	const Pixel& operator()(size_t idx) const 
	{
		assert(idx < m_capacity);
		return m_data[idx]; 
	}

	Pixel* row_begin(size_t y)
	{ 
		return &m_data[get_index(0, y)];
	}

	const Pixel* row_begin(size_t y) const
	{ 
		return &m_data[get_index(0, y)];
	}

	size_t get_index(size_t x, size_t y) const
	{
		assert(x < m_w && y < m_h);
		return x + y*m_w;
	}

	template<typename F>
	void for_each(F f)
	{
		auto* e = m_data + m_w * m_h;
		for (auto* q = m_data; q < e; ++q) {
			f(*q);
		}
	};


	void flip_vertical()
	{
		for (size_t y = 0; 2*y+1 < m_h; ++y) {
			auto* r1 = row_begin(y);
			auto* r2 = row_begin(m_h - y - 1);
			for (size_t x = 0; x < m_w; ++x) {
				std::swap(r1[x], r2[x]);
			}
		}
	};


private:

	size_t m_w = 0;
	size_t m_h = 0;
	size_t m_capacity = 0;
	Pixel* m_data = nullptr;
};
