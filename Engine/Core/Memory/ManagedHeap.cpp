#include "ManagedHeap.h"

#include "ManagedPool.h"
#include "ManagedPage.h"
#include "GC.h"

#include "GCEvent.h"

NAMESPACE_MEMORY_BEGIN

class ManagedHeapGCEvent : public GCEvent
{
private:
	ManagedHeap* m_heap = nullptr;

public:
	ManagedHeapGCEvent(ManagedHeap* heap)
	{
		m_heap = heap;
	}

public:
	virtual void OnMarkBegin() override
	{

	}

	virtual void OnMarkEnd() override
	{

	}

	virtual void OnRemarkBegin() override
	{

	}

	virtual void OnRemarkEnd() override
	{

	}

	virtual void OnSweepBegin() override
	{

	}

	virtual void OnSweepEnd() override
	{
		if (m_heap->m_tinyObjectsTotalAllocatedBytes > m_heap->m_tinyObjectNextAllocatedBytesToPerformGC)
		{
			m_heap->m_tinyObjectNextAllocatedBytesToPerformGC *= 2;
		}

		if (m_heap->m_smallObjectsTotalAllocatedBytes > m_heap->m_smallObjectNextAllocatedBytesToPerformGC)
		{
			m_heap->m_smallObjectNextAllocatedBytesToPerformGC *= 2;
		}

		m_heap->m_isNeedGC = false;
	}
};


ManagedHeap::ManagedHeap(bool GC)
{
	m_isGCActivated = GC;

	m_pagesHead = ::malloc(sizeof(ManagedPage) * LARGE_OBJECT_PAGES_COUNT);
	::memset(m_pagesHead, 0, sizeof(ManagedPage) * LARGE_OBJECT_PAGES_COUNT);
	m_allocatedPageIt = (ManagedPage*)m_pagesHead;

	byte poolId = 0;
	m_poolsHead = ::malloc(sizeof(ManagedPool) * TOTAL_POOLS);
	auto poolIt = (ManagedPool*)m_poolsHead;

	// tiny
	for (auto& pool : m_tinyObjectPools)
	{
		//pool = new ManagedPool(poolId++, TINY_OBJECT_PAGE_SIZE, TINY_OBJECT_EACH_POOL_CHUNK_SIZE);
		pool = poolIt;
		new (pool) ManagedPool(poolId++, TINY_OBJECT_PAGE_SIZE, TINY_OBJECT_EACH_POOL_CHUNK_SIZE);
		pool->SetDeallocateCallback(this, [](void* self, ManagedHandle* handle)
			{
				ManagedHeap* heap = (ManagedHeap*)self;
				heap->m_tinyObjectsTotalAllocatedBytes_ -= handle->TotalSize();
				heap->m_totalAllocatedBytes_ -= handle->TotalSize();
			}
		);
		poolIt++;
	}
	if (m_isGCActivated) gc::RegisterPools(m_tinyObjectPools, TINY_OBJECT_POOLS_COUNT);

	// small
	for (auto& pool : m_smallObjectPools)
	{
		//pool = new ManagedPool(poolId++, SMALL_OBJECT_PAGE_SIZE, SMALL_OBJECT_EACH_POOL_CHUNK_SIZE);
		pool = poolIt;
		new (pool) ManagedPool(poolId++, SMALL_OBJECT_PAGE_SIZE, SMALL_OBJECT_EACH_POOL_CHUNK_SIZE);
		pool->SetDeallocateCallback(this, [](void* self, ManagedHandle* handle)
			{
				ManagedHeap* heap = (ManagedHeap*)self;
				heap->m_smallObjectsTotalAllocatedBytes_ -= handle->TotalSize();
				heap->m_totalAllocatedBytes_ -= handle->TotalSize();
			}
		);
		poolIt++;
	}
	if (m_isGCActivated) gc::RegisterPools(m_smallObjectPools, SMALL_OBJECT_POOLS_COUNT);

	// meidum
	for (auto& pool : m_mediumObjectPools)
	{
		//pool = new ManagedPool(poolId++, MEDIUM_OBJECT_PAGE_SIZE, MEDIUM_OBJECT_EACH_POOL_CHUNK_SIZE);
		pool = poolIt;
		new (pool) ManagedPool(poolId++, MEDIUM_OBJECT_PAGE_SIZE, MEDIUM_OBJECT_EACH_POOL_CHUNK_SIZE);
		pool->SetDeallocateCallback(this, [](void* self, ManagedHandle* handle)
			{
				ManagedHeap* heap = (ManagedHeap*)self;
				heap->m_totalMediumObjectAllocatedBytes_ -= handle->TotalSize();
				heap->m_totalAllocatedBytes_ -= handle->TotalSize();
			}
		);
		poolIt++;
	}
	if (m_isGCActivated) gc::RegisterPools(m_mediumObjectPools, MEDIUM_OBJECT_POOLS_COUNT);


	for (auto& v : m_nextAllocateMediumPoolId)
	{
		v = INVALID_ID;
	}


	//=================================================================================================

	size_t i = 0;
	for (auto& pages : m_pages)
	{
		for (auto& page : pages)
		{
			page = m_allocatedPageIt++;
			new (page) ManagedPage((byte)i);
			i++;
		}
	}

	if (m_isGCActivated) gc::RegisterPages(&m_pages[0][0], LARGE_OBJECT_PAGES_COUNT);

	for (auto& v : m_initializedPages)
	{
		v = 0;
	}


	if (GC)
	{
		m_gcEvent = NewMalloc<ManagedHeapGCEvent>(this);
		gc::SetGCEvent(m_gcEvent);
	}
}

ManagedHeap::~ManagedHeap()
{
	auto pools = &m_tinyObjectPools[0];
	for (size_t i = 0; i < TOTAL_POOLS; i++)
	{
		pools[i]->~ManagedPool();
	}
	::free(m_poolsHead);

	for (auto& pages : m_pages)
	{
		for (auto& page : pages)
		{
			if (page)
			{
				if (page->m_isInitialized) page->~ManagedPage();
			}
		}
	}
	::free(m_pagesHead);

	if (m_gcEvent)
	{
		DeleteMalloc(m_gcEvent);
	}
}

void ManagedHeap::PerformGC(ThreadContext* ctx, spinlock& lock)
{
	//if (m_isGCActivated)
	//{
	//	// perform full GC and lock the lock
	//	if (lock.try_lock())
	//	{
	//		gc::Resume(-1, gc::GC_RESUME_FLAG::ALLOW_START_NEW_GC);
	//	}
	//	else
	//	{
	//		while (lock.try_lock() == false)
	//		{
	//			gc::Resume(-1, gc::GC_RESUME_FLAG::RETURN_ON_EMPTY_TASK);
	//			std::this_thread::yield();
	//		}
	//	}
	//}
	//else
	//{
	//	lock.lock();
	//}

	if (m_isGCActivated)
	{
		m_isNeedGC = true;
	}
	lock.lock();
}

size_t ManagedHeap::ChooseAndLockTinyObjectPool(ThreadContext* ctx, size_t nBytes)
{
	auto nAlignedBytes = ManagedPool::Align(nBytes, TINY_OBJECT_PAGE_SIZE);
	assert(nAlignedBytes <= TINY_OBJECT_MAX_SIZE);

	if (m_tinyObjectsTotalAllocatedBytes_.fetch_add(nAlignedBytes) + nAlignedBytes > m_tinyObjectNextAllocatedBytesToPerformGC)
	{
		//OnSmallObjectPerformGC(ctx, nBytes);
		PerformGC(ctx, m_tinyObjectPerformGCLock);

		/*if (m_tinyObjectsTotalAllocatedBytes > m_tinyObjectNextAllocatedBytesToPerformGC)
		{
			m_tinyObjectNextAllocatedBytesToPerformGC *= 2;
		}*/

		m_tinyObjectPerformGCLock.unlock();
	}

	//constexpr auto TINY_OBJECT_POOLS_COUNT_1 = TINY_OBJECT_POOLS_COUNT - 1;

	auto id = ctx->tinyPoolId;
	auto endId = ctx->tinyPoolId;
	id = (id + 1) % TINY_OBJECT_POOLS_COUNT;

	auto maxId = FindPoolHasMaxAllocatedBytes<TINY_OBJECT_POOLS_COUNT>(nBytes, m_tinyObjectPools);

	// use ring buffer to balance allocated objects on pools
	// in each allocation call, the pool has max allocated bytes of nBytes will be ignored
	while (true)
	{
		if (maxId != id && m_tinyObjectPools[id]->m_lock.try_lock())
		{
			break;
		}
		if (id == endId)
		{
			std::this_thread::yield();
		}
		id = (id + 1) % TINY_OBJECT_POOLS_COUNT;
	}

	if (m_tinyObjectPools[id]->IsEnoughMemory(nBytes) == false)
	{
		m_totalHeapSize_ += TINY_OBJECT_EACH_POOL_CHUNK_SIZE;
	}

	ctx->tinyPoolId = id;

	return id;
}

size_t ManagedHeap::ChooseAndLockSmallObjectPool(ThreadContext* ctx, size_t nBytes)
{
	auto nAlignedBytes = ManagedPool::Align(nBytes, SMALL_OBJECT_PAGE_SIZE);
	assert(nAlignedBytes <= SMALL_OBJECT_MAX_SIZE);

	if (m_smallObjectsTotalAllocatedBytes_.fetch_add(nAlignedBytes) + nAlignedBytes > m_smallObjectNextAllocatedBytesToPerformGC)
	{
		//OnSmallObjectPerformGC(ctx, nBytes);
		PerformGC(ctx, m_smallObjectPerformGCLock);

		/*if (m_smallObjectsTotalAllocatedBytes > m_smallObjectNextAllocatedBytesToPerformGC)
		{
			m_smallObjectNextAllocatedBytesToPerformGC *= 2;
		}*/

		m_smallObjectPerformGCLock.unlock();
	}

	auto id = ctx->smallPoolId;
	auto endId = ctx->smallPoolId;
	id = (id + 1) % SMALL_OBJECT_POOLS_COUNT;

	auto maxId = FindPoolHasMaxAllocatedBytes<SMALL_OBJECT_POOLS_COUNT>(nBytes, m_smallObjectPools);

	// use ring buffer to balance allocated objects on pools
	while (true)
	{
		if (maxId != id && m_smallObjectPools[id]->m_lock.try_lock())
		{
			break;
		}
		if (id == endId)
		{
			std::this_thread::yield();
		}
		id = (id + 1) % SMALL_OBJECT_POOLS_COUNT;
	}

	if (m_smallObjectPools[id]->IsEnoughMemory(nBytes) == false)
	{
		m_totalHeapSize_ += SMALL_OBJECT_EACH_POOL_CHUNK_SIZE;
	}


	ctx->smallPoolId = id;

	//m_totalAllocatedBytes_ += nAlignedBytes;

	return id;
}

size_t ManagedHeap::ChooseAndLockMediumObjectPool(ThreadContext* ctx, size_t nBytes)
{
	auto nAlignedBytes = ManagedPool::Align(nBytes, MEDIUM_OBJECT_PAGE_SIZE);
	assert(nAlignedBytes <= MEDIUM_OBJECT_MAX_SIZE);

	auto id = ctx->mediumPoolId;
	auto endId = ctx->mediumPoolId;
	id = (id + 1) % MEDIUM_OBJECT_POOLS_COUNT;

	size_t totalNotEnoughMemPools = 0;
	while (true)
	{
		if (totalNotEnoughMemPools == MEDIUM_OBJECT_POOLS_COUNT)
		{
			break;
		}

		// tie memory
		if (m_mediumObjectPools[id]->IsEnoughMemory(nBytes))
		{
			if (m_mediumObjectPools[id]->m_lock.try_lock())
			{
				// check again
				if (m_mediumObjectPools[id]->IsEnoughMemory(nBytes))
				{
					break;
				}
				else
				{
					totalNotEnoughMemPools++;
					m_mediumObjectPools[id]->m_lock.unlock();
				}
			}
		}
		else
		{
			totalNotEnoughMemPools++;
		}

		if (id == endId)
		{
			std::this_thread::yield();
		}
		id = (id + 1) % MEDIUM_OBJECT_POOLS_COUNT;
	}

	if (totalNotEnoughMemPools == MEDIUM_OBJECT_POOLS_COUNT)
	{
		// process heap out of memory event
		OnMediumObjectOutOfMemory(ctx, nBytes);

		// retry
		id = ChooseAndLockMediumObjectPool(ctx, nBytes);
	}
	else
	{
		ctx->mediumPoolId = id;
		m_totalMediumObjectAllocatedBytes_ += nAlignedBytes;
		//m_totalAllocatedBytes_ += nAlignedBytes;
	}

	return id;
}

void ManagedHeap::MediumObjectReserveFor(size_t nBytes)
{
	auto& id = m_nextAllocateMediumPoolId[ManagedPool::ChoosePool(nBytes, MEDIUM_OBJECT_PAGE_SIZE)];
	auto& targetPool = m_mediumObjectPools[id];
	id = (id + 1) % MEDIUM_OBJECT_POOLS_COUNT;

	targetPool->m_lock.lock();
	targetPool->ReserveFor(nBytes);
	m_totalHeapSize_ += MEDIUM_OBJECT_EACH_POOL_CHUNK_SIZE;
	m_totalMediumObjectReservedSize_ += MEDIUM_OBJECT_EACH_POOL_CHUNK_SIZE;
	targetPool->m_lock.unlock();
}

void ManagedHeap::OnMediumObjectOutOfMemory(ThreadContext* ctx, size_t nBytes)
{
	auto& id = m_nextAllocateMediumPoolId[ManagedPool::ChoosePool(nBytes, MEDIUM_OBJECT_PAGE_SIZE)];

	if (id == INVALID_ID)
	{
		m_meidumObjectOutOfMemoryLock.lock();
		if (id == INVALID_ID)
		{
			id = 0;
			MediumObjectReserveFor(nBytes);
		}
		m_meidumObjectOutOfMemoryLock.unlock();
		return;
	}

	PerformGC(ctx, m_meidumObjectOutOfMemoryLock);

	bool successed = false;
	for (auto& pool : m_mediumObjectPools)
	{
		if (pool->IsEnoughMemory(nBytes))
		{
			successed = true;
			break;
		}
	}

	if (!successed)
	{
		MediumObjectReserveFor(nBytes);
	}
	
end:
	m_meidumObjectOutOfMemoryLock.unlock();
}

size_t ManagedHeap::ChooseAndLockLargeObjectPage(ThreadContext* ctx, size_t nBytes)
{
	auto id = ctx->pageId;
	auto endId = ctx->pageId;
	id = (id + 1) % LARGE_OBJECT_PAGES_COUNT;

	auto pages = &m_pages[0][0];
	size_t totalNotEnoughMemory = 0;
	while (true)
	{
		if (totalNotEnoughMemory == m_totalPages)
		{
			break;
		}

		if (pages[id]->m_isInitialized)
		{
			if (pages[id]->IsEnoughMemory(nBytes))
			{
				if (pages[id]->m_lock.try_lock())
				{
					if (pages[id]->IsEnoughMemory(nBytes))
					{
						break;
					}
					else
					{
						pages[id]->m_lock.unlock();
						totalNotEnoughMemory++;
					}
				}
			}
			else
			{
				totalNotEnoughMemory++;
			}
		}
		
		if (id == endId)
		{
			std::this_thread::yield();
		}
		id = (id + 1) % LARGE_OBJECT_PAGES_COUNT;
	}

	if (totalNotEnoughMemory == m_totalPages)
	{
		OnLargeObjectOutOfMemory(ctx, nBytes);
		id = ChooseAndLockLargeObjectPage(ctx, nBytes);
	}
	else
	{
		ctx->pageId = id;
	}

	return id;
}

void ManagedHeap::LargeObjectInitPage(size_t row)
{
	auto pages = &m_pages[0][0];

	auto initializedPageInRow = m_initializedPages[row];
	size_t id = row * LARGE_OBJECT_PAGES_COLUMNS + initializedPageInRow;

	if (initializedPageInRow == LARGE_OBJECT_PAGES_COLUMNS)
	{
		while (m_initializedPages[row] == LARGE_OBJECT_PAGES_COLUMNS)
		{
			row++;
		}

		//m_initializedPages[row]++;
		id = row * LARGE_OBJECT_PAGES_COLUMNS + m_initializedPages[row];
	}
	
	m_initializedPages[row]++;
	
	assert(pages[id]->m_isInitialized == false);

	m_totalPages++;
	auto pageSize = LARGE_OBJECT_PAGES_START_SIZE * (size_t)std::pow(size_t(2), row);
	m_totalHeapSize_ += pageSize;
	//pages[id] = new ManagedPage(id, pageSize);
	//pages[id] = m_allocatedPageIt++;

	auto& page = pages[id];
	page->m_lock.lock();

	page->Initialize(id, pageSize);
	page->SetDeallocateCallback(this, [](void* self, ManagedHandle* handle)
		{
			ManagedHeap* heap = (ManagedHeap*)self;
			heap->m_totalAllocatedBytes_ -= handle->TotalSize();
		}
	);

	page->m_lock.unlock();
	//if (m_isGCActivated) gc::RegisterPage(pages[id]);
}

void ManagedHeap::OnLargeObjectOutOfMemory(ThreadContext* ctx, size_t nBytes)
{
	auto pageRow = ChoosePage(nBytes);
	assert(pageRow < LARGE_OBJECT_PAGES_ROWS);
	auto& initializedPage = m_initializedPages[pageRow];
	if (initializedPage == 0)
	{
		m_largeObjectOutOfMemoryLock.lock();
		if (m_initializedPages[pageRow] == 0) LargeObjectInitPage(pageRow);
		m_largeObjectOutOfMemoryLock.unlock();
		return;
	}

	PerformGC(ctx, m_largeObjectOutOfMemoryLock);

	auto pages = &m_pages[0][0];

	// check if gc reclaimed memory
	bool successed = false;
	for (size_t i = 0; i < LARGE_OBJECT_PAGES_COUNT; i++)
	{
		if (pages[i] && pages[i]->IsEnoughMemory(nBytes))
		{
			successed = true;
		}
	}

	if (!successed)
	{
		LargeObjectInitPage(pageRow);
	}

end:
	m_largeObjectOutOfMemoryLock.unlock();
}


ManagedHandle* ManagedHeap::Allocate(size_t nBytes, TraceTable* table, byte** managedLocalBlock, byte stableValue)
{
	ManagedHandle* ret = 0;
	auto realSize = nBytes + EXTERNAL_SIZE;

	spinlock* lock = 0;

	auto tid = ThreadID::Get();

	if (tid == -1)
	{
		tid = 0;
	}

	if (realSize <= TINY_OBJECT_MAX_SIZE)
	{
		auto id = ChooseAndLockTinyObjectPool(&m_contexts[tid], nBytes);
		auto& pool = m_tinyObjectPools[id];

		ret = pool->Allocate(nBytes);
		lock = &pool->m_lock;

		if (pool->GetTotalAllocatedBytesOf(nBytes) / (float)pool->GetTotalReservedBytesFor(nBytes) 
			> MEMORY_THRESHOLD_TO_PERFORM_GC)
		{
			m_isNeedGC = true;
		}
	}
	else if (realSize <= SMALL_OBJECT_MAX_SIZE)
	{
		auto id = ChooseAndLockSmallObjectPool(&m_contexts[tid], nBytes);
		auto& pool = m_smallObjectPools[id];

		ret = pool->Allocate(nBytes);
		lock = &pool->m_lock;

		if (pool->GetTotalAllocatedBytesOf(nBytes) / (float)pool->GetTotalReservedBytesFor(nBytes)
			> MEMORY_THRESHOLD_TO_PERFORM_GC)
		{
			m_isNeedGC = true;
		}
	}
	else if (realSize <= MEDIUM_OBJECT_MAX_SIZE)
	{
		auto id = ChooseAndLockMediumObjectPool(&m_contexts[tid], nBytes);
		auto& pool = m_mediumObjectPools[id];

		ret = pool->Allocate(nBytes);
		lock = &pool->m_lock;

		if (pool->GetTotalAllocatedBytesOf(nBytes) / (float)pool->GetTotalReservedBytesFor(nBytes)
			> MEMORY_THRESHOLD_TO_PERFORM_GC)
		{
			m_isNeedGC = true;
		}
	}
	else
	{
		auto id = ChooseAndLockLargeObjectPage(&m_contexts[tid], nBytes);
		auto& page = (&m_pages[0][0])[id];

		ret = page->Allocate(nBytes);
		lock = &page->m_lock;

		if (page->GetTotalAllocatedBytes() / (float)page->GetSize() > MEMORY_THRESHOLD_TO_PERFORM_GC)
		{
			m_isNeedGC = true;
		}
	}

	m_totalAllocatedBytes_ += ret->TotalSize();

	ret->traceTable = table;

#ifdef _DEBUG
	::memset(ret->GetUsableMemAddress(), -2, ret->UsableSize());
#endif // _DEBUG


	if (managedLocalBlock)
	{
		::memset(ret->GetUsableMemAddress(), 0, ret->UsableSize());
		*managedLocalBlock = ret->GetUsableMemAddress();
	}

	ret->stableValue = stableValue;
	if (stableValue != 0)
	{
		ret->marked = MARK_COLOR::WHITE;
	}

	lock->unlock();

	return ret;
}

void ManagedHeap::Deallocate(ManagedHandle* handle)
{
	if (m_isGCActivated) return;

	if (!handle->IsLargeObject())
	{
		auto pools = &m_tinyObjectPools[0];
		auto& pool = pools[handle->poolId];
		pool->m_lock.lock();
		pool->Deallocate(handle->GetUsableMemAddress());
		pool->m_lock.unlock();
		return;
	}
	
	auto& page = (&m_pages[0][0])[handle->pageId];
	page->m_lock.lock();
	page->Deallocate(handle->GetUsableMemAddress());
	page->m_lock.unlock();
}

void ManagedHeap::FreeStableObjects(byte stableValue, void* userPtr, void(*callback)(void*, ManagedHeap*, ManagedHandle*))
{
	auto trackedStableValue = MAKE_TRACK_STABLE_VALUE(stableValue);

	auto heap = this;
	auto pools = &m_tinyObjectPools[0];
	for (size_t i = 0; i < TOTAL_POOLS; i++)
	{
		pools[i]->ForEachAllocatedBlocks([&](ManagedHandle* handle)
			{
				if (handle->stableValue == stableValue || handle->stableValue == trackedStableValue)
				{
					if (callback) callback(userPtr, heap, handle);

					if (handle->traceTable)
					{
						auto dtor = handle->traceTable->dtor;
						if (dtor)
						{
							dtor(handle->GetUsableMemAddress());
						}
					}

					pools[i]->Deallocate(handle->GetUsableMemAddress());
				}
			}
		);
	}


	auto pages = &m_pages[0][0];
	for (size_t i = 0; i < LARGE_OBJECT_PAGES_COUNT; i++)
	{
		if (pages[i])
		{
			pages[i]->ForEachAllocatedBlocks([&](ManagedHandle* handle)
				{
					if (handle->stableValue == stableValue || handle->stableValue == trackedStableValue)
					{
						if (callback) callback(userPtr, heap, handle);
						pages[i]->Deallocate(handle->GetUsableMemAddress());
					}
				}
			);
		}
	}
}

//bool ManagedHeap::IsNeedGC()
//{
//	if (m_isGCActivated == false)
//	{
//		return false;
//	}
//
//	if (m_isNeedGC)
//	{
//		return true;
//	}
//	return false;
//}

NAMESPACE_MEMORY_END