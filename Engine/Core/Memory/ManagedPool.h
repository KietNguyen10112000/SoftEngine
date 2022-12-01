#pragma once

#include <cassert>

#include "Core/Thread/Spinlock.h"

#include "TypeDef.h"
#include "Pool.h"
#include "Trace.h"
#include "ManagedPointers.h"
#include "ManagedHandle.h"
#include "ManagedLocalScope.h"
#include "MARK_COLOR.h"

NAMESPACE_MEMORY_BEGIN

namespace gc
{
	class ContextSharedHandle;
	class Context;
	class System;
}

/// 
/// page based alloc, max is 254 pages continuously
/// 
class ManagedPool
{
protected:
	friend class gc::ContextSharedHandle;
	friend class gc::Context;
	friend class gc::System;

	using Handle = ManagedHandle;
	//using HandlesPool = ManagedHandlesPool;
	//using HandleUsableMem = ManagedHandleUsableMem;

#define DefineHasClassMethod(method) 										\
	template <typename T>													\
	class Has_##method														\
	{																		\
		typedef char one;													\
		struct two { char x[2]; };											\
		template <typename C> static one test( decltype(&C::method) ) ;		\
		template <typename C> static two test(...); 						\
	public:																	\
		enum { value = sizeof(test<T>(0)) == sizeof(char) };				\
	};

	DefineHasClassMethod(Trace);

#undef DefineHasClassMethod

	struct Link
	{
		Link* prev = 0;
		Link* next = 0;
	};

	constexpr static size_t MAX_POOL = 256;

	constexpr static size_t EXTERNAL_SIZE = sizeof(Handle) + sizeof(Link);

	/**
	* -------------------------------------------------------------
	* | Link (16 bytes) | Handle (16 bytes) | usable mem (n bytes) 
	* -------------------------------------------------------------
	**/

	PoolN m_pools[MAX_POOL] = {};

	Link* m_allocatedHead = 0;

	// init once
	size_t m_PAGE_SIZE = 0;
	size_t m_INIT_EACH_POOL_SIZE = 0;

	size_t m_LOG2_PAGE_SIZE = 0;

	byte m_id = 0;
	

public:
	//byte m_markValue = 0;
	Link* m_sweepIt = 0;
	Link* m_sweepBackwardIt = 0;
	spinlock m_lock;

	void* m_DeallacateCallbackArg = 0;
	void (*m_DeallacateCallback)(void*, Handle*) = 0;

public:
	ManagedPool()
	{
		
	}

	ManagedPool(byte id, size_t PAGE_SIZE, size_t INIT_EACH_POOL_SIZE)
	{
		//m_pools = (PoolN*)malloc(POOLS_COUNT * sizeof(PoolN));
		//memset(m_pools, 0, sizeof(PoolN) * POOLS_COUNT);

		/*for (size_t i = 0; i < POOLS_COUNT; i++)
		{
			auto& pool = m_pools[i];
			new (&pool) PoolN(INIT_EACH_POOL_SIZE, 1, PAGE_SIZE * (i + 1));
		}*/
		m_id = id;

		m_PAGE_SIZE = PAGE_SIZE;
		m_INIT_EACH_POOL_SIZE = INIT_EACH_POOL_SIZE;
		m_LOG2_PAGE_SIZE = (size_t)log2(m_PAGE_SIZE);

		SetupNewGCCycle();
	}

	~ManagedPool()
	{
		/*for (size_t i = 0; i < MAX_POOL; i++)
		{
			auto pool = m_pools[i];

			if (pool)
			{
				pool->~PoolN();
				free(pool);
			}
		}*/
		if (m_allocatedHead)
		{
			size_t remainDeleteCalls = 0;
			CONSOLE_WARN() << "Memory leak detected\n";
			ManagedPool::ForEachAllocatedBlocks(
				[&](ManagedHandle* handle)
				{
					//CONSOLE_WARN() << "[" << (void*)handle->GetUsableMemAddress() << "]: " << handle->TotalSize() << " bytes\n";
					remainDeleteCalls++;
				}
			);
			CONSOLE_WARN() << "Missing: " << remainDeleteCalls << " delete calls\n";
		}
	}

public:
	inline void Deallocate(void* p)
	{
		Handle* handle = ((Handle*)p) - 1;
		Link* link = (Link*)(handle - 1);

		auto next = link->next;
		auto prev = link->prev;

		if (next)
		{
			next->prev = prev;
		}

		if (prev)
		{
			prev->next = next;
		}
		else
		{
			assert(link == m_allocatedHead);
			m_allocatedHead = next;
		}

		assert(handle->poolHint != -1);
		assert(handle->poolHint < (MAX_POOL - 1));

		if (m_DeallacateCallback)
		{
			m_DeallacateCallback(m_DeallacateCallbackArg, handle);
		}

		m_pools[handle->poolHint].Deallocate(link);
	}

	inline void InitializePool(size_t id)
	{
		auto& pool = m_pools[id];
		new (&pool) PoolN(m_INIT_EACH_POOL_SIZE, 1, m_PAGE_SIZE * (id + 1));
	}

public:
	inline static byte ChoosePool(size_t nBytes, const size_t PAGE_SIZE, bool withExternalSize = true)
	{
		nBytes += (withExternalSize ? EXTERNAL_SIZE : 0);
		return (byte)((nBytes / PAGE_SIZE + (nBytes % PAGE_SIZE != 0)) - 1);
	}

	inline static size_t Align(size_t nBytes, const size_t PAGE_SIZE, bool withExternalSize = true)
	{
		nBytes += (withExternalSize ? EXTERNAL_SIZE : 0);
		return (nBytes / PAGE_SIZE + (nBytes % PAGE_SIZE != 0)) * PAGE_SIZE;
	}

	inline void SetDeallocateCallback(void* ptr, void(*callback)(void*, ManagedHandle*))
	{
		m_DeallacateCallbackArg = ptr;
		m_DeallacateCallback = callback;
	}

	inline byte ChoosePool(size_t nBytes)
	{
		return (byte)((nBytes / m_PAGE_SIZE + (nBytes % m_PAGE_SIZE != 0)) - 1);
	}

	inline void ReserveFor(size_t nBytes)
	{
		nBytes += EXTERNAL_SIZE;

		auto id = ChoosePool(nBytes);
		auto& pool = m_pools[id];

		if (pool.IsValid() == false)
		{
			InitializePool(id);
			return;
		}

		pool.Deallocate(pool.Allocate());
	}

	inline void SetupNewGCCycle()
	{
		m_sweepIt = m_allocatedHead;
		m_sweepBackwardIt = m_allocatedHead;
	}

	inline bool IsEnoughMemory(size_t nBytes)
	{
		nBytes += EXTERNAL_SIZE;

		auto& pool = m_pools[ChoosePool(nBytes)];
		if (pool.IsValid() == false) return false;
		if (pool.IsEmpty()) return false;
		return true;
	}

public:
	ManagedHandle* Allocate(size_t nBytes)
	{
		size_t size = nBytes + EXTERNAL_SIZE;

		byte poolId = ChoosePool(size);
		PoolN& pool = m_pools[poolId];

		if (pool.IsValid() == false)
		{
			InitializePool(poolId);
		}

		Link* link = (Link*)pool.Allocate();
		Handle* handle = (Handle*)(link + 1);

		//handle->poolPageSize = m_PAGE_SIZE;
		handle->poolId = m_id;
		handle->log2OfPoolPageSize = m_LOG2_PAGE_SIZE;
		handle->poolHint = poolId;
		handle->paddingBytes = pool.m_blockSize - size;

		// always black
		handle->marked = MARK_COLOR::GRAY;

		handle->pageId = -1;

		link->next = m_allocatedHead;
		if (m_allocatedHead) m_allocatedHead->prev = link;
		link->prev = 0;
		m_allocatedHead = link;

		if (m_sweepBackwardIt == nullptr)
		{
			m_sweepBackwardIt = m_allocatedHead;
		}

		return handle;
	}

//	template <typename T>
//	[[nodiscard]] inline TempPtr<T> Allocate(size_t count)
//	{
//		size_t size = count * sizeof(T) + EXTERNAL_SIZE;
//
//		byte poolId = ChoosePool(size);
//		PoolN& pool = m_pools[poolId];
//
//		if (pool.IsValid() == false)
//		{
//			InitializePool(poolId);
//		}
//
//		Link* link = pool.Allocate();
//		Handle* handle = (Handle*)(link + 1);
//
//		handle->poolHint = poolId;
//		handle->paddingBytes = pool.m_blockSize - size;
//
//		 always 0
//		handle->marked = 0;
//
//		handle->pageId = -1;
//
//		if constexpr (std::is_base_of_v<Traceable<T>, T>)
//		{
//			static_assert(Has_Trace<T>::value, "Traceable must provides trace method \"void Trace(Tracer* tracer);\"");
//			handle->traceTable = Traceable<T>::GetTraceTable();
//		}
//		else
//		{
//			handle->traceTable = TraceableNone<T>::GetTraceTable(); //-1;
//		}
//		
//
//		link->next = m_allocatedHead;
//		if (m_allocatedHead) m_allocatedHead->prev = link;
//		link->prev = 0;
//		m_allocatedHead = link;
//
//		TempPtr<T> ret;
//
//		ret.m_ptr = (T*)(handle->GetUsableMemAddress());
//		ret.m_idx = ManagedLocalScope::Add((byte*)ret.m_ptr);
//
//#ifdef _DEBUG
//		memset(ret.m_ptr, 0, sizeof(T) * count);
//#endif // _DEBUG
//
//		return ret;
//	}

	// iterate over allocated blocks
	// callback = function(Handle* handle, ...)
	// return last block
	template <typename C, typename... Args>
	void ForEachAllocatedBlocks(C callback, Args&&... args)
	{
		auto it = m_allocatedHead;

		while (it)
		{
			auto next = it->next;
			callback((Handle*)(it + 1), std::forward<Args>(args)...);
			it = next;
		}
	}

};


NAMESPACE_MEMORY_END