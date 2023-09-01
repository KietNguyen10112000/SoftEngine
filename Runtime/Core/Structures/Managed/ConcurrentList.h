#pragma once

#include "Core/TypeDef.h"
#include "Core/Thread/Spinlock.h"
#include "Core/Thread/ThreadID.h"
#include "Core/Thread/ThreadUtils.h"

#include "Array.h"

NAMESPACE_BEGIN

template <typename T, size_t N_SPACES = 4>
class ConcurrentList : Traceable<ConcurrentList<T, N_SPACES>>
{
private:
	Array<T> m_lists[N_SPACES] = {};
	Spinlock m_locks[N_SPACES] = {};

private:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_lists);
	}

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
		m_lists[chosenId].Push(v);
		m_locks[chosenId].unlock();
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

};

template <typename Container, size_t N_SPACES = 4>
class ConcurrentListC : Traceable<ConcurrentListC<Container, N_SPACES>>
{
private:
	Container m_lists[N_SPACES];

public:
	friend class Tracer;
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_lists);
	}

	inline void LockAll()
	{
		for (size_t i = 0; i < N_SPACES; i++)
		{
			m_lists[i].lock();
		}
	}

	inline void UnlockAll()
	{
		for (size_t i = 0; i < N_SPACES; i++)
		{
			m_lists[i].unlock();
		}
	}

public:
	//inline ConcurrentListC() {};

	template <typename T, bool AVOID_THREAD_COLLIDE = false>
	void Add(const T& v)
	{
		size_t start = -1;

		if constexpr (AVOID_THREAD_COLLIDE)
		{
			start = ThreadID::Get() % N_SPACES;
		}

		auto chosenId = ThreadUtils::RingBufferLock<false>(start, m_lists, nullptr);
		m_lists[chosenId].Add(v);
		m_lists[chosenId].unlock();
	}

	void Clear()
	{
		for (size_t i = 0; i < N_SPACES; i++)
		{
			m_lists[i].lock();
			m_lists[i].Clear();
			m_lists[i].unlock();
		}
	}

	template <typename Callback>
	void ForEachContainer(Callback callback)
	{
		LockAll();

		for (auto& list : m_lists)
		{
			callback(list);
		}

		UnlockAll();
	}

};


NAMESPACE_END