#include "ManagedLocalScope.h"

#include "Core/Thread/ThreadLimit.h"
#include "Core/Thread/ThreadID.h"
#include "Core/Thread/Spinlock.h"
#include "Core/Fiber/FiberInfo.h"
#include "Core/Memory/GC.h"

NAMESPACE_MEMORY_BEGIN

spinlock g_managedLocalScopeLock;
bool g_managedLocalScopeIsInitialized = false;
ManagedLocalScope::S* ManagedLocalScope::s_managedLocalScopeThreads[ThreadLimit::MAX_THREADS] = {};
ManagedLocalScope::S ManagedLocalScope::s_managedLocalScopeFibers[ThreadLimit::MAX_THREADS + FiberInfo::FIBERS_COUNT] = {};

void ManagedLocalScope::Initialize()
{
	for (auto& v : s_managedLocalScopeFibers)
	{
		v.checkpoints.reserve(1 * KB);
		v.stack.reserve(8 * KB);
		v.transactions.reserve(8 * KB);
		gc::internal::RegisterLocalScope(&v);
	}
	
	g_managedLocalScopeIsInitialized = true;
}

ManagedLocalScope::S** ManagedLocalScope::GetManagedLocalScope()
{
	ManagedLocalScope::S** ret = 0;
	g_managedLocalScopeLock.lock();
	if (g_managedLocalScopeIsInitialized == false)
	{
		Initialize();
	}
	ret = &s_managedLocalScopeThreads[ThreadID::Get()];

//#ifdef _DEBUG
//	SwitchToFiber(ThreadID::Get());
//#else
//	static_assert(0, "implement fiber");
//#endif // _DEBUG

	g_managedLocalScopeLock.unlock();
	return ret;
}

ManagedLocalScope::S** ManagedLocalScope::GetManagedLocalScopeBootPhase()
{
	static ManagedLocalScope::S* s = NewMalloc<ManagedLocalScope::S>();
	return &s;
}

void ManagedLocalScope::ReleaseManagedLocalScopeBootPhase(ManagedLocalScope::S* s)
{
	DeleteMalloc(s);
}

bool ManagedLocalScope::IsManagedLocalScopeBootPhase(S* s)
{
	return !(s >= ManagedLocalScope::s_managedLocalScopeFibers
		&& s < (ManagedLocalScope::s_managedLocalScopeFibers + ThreadLimit::MAX_THREADS + FiberInfo::FIBERS_COUNT));
}

NAMESPACE_MEMORY_END