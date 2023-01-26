#pragma once

#include "Core/TypeDef.h"
#include "Core/Thread/Spinlock.h"
#include "Core/Thread/ThreadID.h"
#include "Core/Thread/ThreadUtils.h"

#include "Core/Structures/STD/STDContainers.h"

NAMESPACE_BEGIN

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
		auto start = 0;

		if constexpr (AVOID_THREAD_COLLIDE)
		{
			start = ThreadID::Get() % N_SPACES;
		}

		auto chosenId = ThreadUtils::RingBufferLock<false>(start, m_locks, nullptr);
		m_lists[chosenId].push_back(v);
		m_locks[chosenId].unlock();
	}

	void Clear(const T& v)
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

}


NAMESPACE_END