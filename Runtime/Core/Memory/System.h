#pragma once

#include "TypeDef.h"

#include <condition_variable>
#include <thread>

#include "Core/Thread/Spinlock.h"

#include "Context.h"
#include "ManagedPage.h"
#include "ManagedPool.h"
#include "NewMalloc.h"

#include "GCEvent.h"

NAMESPACE_MEMORY_BEGIN

namespace gc
{


class System
{
public:
	constexpr static size_t MAX_CONTEXTS = 128;
	constexpr static size_t MAX_TASKS = 1024;

	constexpr static size_t MAX_PAGES = 256;
	constexpr static size_t MAX_POOLS = MAX_PAGES;

	// reserve for threads and fibers, 256 slots
	constexpr static size_t MAX_THREADS = 256;

	constexpr static size_t MAX_ROOTS = 1024;

	struct MarkTask
	{
		byte*** rootBegin = 0;
		byte*** rootEnd = 0;

		byte** crossBoundariesBegin = 0;
		byte** crossBoundariesEnd = 0;

		ManagedLocalScope::S* localScope = 0;
	};

	struct SweepTask
	{
		ManagedPage* page = 0;
		ManagedPool* pool = 0;
	};

	class StackCtx
	{
	public:
		spinlock m_lock;
		Context* m_ctxs[MAX_CONTEXTS / 2] = {};
		Context** m_ctxTop = m_ctxs;

	public:
		inline size_t Size()
		{
			return m_ctxTop - m_ctxs;
		}

		inline bool try_lock() { return m_lock.try_lock(); }
		inline auto& Lock() { return m_lock; }

		inline void Push(Context* ctx)
		{
			*(m_ctxTop++) = ctx;
		}

		inline Context* Pop()
		{
			return *(--m_ctxTop);
		}

	};

	template <typename T>
	class TaskQueue
	{
	public:
		constexpr static auto CAPACITY = MAX_TASKS / 2;
		spinlock m_lock;
		T m_tasks[CAPACITY] = {};

		intmax_t m_frontIdx = 0;
		intmax_t m_backIdx = 0;

	public:
		inline void Reset()
		{
			m_frontIdx = 0;
			m_backIdx = 0;
		}

		inline size_t Size()
		{
			assert(m_backIdx - m_frontIdx >= 0);
			return m_backIdx - m_frontIdx;
		}

		inline bool try_lock() { return m_lock.try_lock(); }
		inline auto& Lock() { return m_lock; }

		inline void Push(const T& v)
		{
			auto idx = m_backIdx;
			m_backIdx = (m_backIdx + 1) % CAPACITY;
			m_tasks[idx] = v;
		}

		inline T Pop()
		{
			auto idx = m_frontIdx;
			m_frontIdx = (m_frontIdx + 1) % CAPACITY;
			return m_tasks[idx];
		}
	};

	friend class Context;

	Context* m_contexts[MAX_CONTEXTS] = {};

	StackCtx m_freeCtxs1 = {};
	//StackCtx m_freeCtxs2 = {};

	StackCtx m_pausedCtxs1 = {};
	//StackCtx m_pausedCtxs2 = {};

	TaskQueue<MarkTask> m_markTasks1 = {};
	//TaskQueue<MarkTask> m_markTasks2 = {};

	TaskQueue<SweepTask> m_sweepTasks1 = {};
	//TaskQueue<SweepTask> m_sweepTasks2 = {};

	ContextSharedHandle m_handle = {};

	byte** m_roots[MAX_ROOTS] = {};
	size_t m_rootsCount = 0;

	std::vector<byte*, STDAllocatorMalloc<byte*>> m_trackedCrossBoundaries;

	ManagedLocalScope::S* m_localScopes[MAX_THREADS] = {};
	size_t m_localScopesCount = 0;

	ManagedPage* m_pages[MAX_PAGES] = {};
	size_t m_pagesCount = 0;

	ManagedPool* m_pools[MAX_POOLS] = {};
	size_t m_poolsCount = 0;

	size_t m_phase = GC_PHASE::IDLE_PHASE;
	spinlock m_globalLock;

	std::vector<void*, STDAllocatorMalloc<void*>> m_deferFreeList;

	GCEvent* m_gcEvent = nullptr;

	ConcurrentQueue<byte*> m_crossBoundaries;

	/*size_t m_markedObjectCount = 0;
	size_t m_unmarkObjectCount = 0;*/
public:
	System()
	{
		for (size_t i = 0; i < MAX_CONTEXTS / 2; i++)
		{
			m_contexts[i] = NewMalloc<Context>(&m_handle);
			m_freeCtxs1.Push(m_contexts[i]);
		}
	}

	~System()
	{
		for (auto& p : m_deferFreeList)
		{
			free(p);
		}
		m_deferFreeList.clear();

		//if (!IsEndGC())
		if (m_phase != GC_PHASE::IDLE_PHASE)
		{
			if (m_phase == GC_PHASE::MARK_PHASE)
			{
				CONSOLE_WARN() << "GC MARK phase is still paused\n";
				CONSOLE_WARN() << m_markTasks1.Size() << " MARK tasks remain\n";
			}
			else if (m_phase == GC_PHASE::REMARK_PHASE)
			{
				CONSOLE_WARN() << "GC REMARK phase is still paused\n";
				CONSOLE_WARN() << m_markTasks1.Size() << " REMARK tasks remain\n";
			}
			else if(m_phase == GC_PHASE::SWEEP_PHASE)
			{
				CONSOLE_WARN() << "GC SWEEP phase is still paused\n";
				CONSOLE_WARN() << m_sweepTasks1.Size() << " SWEEP tasks remain\n";
			}
			else
			{
				assert(0);
			}
			CONSOLE_WARN() << m_pausedCtxs1.Size() << " GC contexts is paused\n";
		}

		for (size_t i = 0; i < MAX_CONTEXTS / 2; i++)
		{
			DeleteMalloc(m_contexts[i]);
		}
	}

private:
	Context* TryGet(StackCtx& s)
	{
		Context* ret = 0;
		if (s.Size() != 0)
		{
			while (ret == 0)
			{
				if (s.Lock().try_lock())
				{
					if (s.Size() != 0)
					{
						ret = s.Pop();
					}
					s.Lock().unlock();
					return ret;
				}
			}
		}
		return ret;
	}

public:
	void InitializeNewMarkCycle(bool remark = false)
	{
		if (remark == false)
		{
			while (m_phase != GC_PHASE::IDLE_PHASE)
			{
				std::this_thread::yield();
			}
		}

		m_globalLock.lock();

		if (remark == false && m_phase != GC_PHASE::IDLE_PHASE/*&& m_handle.m_gcCycles != m_handle.m_markCycles*/)
		{
			m_globalLock.unlock();
			return;
		}

		// just test
		const size_t NUM_MARK_ROOT_TASKS = 8;

		m_handle.m_counter.exchange(0, std::memory_order::memory_order_relaxed);
		m_handle.m_targetCounter = 0;

		m_markTasks1.Lock().lock();

		m_markTasks1.Reset();

		if (remark == false)
		{
			//CONSOLE_LOG()
			//	<< "============ New GC cycle by ThreadID [" << ThreadID::Get() << "] =============\n";

			/*m_markedObjectCount = 0;
			m_unmarkObjectCount = 0;*/

			if (!m_deferFreeList.empty())
			{
				for (auto& p : m_deferFreeList)
				{
					free(p);
				}
				m_deferFreeList.clear();
			}

			if (m_rootsCount > NUM_MARK_ROOT_TASKS)
			{
				size_t tempCount = 0;
				size_t rootPerTask = m_rootsCount / NUM_MARK_ROOT_TASKS;
				while (tempCount + rootPerTask < m_rootsCount)
				{
					m_markTasks1.Push({ &m_roots[tempCount], &m_roots[tempCount + rootPerTask], 0, 0 });
					tempCount += rootPerTask;
					m_handle.m_targetCounter++;
				}
				m_markTasks1.Push({ &m_roots[tempCount], &m_roots[m_rootsCount], 0, 0 });
				m_handle.m_targetCounter++;
			}
			else
			{
				for (size_t i = 0; i < m_rootsCount; i++)
				{
					m_markTasks1.Push({ &m_roots[i], &m_roots[i + 1], 0, 0 });
					m_handle.m_targetCounter++;
				}
			}

			// remember that
			for (size_t i = 0; i < m_pagesCount; i++)
			{
				/*if (ManagedLocalScope::s.lockedPage == m_pages[i])
				{
					m_pages[i]->SetupNewGCCycle();
					continue;
				}*/

				m_pages[i]->m_lock.lock();
				m_pages[i]->SetupNewGCCycle();
				m_pages[i]->m_lock.unlock();
			}

			for (size_t i = 0; i < m_poolsCount; i++)
			{
				m_pools[i]->m_lock.lock();
				m_pools[i]->SetupNewGCCycle();
				m_pools[i]->m_lock.unlock();
			}

		}

		//ManagedLocalScope::s.lock.unlock();
		//ManagedLocalScope::s.lock.unlock();
		for (size_t i = 0; i < m_localScopesCount; i++)
		{
			//m_localScopes[i]->tLock.lock();
			if (remark == false)
			{
				m_localScopes[i]->transactionLock.lock();
				m_localScopes[i]->isRecordingTransactions = true;
				m_localScopes[i]->transactionLock.unlock();
			}

			m_markTasks1.Push({ 0, 0, 0, 0, m_localScopes[i] });
			m_handle.m_targetCounter++;
		}


		if (remark == false)
		{
			// cross boundaries marking
			const size_t NUM_CROSS_BOUNDARIES_TASKS = 8;
			byte* ptr = 0;
			while (m_crossBoundaries.try_dequeue(ptr))
			{
				m_trackedCrossBoundaries.push_back(ptr);
			}

			if (m_trackedCrossBoundaries.size() > 0)
			{
				byte** trackedCrossBoundariesBegin = m_trackedCrossBoundaries.data();
				byte** trackedCrossBoundariesEnd = &m_trackedCrossBoundaries.back() + 1;
				size_t numElmPerTask = m_trackedCrossBoundaries.size() / NUM_CROSS_BOUNDARIES_TASKS;
				numElmPerTask = numElmPerTask == 0 ? 1 : numElmPerTask;

				for (size_t i = 0; i < NUM_CROSS_BOUNDARIES_TASKS; i++)
				{
					auto end = trackedCrossBoundariesBegin + numElmPerTask;
					end = end > trackedCrossBoundariesEnd ? trackedCrossBoundariesEnd : end;

					m_markTasks1.Push({ 0, 0, trackedCrossBoundariesBegin, end, 0 });
					m_handle.m_targetCounter++;

					trackedCrossBoundariesBegin = end;
					if (end == trackedCrossBoundariesEnd)
					{
						break;
					}
				}
			}
		}


		if (remark == false)
		{
			// begin new gc cycle
			//m_handle.m_gcCycles++;
			m_phase = GC_PHASE::MARK_PHASE;

			if (m_gcEvent)
			{
				m_gcEvent->OnMarkBegin();
			}
		}
		else
		{
			m_phase = GC_PHASE::REMARK_PHASE;

			if (m_gcEvent)
			{
				m_gcEvent->OnRemarkBegin();
			}
		}

		m_markTasks1.Lock().unlock();
		
		m_globalLock.unlock();
	}

	bool AssignMarkTask(Context* ctx)
	{
		bool ret = false;

		MarkTask task = {};
		m_markTasks1.Lock().lock();

		if (m_markTasks1.Size() != 0)
		{
			task = m_markTasks1.Pop();
			ret = true;
		}

		m_markTasks1.Lock().unlock();
		
		ctx->m_localScopeAllocatedIdx = 0;
		//ctx->m_transactionIdx = 0;

		ctx->m_localScope = task.localScope;
		ctx->m_rootIt = task.rootBegin;
		ctx->m_rootEnd = task.rootEnd;

		//ctx->m_sweepIt = 0;
		//ctx->m_sweepEnd = 0;
		ctx->m_trackedCrossBoundariesIt = task.crossBoundariesBegin;
		ctx->m_trackedCrossBoundariesEnd = task.crossBoundariesEnd;

		ctx->m_phase = m_phase;

		return ret;
	}

	bool AssignSweepTask(Context* ctx)
	{
		bool ret = false;

		SweepTask task = {};
		m_sweepTasks1.Lock().lock();

		if (m_sweepTasks1.Size() != 0)
		{
			task = m_sweepTasks1.Pop();
			ret = true;
		}

		m_sweepTasks1.Lock().unlock();

		//ctx->m_sweepIt = 0;
		ctx->m_sweepEnd = 0;
		//ctx->m_sweepLinkIt = 0;

		if (task.page)
		{
			//ctx->m_sweepIt = (AllocatedBlock*)task.page->m_buffer;
			ctx->m_sweepEnd = (AllocatedBlock*)(task.page->m_buffer + task.page->m_size);
			ctx->m_page = task.page;
			ctx->m_pool = 0;
		}

		if (task.pool)
		{
			//ctx->m_sweepLinkIt = task.pool->m_allocatedHead;
			ctx->m_pool = task.pool;
			ctx->m_page = 0;
		}

		//ctx->m_isMarkPhase = false;
		ctx->m_phase = m_phase;
		
		return ret;
	}

public:
	inline bool AssignTaskToContext(Context* ctx, bool allowStartNewCycle)
	{
		if (/*IsEndGC()*/m_phase == GC_PHASE::IDLE_PHASE && allowStartNewCycle)
		{
			//if (m_handle.m_gcCycles == m_handle.m_markCycles)
			//if (m_phase == GC_PHASE::IDLE_PHASE)
			//{
				InitializeNewMarkCycle();
			//}
		}

		if (m_phase == GC_PHASE::MARK_PHASE || m_phase == GC_PHASE::REMARK_PHASE/*m_handle.m_markCycles == m_handle.m_sweepCycles*/)
		{
			// mark phase
			return AssignMarkTask(ctx);
		}
		else if (m_phase == GC_PHASE::SWEEP_PHASE)
		{
			// sweep phase
			return AssignSweepTask(ctx);
		}

		return false;
	}

	void InitializeNewSweepCycle()
	{
		m_globalLock.lock();

		m_handle.m_counter.exchange(0, std::memory_order::memory_order_relaxed);
		m_handle.m_targetCounter = 0;

		m_sweepTasks1.Lock().lock();

		m_sweepTasks1.Reset();

		for (size_t i = 0; i < m_pagesCount; i++)
		{
			m_sweepTasks1.Push({ m_pages[i], 0 });
		}

		for (size_t i = 0; i < m_poolsCount; i++)
		{
			m_sweepTasks1.Push({ 0, m_pools[i] });
		}

		m_handle.m_targetCounter = m_pagesCount + m_poolsCount;

		m_sweepTasks1.Lock().unlock();

		//m_handle.m_markCycles++;

		m_phase = GC_PHASE::SWEEP_PHASE;

		if (m_gcEvent)
		{
			m_gcEvent->OnSweepBegin();
		}

		m_globalLock.unlock();
	}

	/*inline void EndGCCycle()
	{
		m_phase = GC_PHASE::IDLE_PHASE;
	}*/

public:
	Context* GetContext(bool allowStartNewCycle)
	{
		auto ret = TryGet(m_pausedCtxs1);
		if (ret == 0)
		{
			ret = TryGet(m_freeCtxs1);
			if (AssignTaskToContext(ret, allowStartNewCycle) == false)
			{
				ReturnContext(ret);
				ret = 0;
			}
		}
		return ret;
	}

	void PauseContext(Context* ctx)
	{
		assert(ctx != 0);
		m_pausedCtxs1.Lock().lock();
		m_pausedCtxs1.Push(ctx);
		m_pausedCtxs1.Lock().unlock();
	}

	void ReturnContext(Context* ctx)
	{
		assert(ctx != 0);
		m_freeCtxs1.Lock().lock();
		m_freeCtxs1.Push(ctx);
		m_freeCtxs1.Lock().unlock();
	}

	void RegisterPages(ManagedPage** pages, size_t count)
	{
		m_globalLock.lock();

		for (size_t i = 0; i < count; i++)
		{
			m_pages[m_pagesCount++] = pages[i];
		}

		m_globalLock.unlock();
	}

	void RegisterPools(ManagedPool** pools, size_t count)
	{
		m_globalLock.lock();

		for (size_t i = 0; i < count; i++)
		{
			m_pools[m_poolsCount++] = pools[i];
		}

		m_globalLock.unlock();
	}

	void RegisterRoots(byte*** begin, size_t count)
	{
		m_globalLock.lock();

		for (size_t i = 0; i < count; i++)
		{
			m_roots[m_rootsCount++] = begin[i];
		}

		m_globalLock.unlock();
	}

	void RegisterLocalScope(void* s)
	{
		m_globalLock.lock();
		auto mngl = (ManagedLocalScope::S*)s;
		if (mngl->isRegistered == false)
		{
			m_localScopes[m_localScopesCount++] = mngl;
			mngl->crossBoundaries = &m_crossBoundaries;
			mngl->isRegistered = true;
		}
		m_globalLock.unlock();
	}

	bool UnregisterLocalScope(void* s)
	{
		while (m_phase == GC_PHASE::REMARK_PHASE)
		{
			std::this_thread::yield();
		}

		m_globalLock.lock();

		size_t i;
		for (i = 0; i < m_localScopesCount; i++)
		{
			if (m_localScopes[i] == s)
			{
				break;
			}
		}

		if (i != m_localScopesCount)
		{
			m_localScopes[i] = m_localScopes[m_localScopesCount--];
		}
		else
		{
			assert(0);
		}

		m_globalLock.unlock();

		return m_localScopesCount == 0;
	}

	//inline bool IsEndGC()
	//{
	//	//return m_handle.m_gcCycles == m_handle.m_markCycles && m_handle.m_markCycles == m_handle.m_sweepCycles;
	//}

	inline bool IsEndGC()
	{
		return m_phase == GC_PHASE::IDLE_PHASE;
	}

	inline void DeferFree(void* p)
	{
		m_globalLock.lock();
		m_deferFreeList.push_back(p);
		m_globalLock.unlock();
	}

	// stableValue must be tracked stable value
	inline void ClearTrackedBoundariesOfStableValue(byte stableValue)
	{
		//m_globalLock.lock();
		for (size_t i = 0; i < m_trackedCrossBoundaries.size(); i++)
		{
			ManagedHandle* handle = (ManagedHandle*)m_trackedCrossBoundaries[i] - 1;
			if (handle->stableValue == stableValue)
			{
				m_trackedCrossBoundaries[i] = m_trackedCrossBoundaries.back();
				m_trackedCrossBoundaries.pop_back();
				i--;
			}
		}
		//m_globalLock.unlock();
	}
};

}

NAMESPACE_MEMORY_END