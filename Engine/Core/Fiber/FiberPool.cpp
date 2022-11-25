#include "FiberPool.h"

#include <iostream>

#include "Core/Thread/Thread.h"
#include "Platform/Platform.h"

NAMESPACE_BEGIN

Fiber FiberPool::s_fibers[ThreadLimit::MAX_THREADS + FiberInfo::FIBERS_COUNT] = {};
FiberPool::Pool FiberPool::s_pool = {};

void FiberPool::Initialize()
{
	// create external fibers
	for (size_t i = ThreadLimit::MAX_THREADS; i < ThreadLimit::MAX_THREADS + FiberInfo::FIBERS_COUNT; i++)
	{
		s_fibers[i].m_nativeHandle = platform::CreateFiber();
		s_pool.Store(&s_fibers[i]);
	}

	// indexing
	for (size_t i = 0; i < ThreadLimit::MAX_THREADS + FiberInfo::FIBERS_COUNT; i++)
	{
		s_fibers[i].m_id = i;
	}
}

void FiberPool::Finalize()
{
	for (size_t i = ThreadLimit::MAX_THREADS; i < ThreadLimit::MAX_THREADS + FiberInfo::FIBERS_COUNT; i++)
	{
		//std::cout << "========================== FiberPool::Finalize --- " << i << "====================\n";

		if (Thread::GetCurrentFiberID() != i) {
			if (s_fibers[i].m_lock.try_lock() == false)
			{
				Throw("Fiber pool finalize error");
			}
		}

		platform::DeleteFiber(s_fibers[i].m_nativeHandle);
	}
}

NAMESPACE_END