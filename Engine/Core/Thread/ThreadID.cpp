#include "ThreadID.h"

#include <map>
#include <thread>

#include "Core/Memory/Pool.h"
#include "Core/Thread/Spinlock.h"

#include "ThreadLimit.h"

#if defined WIN32 || defined WIN64
#include <Windows.h>
#endif

NAMESPACE_BEGIN

struct ThreadIdHandle
{
	size_t id;

	// for thread local storeage of dynamic-linked modules
	size_t moduleCounter;
};

using ThreadIdMap = std::map<size_t, ThreadIdHandle,
	std::less<size_t>, STLNodeGlobalAllocator<std::pair<const size_t, ThreadIdHandle>>>;

bool g_threadIdIsInitialized = false;
Spinlock g_threadIdSpinlock;
ThreadIdMap g_threadHandleIdMap;

// act like stack
size_t g_threadIdTop = 0;
size_t g_threadIdAvailableIds[ThreadLimit::MAX_THREADS] = {};

size_t ThreadIdGetThreadHandle()
{
#if defined WIN32 || defined WIN64
	return (size_t)::GetCurrentThreadId();
#else
	static_assert(0, "re-implement");
#endif // WIN32
}

void ThreadIdInitilizeOnce()
{
	for (size_t i = 0; i < ThreadLimit::MAX_THREADS; i++)
	{
		g_threadIdAvailableIds[i] = ThreadLimit::MAX_THREADS - i - 1;
	}
	g_threadIdTop = ThreadLimit::MAX_THREADS - 1;

	g_threadIdIsInitialized = true;
}

inline size_t ThreadIdGetThreadId()
{
	assert(g_threadIdTop != 0);
	return g_threadIdAvailableIds[g_threadIdTop--];
}

inline void ThreadIdReleaseThreadId(size_t id)
{
	assert(g_threadIdTop != ThreadLimit::MAX_THREADS);
	g_threadIdAvailableIds[g_threadIdTop++] = id;
}

size_t ThreadID::_Get()
{
	size_t ret = -1;

	g_threadIdSpinlock.lock();

	if (g_threadIdIsInitialized == false)
	{
		ThreadIdInitilizeOnce();
	}

	auto handle = ThreadIdGetThreadHandle();
	auto it = g_threadHandleIdMap.find(handle);

	if (it != g_threadHandleIdMap.end())
	{
		it->second.moduleCounter++;
	}
	else
	{
		it = g_threadHandleIdMap.insert({ handle, {} }).first;
		it->second.id = ThreadIdGetThreadId();
		it->second.moduleCounter = 1;
	}

	ret = it->second.id;

	g_threadIdSpinlock.unlock();

	return ret;
}

void ThreadID::Finalize()
{
	g_threadIdSpinlock.lock();

	auto handle = ThreadIdGetThreadHandle();
	auto it = g_threadHandleIdMap.find(handle);

	assert(it != g_threadHandleIdMap.end());

	if ((--(it->second.moduleCounter)) == 0)
	{
		auto id = it->second.id;
		g_threadHandleIdMap.erase(it);
		ThreadIdReleaseThreadId(id);
	}

	g_threadIdSpinlock.unlock();
}

NAMESPACE_END