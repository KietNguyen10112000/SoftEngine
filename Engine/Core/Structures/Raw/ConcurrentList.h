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
};

}

NAMESPACE_END