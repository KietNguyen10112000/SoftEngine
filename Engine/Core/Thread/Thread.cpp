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
	ThreadID::InitializeForThisThreadInThisModule();

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

	/*if (currentFiber->m_id != 0)
	{
		// never delete starting thread's fiber
		platform::DeleteFiber(currentFiber->m_nativeHandle);
	}*/

	GetCurrentFiber()->m_lock.unlock_no_check_own_thread();

	ThreadID::FinalizeForThisThreadInThisModule();
}

void Thread::SwitchToFiber(Fiber* fiber, bool returnCurrentFiberToFiberPool)
{
	if (Thread::GetCurrentFiber() == fiber) return;

	auto fiberId = fiber->m_id;

	auto currentFiber = FiberPool::Get(s_threadLocalStorage[ThreadID::Get()].currentFiberID);
	s_threadLocalStorage[ThreadID::Get()].currentFiberID = fiberId;
	ManagedLocalScope::s_managedLocalScopeThreads[ThreadID::Get()] = &ManagedLocalScope::s_managedLocalScopeFibers[fiberId];

	if (returnCurrentFiberToFiberPool && !currentFiber->IsPrimary())
	{
		//FiberPool::Return(currentFiber);
		fiber->m_nativeHandleFrom = currentFiber;
	}
	else 
	{
		fiber->m_nativeHandleFrom = 0;
	}

	currentFiber->m_lock.unlock();

#ifdef _DEBUG
	if (fiber->m_lock.try_lock() == false)
	{
		std::cout << "[   WARN   ]\tFiber switching hazard\n";
		fiber->m_lock.lock();
	}
#else
	fiber->m_lock.lock();
#endif // _DEBUG

	std::cout << "Switch from " << currentFiber->m_id << " to " << fiber->m_id << "\n";

	platform::SwitchToFiber(fiber->m_nativeHandle);

	// starting of fiber
	fiber = Thread::GetCurrentFiber();

	if (fiber->m_nativeHandleFrom != 0)
	{
		fiber->m_nativeHandleFrom->m_nativeHandleFrom = 0;
		FiberPool::Return(fiber->m_nativeHandleFrom);
		fiber->m_nativeHandleFrom = 0;
	}
}

void Thread::SwitchToPrimaryFiberOfThisThread()
{
	auto currentFiber = GetCurrentFiber();
	auto thisThreadPrimaryFiber = FiberPool::Get(ThreadID::Get());

	if (currentFiber == thisThreadPrimaryFiber) return;

	//thisThreadPrimaryFiber->m_lock.lock();

	auto fiberId = thisThreadPrimaryFiber->m_id;
	s_threadLocalStorage[ThreadID::Get()].currentFiberID = fiberId;
	ManagedLocalScope::s_managedLocalScopeThreads[ThreadID::Get()] = &ManagedLocalScope::s_managedLocalScopeFibers[fiberId];

	if (currentFiber->IsPrimary() == false) FiberPool::Return(currentFiber);
	//currentFiber->m_lock.unlock();

	platform::SwitchToFiber(thisThreadPrimaryFiber->m_nativeHandle);
}

void Thread::Sleep(size_t ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

NAMESPACE_END