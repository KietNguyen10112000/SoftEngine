#include "GC.h"

#include <vector>
#include <thread>

#include "Core/Time/Clock.h"

#include "System.h"
#include "NewMalloc.h"
#include "ContextManual.h"

NAMESPACE_MEMORY_BEGIN

//extern ManagedHeap* g_rawHeap;
extern ManagedHeap* g_stableHeap;
extern ManagedHeap* g_gcHeap;

namespace gc
{

gc::System* g_system = 0;
spinlock g_lock;

ContextManual* g_manualContext = nullptr;

void gc::internal::InitializeNewSweepCycle()
{
	g_system->InitializeNewSweepCycle();
}

void gc::internal::DeferFree(void* p)
{
	if (g_system == 0)
	{
		free(p);
		return;
	}

	g_system->DeferFree(p);
}

void gc::Initialize()
{
	if (g_system) return;

	g_lock.lock();
	if (g_system == 0)
	{
		g_system		= NewMalloc<System>();
		g_manualContext = NewMalloc<ContextManual>();
	}
	g_lock.unlock();
}

void gc::Finalize()
{
	if (g_system)
	{
		DeleteMalloc(g_system);
		DeleteMalloc(g_manualContext);
		g_system = 0;
		g_manualContext = nullptr;
	}
}

void gc::RegisterPages(ManagedPage** pages, size_t count)
{
	Initialize();

	g_system->RegisterPages(pages, count);
}

void gc::RegisterPools(ManagedPool** pools, size_t count)
{
	Initialize();

	g_system->RegisterPools(pools, count);
}

void RegisterRoots(byte*** begin, size_t count)
{
	Initialize();

	g_system->RegisterRoots(begin, count);
}

void gc::SetGCEvent(GCEvent* evt)
{
	g_system->m_gcEvent = evt;
}

void gc::ClearTrackedBoundariesOfStableValue(byte value)
{
	g_system->ClearTrackedBoundariesOfStableValue(value);
}

void BlockGC(bool block)
{
	if (block)
	{
	Loop:
		g_system->m_globalLock.lock();

		if (!g_system->IsEndGC())
		{
			g_system->m_globalLock.unlock();
			std::this_thread::yield();
			goto Loop;
		}
	}
	else
	{
		g_system->m_globalLock.unlock();
	}
}

void PerformFullSystemGC(byte MARK_VALUE, byte* RESET_MARK_VALUE)
{
	ManagedHeap* heaps[] = {
		g_stableHeap,
		g_gcHeap
	};
	g_manualContext->DoGC(MARK_VALUE, g_system, RESET_MARK_VALUE, heaps, 2);
}

void gc::internal::RegisterLocalScope(void* s)
{
	Initialize();

	if (g_system)
	{
		g_system->RegisterLocalScope(s);
	}
}

void gc::internal::UnregisterLocalScope(void* s)
{
	if (g_system)
	{
		if (g_system->UnregisterLocalScope(s))
		{
			Finalize();
		}
	}
}

inline void WaitForCtx(Context*& ctx, bool allowStartNewCycle)
{
	while (true)
	{
		ctx = g_system->GetContext(allowStartNewCycle);

		if (ctx == 0)
		{
			std::this_thread::yield();
			continue;
		}

		break;
	}
}

GC_RETURN gc::Resume(size_t timeLimit, size_t flag)
{
	bool allowStartNewCycle = ((flag & GC_RESUME_FLAG::ALLOW_START_NEW_GC) != 0);
	auto ctx = g_system->GetContext(allowStartNewCycle);

	if (ctx == 0)
	{
		if (/*g_system->IsEndGC() && */allowStartNewCycle == false)
		{
			return GC_RETURN::END_OF_GC_CYCLE;
		}
		if ((flag & GC_RESUME_FLAG::RETURN_ON_EMPTY_TASK))
		{
			return GC_RETURN::EMPTY_TASK;
		}
		WaitForCtx(ctx, allowStartNewCycle);
	}

	ctx->Resume(timeLimit, flag);

	auto t0 = ctx->T0();
	while (true)
	{
		ctx->Run();
		if (ctx->IsPaused())
		{
			g_system->PauseContext(ctx);
			return GC_RETURN::TIMELIMIT_EXCEED;
		}

		if (t0 != -1 && Clock::ns::now() - t0 >= timeLimit)
		{
			g_system->ReturnContext(ctx);
			return GC_RETURN::TIMELIMIT_EXCEED;
		}

		if (g_system->IsEndGC())
		{
			g_system->ReturnContext(ctx);
			return GC_RETURN::END_OF_GC_CYCLE;
		}
		if (g_system->AssignTaskToContext(ctx, allowStartNewCycle) == false)
		{
			g_system->ReturnContext(ctx);

			if (flag & GC_RESUME_FLAG::RETURN_ON_EMPTY_TASK)
			{
				return GC_RETURN::EMPTY_TASK;
			}
			
			WaitForCtx(ctx, allowStartNewCycle);
		}
	}
}

GC_PHASE::ENUM GetCurrentPhase()
{
	return (GC_PHASE::ENUM)g_system->m_phase;
}

}

NAMESPACE_MEMORY_END