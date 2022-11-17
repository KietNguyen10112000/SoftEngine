#include "Thread.h"

#include "Core/Memory/ManagedLocalScope.h"
#include "Core/Fiber/FiberInfo.h"
#include "Core/Fiber/FiberPool.h"
#include "Platform/Platform.h"

#include "ThreadID.h"

NAMESPACE_BEGIN

ThreadLocalStorage Thread::s_threadLocalStorage[ThreadLimit::MAX_THREADS] = {};

void Thread::InitializeForThisThread()
{
	ThreadID::InitializeForThisThread();

	// convert thread to fiber
	s_threadLocalStorage[ThreadID::Get()].currentFiberID = ThreadID::Get();
	ManagedLocalScope::s_managedLocalScopeThreads[ThreadID::Get()] = &ManagedLocalScope::s_managedLocalScopeFibers[ThreadID::Get()];
	FiberPool::s_fibers[ThreadID::Get()].m_nativeHandle = platform::ConvertThisThreadToFiber();

	GetCurrentFiber()->m_lock.lock();

	ManagedLocalScope::InitializeForThisThread();
}

void Thread::FinalizeForThisThread()
{
	auto currentFiber = GetCurrentFiber();
	if (GetCurrentFiber()->m_id != ThreadID::Get())
	{
		// at the end of thread, fiber id must equal thread id
		Throw("Finalize error");
	}
	GetCurrentFiber()->m_lock.unlock();
}

void Thread::SwitchToFiber(Fiber* fiber, bool returnCurrentFiberToFiberPool)
{
	if (Thread::GetCurrentFiber() == fiber) return;

#ifdef _DEBUG
	if (fiber->m_lock.try_lock() == false)
	{
		std::cout << "[   WARN   ]\tFiber switching hazard\n";
		fiber->m_lock.lock();
	}
#else
	fiber->m_lock.lock();
#endif // _DEBUG

	auto fiberId = fiber->m_id;

	auto currentFiber = FiberPool::Get(s_threadLocalStorage[ThreadID::Get()].currentFiberID);
	s_threadLocalStorage[ThreadID::Get()].currentFiberID = fiberId;
	ManagedLocalScope::s_managedLocalScopeThreads[ThreadID::Get()] = &ManagedLocalScope::s_managedLocalScopeFibers[fiberId];

	if (returnCurrentFiberToFiberPool) FiberPool::Return(currentFiber);
	currentFiber->m_lock.unlock();

	platform::SwitchToFiber(fiber->m_nativeHandle);
}

void Thread::SwitchToPrimaryFiberOfThisThread()
{
	auto currentFiber = GetCurrentFiber();
	auto thisThreadPrimaryFiber = FiberPool::Get(ThreadID::Get());

	if (currentFiber == thisThreadPrimaryFiber) return;

	thisThreadPrimaryFiber->m_lock.lock();

	auto fiberId = thisThreadPrimaryFiber->m_id;
	s_threadLocalStorage[ThreadID::Get()].currentFiberID = fiberId;
	ManagedLocalScope::s_managedLocalScopeThreads[ThreadID::Get()] = &ManagedLocalScope::s_managedLocalScopeFibers[fiberId];

	if (currentFiber->IsPrimary() == false) FiberPool::Return(currentFiber);
	currentFiber->m_lock.unlock();

	platform::SwitchToFiber(thisThreadPrimaryFiber->m_nativeHandle);
}

NAMESPACE_END