#pragma once

#include "Core/TypeDef.h"

#include <thread>

NAMESPACE_BEGIN

class ThreadUtils
{
public:
	// return locked space
	template <bool YIELD, typename Lock, typename Condition, size_t N_SPACES>
	static size_t RingBufferLock(size_t startId, Lock(&locks)[N_SPACES], Condition callable)
	{
		auto id = (startId + 1) % N_SPACES;
		while (true)
		{
			if (callable(id) && locks[id].try_lock())
			{
				break;
			}
			id = (id + 1) % N_SPACES;

			if constexpr (YIELD)
			{
				if (id == startId)
				{
					std::this_thread::yield();
				}
			}
		}
		return id;
	}

};

NAMESPACE_END