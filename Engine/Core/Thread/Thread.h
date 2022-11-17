#pragma once

#include "Core/TypeDef.h"

#include "Core/Fiber/FiberPool.h"

#include "ThreadLimit.h"
#include "ThreadID.h"

NAMESPACE_BEGIN

using ThreadEntryPoint = void(*)(void*);

class Fiber;

struct ThreadLocalStorage
{
	size_t currentFiberID = - 1;
};

class Thread
{
public:
	API static ThreadLocalStorage s_threadLocalStorage[ThreadLimit::MAX_THREADS];

public:
	// must call at the beginning of thread entry point func
	API static void InitializeForThisThread();

	// must call at the end of thread entry point func
	API static void FinalizeForThisThread();

	API static void SwitchToFiber(Fiber* fiber, bool returnCurrentFiberToFiberPool);

	API static void SwitchToPrimaryFiberOfThisThread();

public:
	inline static size_t GetCurrentFiberID()
	{
		return s_threadLocalStorage[ThreadID::Get()].currentFiberID;
	}

	inline static Fiber* GetCurrentFiber()
	{
		return FiberPool::Get(s_threadLocalStorage[ThreadID::Get()].currentFiberID);
	}

	inline static size_t GetID()
	{
		return ThreadID::Get();
	}

};

NAMESPACE_END