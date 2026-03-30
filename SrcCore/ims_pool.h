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


//for quick allocation, deallocation, and creation of POD type arrays
struct ims_pool : public boost::noncopyable
{
	struct bucket
	{
		struct link { link* next = nullptr; };

		~bucket()
		{
			clear();
		};

		void dealloc(link* s)
		{
			s->next = m_free;
			m_free = s;
		};

		link* alloc()
		{
			auto* ret = m_free;
			if (m_free)m_free = m_free->next;
			return ret;
		};

		void clear() 
		{
			while (m_free) {
				auto* d = m_free;
				m_free = m_free->next;
				std::free(d);
			}
		}

	private:

		link* m_free = nullptr;//list of free for use
	};

	static constexpr uint8_t get_idx(size_t bytes)
	{
		assert(bytes > 0);
		//1-8:	7		-> 0
		//9-16: 8-15	-> 1
		//17-32:16-31	-> 2

		return static_cast<uint8_t>(std::bit_width((bytes - 1) >> s_min_bits));

		//mem_bytes = std::max(s_min_bytes, bytes) - 1;
		//return static_cast<uint8_t>((64 - s_min_bits) - clz64(bytes));
	};

	static constexpr size_t get_size(uint8_t bucket_idx)
	{
		return s_min_bytes << bucket_idx;
	}

	void* alloc_by_idx(uint8_t idx)
	{
		if (idx < m_bucket.size()) {
#ifndef NDEBUG
			//ims_increment(m_allocated_elems);
#endif
			assert(idx < s_num_buckets);
			auto& b = m_bucket[idx];
			auto* ret = b.alloc();
			if (ret)return ret;
		}

		return std::malloc(get_size(idx));
	};

	void dealloc_by_idx(size_t idx, void* e)
	{
		if (idx < m_bucket.size()) {
#ifndef NDEBUG
			//assert(m_allocated_elems > 0);
			//ims_decrement(m_allocated_elems);
#endif
			auto& b = m_bucket[idx];
			b.dealloc((bucket::link*)e);
		} else {
			std::free(e);
		}
	};

	void clear() 
	{
		for (auto& b : m_bucket) {
			b.clear();
		}
	}

	////////////////////////////////////////////////////////////////////////////

	void* allocate(size_t mem_bytes)
	{
		return alloc_by_idx(get_idx(mem_bytes));
	};

	void deallocate(void* e, size_t mem_bytes)
	{
		dealloc_by_idx(get_idx(mem_bytes), e);
	};

	void* reallocate(void* e, size_t old_bytes, size_t new_bytes)
	{
		assert(new_bytes >= old_bytes);

		let idx_old = get_idx(old_bytes);
		let idx_new = get_idx(new_bytes);
		if (idx_old == idx_new) {
			return e;
		}
		auto* ret = alloc_by_idx(idx_new);
		std::memcpy(ret, e, old_bytes);//only for POD types!
		dealloc_by_idx(idx_old, e);
		return ret;
	};

private:

	// allocated fragments - not less than 2^3 bytes
	static constexpr size_t s_min_bits = 3;
	//fragments larger than 2^(s_num_buckets+s_min_bits-1) bytes
	//are allocated and deallocated directly via malloc/free - bypassing ims_pool
	static constexpr size_t s_num_buckets = 15;
	static constexpr size_t s_min_bytes = (1 << s_min_bits);

	static_assert(sizeof(bucket::link) <= s_min_bytes);

	//free blocks
	std::array<bucket, s_num_buckets> m_bucket{};

#ifndef NDEBUG
	//int m_allocated_elems;
#endif

};


//using alloc_type = ims_pool_allocator<ims_val*>;
//alloc_type alloc(m_ep->get_pool());
//std::vector<ims_val*, alloc_type> vec(alloc);
template<class T>
struct ims_pool_allocator
{
	using value_type = T;

	ims_pool_allocator(ims_pool* p) : m_pool(p) {};

	template<class U>
	ims_pool_allocator(const ims_pool_allocator <U>& u) : m_pool(u.m_pool) {}

	T* allocate(size_t n) noexcept
	{
		return static_cast<T*>(m_pool->allocate(n * sizeof(T)));
	}

	void deallocate(T* p, size_t n) noexcept
	{
		m_pool->deallocate(p, n * sizeof(T));
	}

	ims_pool* m_pool = nullptr;
};

template<class T, class U>
bool operator==(const ims_pool_allocator <T>&, const ims_pool_allocator <U>&) { return true; }

template<class T, class U>
bool operator!=(const ims_pool_allocator <T>&, const ims_pool_allocator <U>&) { return false; }