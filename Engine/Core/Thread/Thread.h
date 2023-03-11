#pragma once

#include "Core/TypeDef.h"

#include "Core/Fiber/FiberPool.h"
#include "Core/Memory/ManagedLocalScope.h"

#include "ThreadLimit.h"
#include "ThreadID.h"

NAMESPACE_BEGIN

using ThreadEntryPoint = void(*)(void*);

class Fiber;

struct ThreadLocalStorage
{
	size_t currentFiberID = - 1;
};

class API Thread
{
public:
	static ThreadLocalStorage s_threadLocalStorage[ThreadLimit::MAX_THREADS];

private:
	// must call at the beginning of thread entry point func
	static void InitializeForThisThread();

	// must call at the end of thread entry point func
	static void FinalizeForThisThread();

	friend class Fiber;
	static void BeginFiber();

public:
	static void SwitchToFiber(Fiber* fiber, bool returnCurrentFiberToFiberPool);

	static void SwitchToPrimaryFiberOfThisThread();

	static void Sleep(size_t ms);

	// call from PE module
	template <typename T = void>
	inline static void InitializeForThisThreadInThisModule()
	{
		ThreadID::InitializeForThisThreadInThisModule();
		InitializeForThisThread();
		ManagedLocalScope::InitializeForThisThread();
	}

	// call from PE module
	template <typename T = void>
	inline static void FinalizeForThisThreadInThisModule()
	{
		FinalizeForThisThread();
		ThreadID::FinalizeForThisThreadInThisModule();
	}

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

	static void EntryPointOfFiber(void*);

#undef Yield
	inline static void Yield()
	{
		std::this_thread::yield();
	}

};

NAMESPACE_END