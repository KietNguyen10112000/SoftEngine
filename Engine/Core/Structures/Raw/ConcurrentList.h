#pragma once

#include "Core/TypeDef.h"
#include "Core/Thread/Spinlock.h"
#include "Core/Thread/ThreadID.h"
#include "Core/Thread/ThreadUtils.h"

#include "Core/Structures/STD/STDContainers.h"

NAMESPACE_BEGIN

namespace raw
{

// same as ConcurrentBag but dynamic size and for each support
template <typename T, size_t N_SPACES = 4>
class ConcurrentList
{
public:
	struct RingIteration
	{
		ConcurrentList<T, N_SPACES>*	list;
		size_t							spaceId;
		T*								it;
		T*								spaceEnd;

		inline void NextSpace()
		{
			auto oriSpace = spaceId;
			do
			{
				spaceId = (spaceId + 1) % N_SPACES;
				auto& l = list->m_lists[spaceId];
				it = l.data();
				spaceEnd = it + l.size();
			} while (it == spaceEnd && spaceId != oriSpace);
			
		}

		inline RingIteration& operator++()
		{
			++it;

			if (it == spaceEnd)
			{
				NextSpace();
			}

			return *this;
		}

		inline RingIteration& operator+=(size_t n)
		{
			while (true)
			{
				size_t remain = spaceEnd - it;

				if (remain > n)
				{
					it += n;
					break;
				}

				NextSpace();
				n -= remain;
			}
			return *this;
		}

		inline bool operator==(const RingIteration& iter)
		{
			return it == iter.it;
		}

		inline bool operator!=(const RingIteration& iter)
		{
			return it != iter.it;
		}

		inline T& operator*()
		{
			return *it;
		}
	};

private:
	std::Vector<T> m_lists[N_SPACES];
	Spinlock m_locks[N_SPACES] = {};

private:
	inline void LockAll()
	{
		for (size_t i = 0; i < N_SPACES; i++)
		{
			m_locks[i].lock();
		}
	}

	inline void UnlockAll()
	{
		for (size_t i = 0; i < N_SPACES; i++)
		{
			m_locks[i].unlock();
		}
	}

public:
	template <bool AVOID_THREAD_COLLIDE = false>
	void Add(const T& v)
	{
		size_t start = -1;

		if constexpr (AVOID_THREAD_COLLIDE)
		{
			start = ThreadID::Get() % N_SPACES;
		}

		auto chosenId = ThreadUtils::RingBufferLock<false>(start, m_locks, nullptr);
		m_lists[chosenId].push_back(v);
		m_locks[chosenId].unlock();
	}

	void AddToSpace(size_t spaceId, const T& v)
	{
		m_locks[spaceId].lock();
		m_lists[spaceId].push_back(v);
		m_locks[spaceId].unlock();
	}

	void AddToSpaceUnsafe(size_t spaceId, const T& v)
	{
		m_lists[spaceId].push_back(v);
	}

	void Clear()
	{
		for (size_t i = 0; i < N_SPACES; i++)
		{
			m_locks[i].lock();
			m_lists[i].clear();
			m_locks[i].unlock();
		}
	}

	template <typename Callback>
	void ForEach(Callback callback)
	{
		LockAll();

		for (auto& list : m_lists)
		{
			for (auto& v : list)
			{
				callback(v);
			}
		}

		UnlockAll();
	}

	inline size_t size() const
	{
		size_t sum = 0;
		for (auto& l : m_lists)
		{
			sum += l.size();
		}
		return sum;
	}

	template <bool AVOID_THREAD_COLLIDE = false>
	inline T Take()
	{
		assert(size() > 0);

		size_t start = -1;

		if constexpr (AVOID_THREAD_COLLIDE)
		{
			start = ThreadID::Get() % N_SPACES;
		}

		auto chosenId = ThreadUtils::RingBufferLock<false>(start, m_locks,
			[&](size_t id)
			{
				return m_lists[id].size() > 0;
			}
		);

		T ret = m_lists[chosenId].back();
		m_lists[chosenId].pop_back();
		return ret;
	}

	// no thread-safe
	// output size must >= N_SPACES
	// output2 size = size() - output1Count
	inline void Split(size_t output1Count, T** output1, T** output2, 
		size_t* output1EndIndex = 0, size_t* output2EndIndex = 0)
	{
		output1[0] = 0;
		output2[0] = 0;

		size_t total = 0;
		size_t i = 0;
		T** p = output1;
		for (auto& l : m_lists)
		{
			auto increSize = l.size();

			if (increSize == 0) continue;

			total += increSize;

			if (total > output1Count)
			{
				auto remain = output1Count - (total - increSize);

				if (remain != 0) 
				{
					p[i++] = &l[0];
					p[i++] = &l[0] + remain;

					if (output1EndIndex) *output1EndIndex = i;

					p[i++] = 0;
				}

				p = output2;
				i = 0;
				output1Count = std::numeric_limits<size_t>::max();

				p[i++] = &l[remain];
				p[i++] = &l.back() + 1;

				continue;
			}

			p[i++] = &l[0];
			p[i++] = &l.back() + 1;
		}


		if (output2EndIndex) *output2EndIndex = i;

		p[i++] = 0;
	}

	// return double of num chunk
	inline size_t GetChunks(T** output)
	{
		size_t i = 0;
		for (auto& l : m_lists)
		{
			if (l.size() == 0) continue;

			output[i++] = &l[0];
			output[i++] = &l.back() + 1;
		}
		output[i] = 0;
		return i;
	}

	inline T* GetChunkAddrAtIndex(size_t id)
	{
		return m_lists[id].data();
	}

	inline size_t GetChunkSizeAt(size_t id)
	{
		return m_lists[id].size();
	}

	inline void ResizeChunk(size_t chunkId, size_t chunkSize)
	{
		m_lists[chunkId].resize(chunkSize);
	}

	inline auto GetChunksCount() const
	{
		return N_SPACES;
	}

	inline RingIteration RingBegin()
	{
		auto& list = m_lists[0];
		RingIteration ret;
		ret.list = this;
		ret.spaceId = 0;
		ret.it = list.data();
		ret.spaceEnd = ret.it + list.size();
		return ret;
	}
};

//// same as ConcurrentBag but dynamic size and for each support
//template <typename T, size_t N_SPACES = 4>
//class ConcurrentList
//{
//private:
//	std::Vector<T>		m_buffer;
//	std::atomic<size_t> m_size		= { 0 };
//	std::atomic<size_t> m_numWriter	= { 0 };
//	spinlock			m_growthLock;
//
//private:
//	inline void LockAll()
//	{
//		(void)(0);
//	}
//
//	inline void UnlockAll()
//	{
//		(void)(0);
//	}
//
//	inline void _GrowthCapacity()
//	{
//		auto capacity = m_buffer.size();
//		if (capacity == 0)
//		{
//			m_buffer.resize(16);
//		}
//		else
//		{
//			// growth by 2
//			auto newCapacity = 2 * capacity;
//			m_buffer.resize(newCapacity);
//		}
//		
//	}
//
//	inline void WaitForNoWriter()
//	{
//		while (m_numWriter.load(std::memory_order_relaxed) != 0)
//		{
//			std::this_thread::yield();
//		}
//	}
//
//	inline void TryGrowthCapacity(size_t expectCapacity)
//	{
//		WaitForNoWriter();
//		m_growthLock.lock();
//
//		while (m_buffer.size() <= expectCapacity)
//		{
//			_GrowthCapacity();
//		}
//
//		m_growthLock.unlock();
//	}
//
//public:
//	template <bool AVOID_THREAD_COLLIDE = false>
//	inline void Add(const T& v)
//	{
//		auto id = m_size++;
//
//		if (id >= m_buffer.size())
//		{
//			TryGrowthCapacity(id);
//		}
//		
//		++m_numWriter;
//		m_buffer[id] = v;
//		--m_numWriter;
//	}
//
//	void Clear()
//	{
//		m_size = 0;
//	}
//
//	template <typename Callback>
//	void ForEach(Callback callback)
//	{
//		auto _size = size();
//		for (size_t i = 0; i < _size; i++)
//		{
//			auto& elm = m_buffer[i];
//			callback(elm);
//		}
//	}
//
//	inline size_t size() const
//	{
//		return m_size.load(std::memory_order_relaxed);
//	}
//
//	template <bool AVOID_THREAD_COLLIDE = false>
//	inline T Take()
//	{
//		return m_buffer[--m_size];
//	}
//
//	// no thread-safe
//	// output size must >= N_SPACES
//	// output2 size = size() - output1Count
//	inline void Split(size_t output1Count, T** output1, T** output2)
//	{
//		intmax_t _size = size();
//		output1[0] = 0;
//		output2[0] = 0;
//
//		intmax_t remain = _size - output1Count;
//
//		if (_size != 0)
//		{
//			auto end = _size >= output1Count ? output1Count : _size;
//			output1[0] = &m_buffer[0];
//			output1[1] = output1[0] + end;
//			output1[2] = 0;
//		}
//
//		if (remain > 0)
//		{
//			output2[0] = output1[1];
//			output2[1] = output2[0] + remain;
//			output2[2] = 0;
//		}
//	}
//
//	inline T* GetChunkAddrAtIndex(size_t id)
//	{
//		assert(id == 0);
//		return m_buffer.data();
//	}
//
//	inline size_t GetChunkSizeAt(size_t id)
//	{
//		assert(id == 0);
//		return m_buffer.size();
//	}
//
//	inline void Resize(size_t size)
//	{
//		m_size.exchange(size, std::memory_order_relaxed);
//	}
//
//	inline size_t GetChunks(T** output)
//	{
//		size_t i = 0;
//		if (size() != 0)
//		{
//			output[i++] = &m_buffer[0];
//			output[i++] = &m_buffer.back() + 1;
//		}
//		output[i] = 0;
//		return i;
//	}
//};

}

NAMESPACE_END