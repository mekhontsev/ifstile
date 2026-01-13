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

//a container for large data with fast index access and
//the ability to expand indefinitely without moving elements
//BlockSize - the approximate size of one block in bytes
template <typename T, size_t BlockSize = 1024 * 1024 * 8>
struct big_array : public boost::noncopyable
{
public:
	~big_array() { clear();shrink_to_fit(); };

	//return the number of elements
	size_t size() const { return m_size; };
	bool empty() const { return m_size == 0; };
	void clear() { resize(0); };
	size_t capacity() { return m_blocks*s_block_size; };

	//void push_back(const T& v) { emplace_back(v); };//for compatibility only
	void pop_back() { ASSUME(size() > 0); resize(size() - 1); }

	const T& back() const { return at(m_size - 1); };
	T& back() { return at(m_size - 1); };

	template <class... Types>
	T* emplace_back(Types&&... p) noexcept {

		if (!m_arr) {
			m_arr = (T**)std::malloc(s_max_blocks * sizeof(T*));
			if (!m_arr) {
				return nullptr;
			}
		}

		let idx_block = m_size >> s_bits;//block number for the next element
		if (idx_block == m_blocks) {//the block has not yet been allocated
			ASSUME(m_blocks < s_max_blocks);
			auto* m= (T*)std::malloc(s_block_size * sizeof(T));
			if (!m) {
				return nullptr;
			}
			m_arr[m_blocks++] = m;
		}

		++m_size;

		auto* ret = &back();
		new (ret) T(p...);
		return ret;
	}


	const T& operator [] (size_t i) const { return at(i); };
	T& operator [] (size_t i) { return at(i); };

	//get the element with the given number
	T& at(size_t i) const {
		ASSUME(i < size());
		return *(m_arr[i >> s_bits] + (i&s_mask));
	};

	void shrink_to_fit()
	{
		//starting from this block you can delete everyone
		auto from = (m_size + s_mask) >> s_bits;

		while (from < m_blocks) {
			std::free(m_arr[--m_blocks]);
		};

		if (m_blocks == 0 && m_arr) {
			ASSUME(m_size == 0);
			std::free(m_arr);
			m_arr = nullptr;
		}
	}

	//set a new size
	bool resize(size_t new_size)
	{
		while (size() < new_size) {//increase
			if (!emplace_back())return false;	
		}
		//call destructors when decreasing
		while (size() > new_size) {
			at(m_size-1).~T();
			--m_size;
		}
		return true;
	}

	//how much heap memory is currently in use, in bytes
	size_t mem_used() const
	{
		return capacity()*sizeof(T) + (m_blocks ? s_max_blocks*sizeof(T*) : 0);
	};


	//get a CONTINUOUS chunk of memory starting not before than 'from'
	//changes 'from' to point to the index following the allocated region
	T* get_arr(size_t& from, const size_t sz)
	{
		if (sz == 0 || sz > s_block_size) {
			return nullptr;//impossible
		}

		auto to = from + sz;
		if (to > size()) {
			resize(to);
		}

		//blocks corresponding to the ends, inclusive
		let bf = from >> s_bits;
		let bt = (to - 1) >> s_bits;

		if (bf == bt) {
			auto* ret = at(from);
			from = to;
			return ret;
		}

		//jump to the next block, recursion
		from = bt << s_bits;
		return get_arr(from, sz);
	};


	////////////////////////////////////////////////////////////////////////////

	class iterator: 
		public boost::iterator_facade<iterator,T, boost::random_access_traversal_tag >
	{
	
	public:
		iterator(big_array* t,size_t idx):m_arr(t),m_index(idx){}
		
	private:

		friend class boost::iterator_core_access;

		void increment(){++m_index;}
		void decrement(){--m_index;}
		void advance(ptrdiff_t n){ m_index += n;}

		ptrdiff_t distance_to(iterator const& other) const
		{
			return other.m_index - m_index;
		}

		bool equal(iterator const& other) const	
		{
			return m_index == other.m_index;
		}

		T& dereference() const{	return m_arr->at(m_index);}
	
		big_array* m_arr;
		size_t m_index;
	};


	iterator begin(){return iterator(this,0);}
	iterator end(){return iterator(this, size());}

	size_t num_blocks() const { return m_blocks; };

private:

	///////////////////////////////////////////////////////////////////////////
	//integer binary logarithm, for example: 7->2, 8->3
	template<size_t v>
	struct ctz32_t
	{
		enum
		{
			r1 = (v > 0xFFFF) << 4, v1 = v >> r1,
			s2 = (v1 > 0xFF) << 3, v2 = v1 >> s2, r2 = r1 | s2,
			s3 = (v2 > 0xF) << 2, v3 = v2 >> s3, r3 = r2 | s3,
			s4 = (v3 > 0x3) << 1, v4 = v3 >> s4, r4 = r3 | s4,
			res = r4 | (v4 >> 1),
		};
	};

	static const size_t
		s_bits = ctz32_t<BlockSize / sizeof(T)>::res,
		s_block_size = size_t(1) << s_bits,	//elements in a block
		s_mask = s_block_size - 1,
		s_max_blocks = 256 * 1024;	//max number of blocks
	
		

	T**	m_arr = nullptr;//array of pointers to blocks
	size_t	m_size = 0;	//current number of elements
	size_t	m_blocks = 0;//How many blocks are allocated?
};


