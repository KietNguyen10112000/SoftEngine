#pragma once

#include "TypeDef.h"

NAMESPACE_MEMORY_BEGIN

class ManagedPage;
class ManagedPool;
class GCEvent;

namespace gc
{
namespace internal
{
API void InitializeNewSweepCycle();
API void RegisterLocalScope(void* local);
API void UnregisterLocalScope(void* local);
API void DeferFree(void* p);
}

API void Initialize();
API void RegisterPages(ManagedPage** pages, size_t count);
API void RegisterPools(ManagedPool** pools, size_t count);
API void RegisterRoots(byte*** begin, size_t count);
API void Finalize();

API void SetGCEvent(GCEvent* evt);

API void ClearTrackedBoundariesOfStableValue(byte value);

API void BlockGC(bool block);

// RESET_MARK_VALUES[0] for stableHeap, RESET_MARK_VALUES[1] for gcHeap
API void PerformFullSystemGC(byte MARK_VALUE, byte* RESET_MARK_VALUES);

class GC_RESUME_FLAG
{
public:
	enum
	{
		RETURN_ON_EMPTY_TASK	= 1,
		CLUSTER_MODE			= 2,
		ALLOW_START_NEW_GC		= 4,
	};
};

enum class GC_RETURN
{
	EMPTY_TASK			= 0, 
	END_OF_GC_CYCLE		= 1,
	TIMELIMIT_EXCEED	= 2,
	//NULL_CONTEXT		= 3,
};

class GC_PHASE
{
public:
	enum ENUM
	{
		MARK_PHASE = 0,
		REMARK_PHASE = 1,
		SWEEP_PHASE = 2,
		IDLE_PHASE = 3,
		COUNT = 4,
	};
};

API GC_RETURN Resume(size_t timeLimit, size_t flag);

inline GC_RETURN Run(size_t timeLimit, size_t flag = GC_RESUME_FLAG::RETURN_ON_EMPTY_TASK | GC_RESUME_FLAG::ALLOW_START_NEW_GC)
{
	return Resume(timeLimit, flag);
}

template <template <typename> class P, typename T>
void RegisterRoot(P<T>* arr)
{
	byte** begin = &arr->m_block;
	RegisterRoots(&begin, 1);
}

inline void RegisterPage(ManagedPage* page)
{
	RegisterPages(&page, 1);
}

API GC_PHASE::ENUM GetCurrentPhase();

}

NAMESPACE_MEMORY_END