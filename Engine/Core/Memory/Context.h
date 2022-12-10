#pragma once

#include "TypeDef.h"

#include <vector>
#include <atomic>

#include "Core/Thread/Spinlock.h"

#include "Mark.h"
#include "ManagedLocalScope.h"
#include "NewMalloc.h"

#ifdef _DEBUG
#define GC_STRESS_DEBUG
#endif // _DEBUG

NAMESPACE_MEMORY_BEGIN

class ManagedPage;
class ManagedPool;

namespace gc
{

class ContextSharedHandle
{
public:
	//> gc cycles section
	
	// when start new gc cycle m_gcCycles increased by 1
	//size_t m_gcCycles = 0;
	// when mark phase done m_markCycles increased by 1
	//size_t m_markCycles = 0;
	// when sweep phase done m_markCycles increased by 1
	//size_t m_sweepCycles = 0;
	// when counter reach target, kick next gc tasks and modify m_gcCycles, m_markCycles, m_sweepCycles
	size_t m_targetCounter = 0;
	std::atomic<size_t> m_counter = { 0 };

	//< gc cycles section
	
	
	// shared resources must be controlled by thread that have lock
	spinlock m_lock;

	//> shared resources

	// whenever Context need self control (eg: timing, pause, ...)
	bool m_isSelfHandle = true;
	bool m_isPaused = false;

#ifdef GC_STRESS_DEBUG
	// repeat run full batch then run control
	size_t m_markBatchSize = 1;
	size_t m_sweepBatchSize = 1;
#else
	// repeat run full batch then run control
	size_t m_markBatchSize = 64 * KB;
	size_t m_sweepBatchSize = 64 * KB;
#endif // GC_STRESS_DEBUG

	// time limit per context resume call
	// in nano seconds
	size_t m_timeLimit = 16'000; // 0.016ms

	//< shared resources

	std::atomic<size_t>* m_totalAllocatedBytes = 0;

public:
	bool Control(Context* ctx);

	void EndMark(Context* ctx);
	void EndRemark(Context* ctx);
	void EndSweep(Context* ctx);
};

class Context
{
public:
	ContextSharedHandle* m_sharedHandle = 0;

	//> start resume
	// time when start resume
	size_t m_t0 = 0;
	// a copy of ContextSharedHandle::m_markBatchSize or m_sweepBatchSize
	size_t m_batchSize;
	// counter for batch size
	size_t m_counter = 0;
	size_t m_timeLimit = 0;
	//< start resume

	//> mark section
	// for root
	byte*** m_rootIt = 0;
	byte*** m_rootEnd = 0;

	// for local
	ManagedLocalScope::S* m_localScope;
	size_t m_localScopeAllocatedIdx = 0;
	//size_t m_transactionIdx = 0;
	 
	std::vector<MarkState, STDAllocatorMalloc<MarkState>> m_stack;
	//< mark section

	//> sweep section
	// for page
	//AllocatedBlock* m_sweepIt = 0;
	AllocatedBlock* m_sweepEnd = 0;
	ManagedPage* m_page = 0;

	// for pool
	//void* m_sweepLinkIt = 0;
	ManagedPool* m_pool = 0;

	size_t m_totalReclaimedBytes = 0;
	//< sweep section

	bool m_isPaused = false;

	size_t m_resumeFlags = 0;
	size_t m_phase = GC_PHASE::IDLE_PHASE;

	std::vector<byte*, STDAllocatorMalloc<byte*>> m_copiedLocals;

public:
	Context(ContextSharedHandle* hd);
	~Context();

private:
	inline bool IsInMarkPhase()
	{
		/*return (m_localScope && m_localScopeAllocatedIdx != m_localScope->List().Size())
			|| m_rootIt != m_rootEnd 
			|| !m_stack.empty();*/
		return m_phase == GC_PHASE::MARK_PHASE;
	}

	inline bool IsInRemarkPhase()
	{
		return m_phase == GC_PHASE::REMARK_PHASE;
	}

	inline bool IsInSweepPhase()
	{
		return m_phase == GC_PHASE::SWEEP_PHASE;
		//return m_sweepIt != m_sweepEnd || m_sweepLinkIt != 0;
	}

	void Mark();
	void MarkPhase();
	void RemarkPhase();

	void SweepPage();
	void SweepPool();
	void SweepPhase();
	bool Handle();
	void CallDestructor(ManagedHandle* handle);

public:
	void Resume(size_t timeLimit, size_t flags);
	void Run();

	inline bool IsPaused() 
	{ 
		return m_isPaused; 
	}

	inline bool IsEmpty()
	{
		return !IsInMarkPhase() && !IsInSweepPhase();
	}

	inline size_t T0()
	{
		return m_t0;
	}
};

}


NAMESPACE_MEMORY_END