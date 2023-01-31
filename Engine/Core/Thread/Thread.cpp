#include "Thread.h"

#include "Core/Memory/ManagedLocalScope.h"
#include "Core/Fiber/FiberInfo.h"
#include "Core/Fiber/FiberPool.h"
#include "Platform/Platform.h"

#include "ThreadID.h"

#include "TaskSystem/TaskWorker.h"

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

void Thread::BeginFiber()
{
	// starting of fiber
	auto fiber = Thread::GetCurrentFiber();

	//std::cout << "Begin Fiber " << fiber->m_id << "\n";

	if (fiber->m_prevFiber)
	{
		//std::cout << "Fiber switched from " << fiber->m_prevFiber->m_id << " to " << fiber->m_id << "\n";

		fiber->m_prevFiber->m_prevFiber = 0;
		fiber->m_prevFiber->m_lock.unlock();

		if (fiber->m_returnPrevFiberToPool)
		{
			FiberPool::Return(fiber->m_prevFiber);
		}

		fiber->m_prevFiber = 0;
	}
}

void Thread::SwitchToFiber(Fiber* fiber, bool returnCurrentFiberToFiberPool)
{
	auto currentFiber = Thread::GetCurrentFiber();
	if (currentFiber == fiber) return;

	auto fiberId = fiber->m_id;
	s_threadLocalStorage[ThreadID::Get()].currentFiberID = fiberId;
	ManagedLocalScope::s_managedLocalScopeThreads[ThreadID::Get()] = &ManagedLocalScope::s_managedLocalScopeFibers[fiberId];

#ifdef _DEBUG
	if (fiber->m_lock.try_lock() == false)
	{
		std::cout << "[   WARN   ]\tFiber switching hazard\n";
		fiber->m_lock.lock();
	}
#else
	fiber->m_lock.lock();
#endif // _DEBUG

	fiber->m_prevFiber = currentFiber;

	if (returnCurrentFiberToFiberPool && !currentFiber->IsPrimary())
	{
		fiber->m_returnPrevFiberToPool = true;
	}
	else
	{
		fiber->m_returnPrevFiberToPool = false;
	}

	platform::SwitchToFiber(fiber->m_nativeHandle);

	Thread::BeginFiber();
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

void Thread::EntryPointOfFiber(void*)
{
	Thread::BeginFiber();
	TaskWorker::Get()->Main();
}

NAMESPACE_END