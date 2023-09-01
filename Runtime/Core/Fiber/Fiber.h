#pragma once

#include "TypeDef.h"

#include "Core/Thread/ThreadLimit.h"
#include "Core/Thread/Spinlock.h"

NAMESPACE_BEGIN

class Fiber
{
protected:
	friend class Thread;
	friend class FiberPool;

	size_t m_id = 0;
	Spinlock m_lock;
	FiberNativeHandle* m_nativeHandle = 0;

	// which fiber called to switch to me
	Fiber* m_prevFiber = 0;
	bool m_returnPrevFiberToPool = 0;

public:
	constexpr static size_t LOCAL_STORAGE_SIZE = 16;
	size_t m_localStorage[LOCAL_STORAGE_SIZE] = {};

	inline bool IsPrimary() const
	{
		return m_id < ThreadLimit::MAX_THREADS;
	}

	inline auto Id() const
	{
		return m_id;
	}

};

NAMESPACE_END