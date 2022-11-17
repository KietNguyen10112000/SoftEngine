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

public:
	inline bool IsPrimary() const
	{
		return m_id < ThreadLimit::MAX_THREADS;
	}

};

NAMESPACE_END