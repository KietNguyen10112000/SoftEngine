#pragma once

#include "Core/Structures/Raw/ConcurrentBag.h"
#include "Core/Thread/ThreadLimit.h"

#include "Fiber.h"
#include "FiberInfo.h"

NAMESPACE_BEGIN

class API FiberPool
{
protected:
	using Pool = raw::ConcurrentBag<Fiber*, FiberInfo::FIBERS_COUNT>;
	friend class Thread;

	constexpr static size_t TOTAL_FIBERS = ThreadLimit::MAX_THREADS + FiberInfo::FIBERS_COUNT;

	static Fiber s_fibers[TOTAL_FIBERS];
	static Pool s_pool;

public:
	static void Initialize();
	static void Finalize();

	inline static Fiber* Take()
	{
		return s_pool.Take();
	}

	inline static void Return(Fiber* fiber)
	{
		s_pool.Store(fiber);
	}

	/*inline static void Swap(Fiber* from, Fiber* to)
	{
		s_pool.TakeSwap()
	}*/

	inline static Fiber* Get(size_t id)
	{
		assert(id < TOTAL_FIBERS);
		return &s_fibers[id];
	}
};

NAMESPACE_END