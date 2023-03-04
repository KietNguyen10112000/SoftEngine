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

};

}

NAMESPACE_END