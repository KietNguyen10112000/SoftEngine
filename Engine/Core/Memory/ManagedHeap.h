#pragma once

#include "TypeDef.h"
#include "ManagedHandle.h"

#include "Core/Thread/Spinlock.h"

#include <atomic>

NAMESPACE_MEMORY_BEGIN

class ManagedPage;
class ManagedPool;
struct ManagedHandle;

class ManagedHeap
{
public:
#ifdef _DEBUG
    constexpr static size_t RELEASE_POOL_COEFF = 1;
#else
    constexpr static size_t RELEASE_POOL_COEFF = 16;
#endif // _DEBUG

    constexpr static size_t MAX_THREADS = 128;

    constexpr static size_t TINY_OBJECT_PAGE_SIZE = 64;
    constexpr static size_t TINY_OBJECT_MAX_SIZE = 512;
    constexpr static size_t TINY_OBJECT_MAX_PAGES = TINY_OBJECT_MAX_SIZE / TINY_OBJECT_PAGE_SIZE;
    constexpr static size_t TINY_OBJECT_POOLS_COUNT = 4;

//#ifdef _DEBUG
    constexpr static size_t TINY_OBJECT_EACH_POOL_CHUNK_SIZE = 16 * KB * RELEASE_POOL_COEFF;
    constexpr static size_t TINY_OBJECT_GC_START_SIZE = 1 * MB * RELEASE_POOL_COEFF;
//#else
//    constexpr static size_t TINY_OBJECT_EACH_POOL_CHUNK_SIZE = 1 * MB;
//    constexpr static size_t TINY_OBJECT_GC_START_SIZE = 16 * MB;
//#endif // _DEBUG


    constexpr static size_t SMALL_OBJECT_PAGE_SIZE = 1 * KB;
    constexpr static size_t SMALL_OBJECT_MAX_SIZE = 32 * KB;
    constexpr static size_t SMALL_OBJECT_MAX_PAGES = SMALL_OBJECT_MAX_SIZE / SMALL_OBJECT_PAGE_SIZE;
    constexpr static size_t SMALL_OBJECT_POOLS_COUNT = 4;

//#ifdef _DEBUG
    constexpr static size_t SMALL_OBJECT_EACH_POOL_CHUNK_SIZE = 256 * KB * RELEASE_POOL_COEFF;
    constexpr static size_t SMALL_OBJECT_GC_START_SIZE = 16 * MB * RELEASE_POOL_COEFF;
//#else
//    constexpr static size_t SMALL_OBJECT_EACH_POOL_CHUNK_SIZE = 8 * MB;
//    constexpr static size_t SMALL_OBJECT_GC_START_SIZE = 64 * MB;
//#endif // _DEBUG


    constexpr static size_t MEDIUM_OBJECT_PAGE_SIZE = 8 * KB;
    constexpr static size_t MEDIUM_OBJECT_MAX_SIZE = 256 * KB;
    constexpr static size_t MEDIUM_OBJECT_MAX_PAGES = MEDIUM_OBJECT_MAX_SIZE / MEDIUM_OBJECT_PAGE_SIZE;
    constexpr static size_t MEDIUM_OBJECT_POOLS_COUNT = 4;

//#ifdef _DEBUG
    constexpr static size_t MEDIUM_OBJECT_EACH_POOL_CHUNK_SIZE = 8 * MB * RELEASE_POOL_COEFF;
//#else
//    constexpr static size_t MEDIUM_OBJECT_EACH_POOL_CHUNK_SIZE = 128 * MB;
//#endif // _DEBUG


    constexpr static size_t TOTAL_POOLS = 
        TINY_OBJECT_POOLS_COUNT 
        + SMALL_OBJECT_POOLS_COUNT 
        + MEDIUM_OBJECT_POOLS_COUNT;

    // for large object

    // pages in same row have same size, [0->3], [4->7], ...
    // pages in row(i + 1) = size of page in row(i) * 2
    constexpr static size_t LARGE_OBJECT_PAGES_COLUMNS = 4;
    constexpr static size_t LARGE_OBJECT_PAGES_ROWS = 32;

    constexpr static size_t LARGE_OBJECT_PAGES_COUNT = LARGE_OBJECT_PAGES_COLUMNS * LARGE_OBJECT_PAGES_ROWS;

#ifdef _DEBUG
    // size of pages in row 0
    constexpr static size_t LARGE_OBJECT_PAGES_START_SIZE = 64 * MB;
#else
    constexpr static size_t LARGE_OBJECT_PAGES_START_SIZE = 256 * MB;
#endif
    
    // 1 page will allocated when construct ManagedHeap
    constexpr static size_t LARGE_OBJECT_DEFAULT_PAGES_COUNT = 1;

    constexpr static size_t INVALID_ID = -1;
    constexpr static size_t EXTERNAL_SIZE = sizeof(ManagedHandle) + sizeof(AllocatedBlock);

public:
    struct ThreadContext
    {
        // store last success allocate call
        size_t tinyPoolId = 0;
        size_t smallPoolId = 0;
        size_t mediumPoolId = 0;
        size_t pageId = 0;

        // temp buffer
        //size_t IDs[LARGE_OBJECT_PAGES_COUNT] = {};
        //size_t IDsSize = 0;
    };

public:
    // for system malloc
    void* m_poolsHead = 0;
    void* m_pagesHead = 0;
    ManagedPage* m_allocatedPageIt = 0;
    void* padding0;

    ManagedPool* m_tinyObjectPools[TINY_OBJECT_POOLS_COUNT] = {};
    ManagedPool* m_smallObjectPools[SMALL_OBJECT_POOLS_COUNT] = {};
    ManagedPool* m_mediumObjectPools[MEDIUM_OBJECT_POOLS_COUNT] = {};
    ManagedPage* m_pages[LARGE_OBJECT_PAGES_ROWS][LARGE_OBJECT_PAGES_COLUMNS] = {};


    // each thread has it own context
    ThreadContext m_contexts[MAX_THREADS] = {};


    //==================================================================================
    // tiny object
    spinlock m_tinyObjectPerformGCLock;
    union
    {
        size_t m_tinyObjectsTotalAllocatedBytes = 0;
        std::atomic<size_t> m_tinyObjectsTotalAllocatedBytes_;
    };

    size_t m_tinyObjectNextAllocatedBytesToPerformGC = TINY_OBJECT_GC_START_SIZE;


    //==================================================================================
    // small objects
    spinlock m_smallObjectPerformGCLock;
    union
    {
        size_t m_smallObjectsTotalAllocatedBytes = 0;
        std::atomic<size_t> m_smallObjectsTotalAllocatedBytes_;
    };
    size_t m_smallObjectNextAllocatedBytesToPerformGC = SMALL_OBJECT_GC_START_SIZE;


    //==================================================================================
    // meidum objects
    spinlock m_meidumObjectOutOfMemoryLock;
    // the newest
    size_t m_nextAllocateMediumPoolId[MEDIUM_OBJECT_MAX_PAGES] = {};
    union
    {
        size_t m_totalMediumObjectReservedSize = 0;
        std::atomic<size_t> m_totalMediumObjectReservedSize_;
    };

    union
    {
        size_t m_totalMediumObjectAllocatedBytes = 0;
        std::atomic<size_t> m_totalMediumObjectAllocatedBytes_;
    };
    

    //==================================================================================
    // large objects
    spinlock m_largeObjectOutOfMemoryLock;
    // m_initializedPages[i] handles how many page in row is initialized
    size_t m_initializedPages[LARGE_OBJECT_PAGES_ROWS] = {};
    // plan to initialize next page-size
    size_t m_pageSizePlan[LARGE_OBJECT_PAGES_ROWS] = {};
    size_t m_totalPages = 0;

    
    //==================================================================================
    union
    {
        size_t m_totalHeapSize = 0;
        std::atomic<size_t> m_totalHeapSize_;
    };

    union
    {
        size_t m_totalAllocatedBytes = 0;
        std::atomic<size_t> m_totalAllocatedBytes_;
    };


    // when turn off GC, this heap
    bool m_isGCActivated = true;
    
public:
    ManagedHeap(bool GC = true);
    ~ManagedHeap();

private:
    // return row
    inline size_t ChoosePage(size_t nBytes)
    {
        nBytes += EXTERNAL_SIZE;
        return ((nBytes / LARGE_OBJECT_PAGES_START_SIZE) + (nBytes % LARGE_OBJECT_PAGES_START_SIZE == 0 ? 0 : 1)) - 1;
    }

    void PerformGC(ThreadContext* ctx, spinlock& lock);
    //void OnSmallObjectPerformGC(ThreadContext* ctx, size_t nBytes);
    // tie memory of large objects and medium objects, small objects will be free to use memory of OS
    void MediumObjectReserveFor(size_t nBytes);
    void OnMediumObjectOutOfMemory(ThreadContext* ctx, size_t nBytes);

    void LargeObjectInitPage(size_t row);
    void OnLargeObjectOutOfMemory(ThreadContext* ctx, size_t nBytes);

    // return id
    size_t ChooseAndLockTinyObjectPool(ThreadContext* ctx, size_t nBytes);
    size_t ChooseAndLockSmallObjectPool(ThreadContext* ctx, size_t nBytes);
    size_t ChooseAndLockMediumObjectPool(ThreadContext* ctx, size_t nBytes);
    size_t ChooseAndLockLargeObjectPage(ThreadContext* ctx, size_t nBytes);

public:
    ManagedHandle* Allocate(size_t nBytes, TraceTable* table, byte** managedLocalBlock);
    void Deallocate(ManagedHandle* handle);

    void FreeStableObjects(byte stableValue, void* userPtr, void(*callback)(void*, ManagedHeap*, ManagedHandle*));

};


NAMESPACE_MEMORY_END