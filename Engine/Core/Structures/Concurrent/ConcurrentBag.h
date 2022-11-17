#pragma once

#include "Core/TypeDef.h"
#include "Core/Thread/Spinlock.h"
#include "Core/Thread/ThreadID.h"
#include "Core/Thread/ThreadUtils.h"

NAMESPACE_BEGIN

// fixed capacity ConcurrentBag
template <typename T, size_t N_ELEMENTS, size_t N_SPACES = 8>
class ConcurrentBag
{
protected:
	constexpr static size_t N_ELEMENTS_PER_SPACE = N_ELEMENTS / N_SPACES;

	struct SpaceCtx
	{
		// like stack top
		size_t topId = 0;
	};

	T m_elements[N_SPACES][N_ELEMENTS_PER_SPACE] = {};
	Spinlock m_locks[N_SPACES] = {};
	SpaceCtx m_ctxs[N_SPACES] = {};

	// the current size
	std::atomic<size_t> m_size = { 0 };

	static_assert(std::is_pointer_v<T>, "This class is currently safe for pointer type!");

public:
	inline void Store(const T& v)
	{
		auto start = ThreadID::Get() % N_SPACES;
		auto chosenId = ThreadUtils::RingBufferLock<false>(start, m_locks,
			[&](size_t id) -> bool
			{
				return m_ctxs[id].topId != N_ELEMENTS_PER_SPACE;
			}
		);

		m_elements[chosenId][(m_ctxs[chosenId].topId)++] = v;
		m_locks[chosenId].unlock();
		++m_size;
	}

	inline T Take()
	{
		// not enough capacity, try increase N_ELEMENTS
		assert(m_size.load() != 0);

		auto start = ThreadID::Get() % N_SPACES;
		auto chosenId = ThreadUtils::RingBufferLock<false>(start, m_locks,
			[&](size_t id) -> bool
			{
				return m_ctxs[id].topId != 0;
			}
		);

		T ret = m_elements[chosenId][--(m_ctxs[chosenId].topId)];
		m_locks[chosenId].unlock();
		--m_size;
		return ret;
	}

	// just like Take(), but swap return result with v1 under lock guard
	inline T TakeSwap(const T& v1)
	{
		// not enough capacity, try increase N_ELEMENTS
		assert(m_size.load() != 0);

		auto start = ThreadID::Get() % N_SPACES;
		auto chosenId = ThreadUtils::RingBufferLock<false>(start, m_locks,
			[&](size_t id) -> bool
			{
				return m_ctxs[id].topId != 0;
			}
		);

		T ret = m_elements[chosenId][m_ctxs[chosenId].topId - 1];
		m_elements[chosenId][m_ctxs[chosenId].topId - 1] = v1;
		m_locks[chosenId].unlock();
		return ret;
	}

};

NAMESPACE_END