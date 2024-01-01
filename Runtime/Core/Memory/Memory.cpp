#include "Memory.h"

#include "ManagedHeap.h"
#include "NewMalloc.h"

NAMESPACE_MEMORY_BEGIN

ManagedHeap* g_rawHeap = 0;
ManagedHeap* g_stableHeap = 0;
ManagedHeap* g_gcHeap = 0;

thread_local size_t g_stableValue = 0;

constexpr size_t NEW_DELETE_OPERATOR_SIGN_BEGIN = 88888887;
constexpr size_t NEW_DELETE_OPERATOR_SIGN_END = 0x00000011;

constexpr size_t MALLOC_SIGN_BEGIN = 88888889;
constexpr size_t MALLOC_SIGN_END = 88888899;

void MemoryInitialize()
{
    if (g_rawHeap) return;

    g_rawHeap = NewMalloc<ManagedHeap>(false);
    g_stableHeap = NewMalloc<ManagedHeap>(false);
    g_gcHeap = NewMalloc<ManagedHeap>();
}

void MemoryFinalize()
{
    if (g_rawHeap)
    {
        std::cout << "MemoryFinalize Raw Heap\n";
        DeleteMalloc(g_rawHeap);
        g_rawHeap = 0;
    }

    //std::cout << "end g_rawHeap\n";

    if (g_stableHeap)
    {
        std::cout << "MemoryFinalize Stable Heap\n";
        DeleteMalloc(g_stableHeap);
        g_stableHeap = 0;
    }

    //std::cout << "end g_stableHeap\n";

    if (g_gcHeap)
    {
        std::cout << "MemoryFinalize GC Heap\n";
        DeleteMalloc(g_gcHeap);
        g_gcHeap = 0;
    }

    //std::cout << "end g_gcHeap\n";
}


//=========================================================================================
ManagedHandle* mheap::internal::Allocate(size_t nBytes, TraceTable* table, byte** managedLocalBlock)
{
    if (g_stableValue == 0) return g_gcHeap->Allocate(nBytes, table, managedLocalBlock, g_stableValue);

    auto handle = g_stableHeap->Allocate(nBytes, table, managedLocalBlock, g_stableValue);
    return handle;
}

void mheap::internal::Deallocate(ManagedHandle* handle)
{
    assert(handle->stableValue != 0);
    g_stableHeap->Deallocate(handle);
}

ManagedHeap* mheap::internal::Get()
{
    return g_gcHeap;
}

ManagedHeap* mheap::internal::GetStableHeap()
{
    return g_stableHeap;
}

byte mheap::internal::GetStableValue()
{
    return (byte)g_stableValue;
}

void mheap::internal::SetStableValue(byte value)
{
    g_stableValue = (size_t)value;
}

void mheap::internal::ChangeStableValue(byte newValue, ManagedHandle* returnedByAllocate)
{
    returnedByAllocate->stableValue = newValue;
}

void mheap::internal::FreeStableObjects(byte stableValue, void* userPtr, void(*callback)(void*, ManagedHeap*, ManagedHandle*))
{
    gc::BlockGC(true);
    g_stableHeap->FreeStableObjects(stableValue, userPtr, callback);
    TRACK_STABLE_VALUE(stableValue);
    gc::ClearTrackedBoundariesOfStableValue(stableValue);
    gc::BlockGC(false);
}

void mheap::internal::Reset()
{
    g_gcHeap->~ManagedHeap();
    new (g_gcHeap) ManagedHeap();

    g_stableHeap->~ManagedHeap();
    new (g_stableHeap) ManagedHeap(false);
}


//=========================================================================================
ManagedHandle* rheap::internal::Allocate(size_t nBytes)
{
    return g_rawHeap->Allocate(nBytes, 0, 0, 0);
}

void rheap::internal::Deallocate(ManagedHandle* handle)
{
    if (g_rawHeap)
        g_rawHeap->Deallocate(handle);
    else
        // case of static initalizer
        std::cout << "[   WARN   ]\tinvalid deallocate call\n";
}

ManagedHeap* rheap::internal::Get()
{
    return g_rawHeap;
}

void rheap::internal::Reset()
{
    g_rawHeap->~ManagedHeap();
    new (g_rawHeap) ManagedHeap(false);
}

#ifdef _DEBUG
void* rheap::malloc(size_t nBytes)
{
    auto handle = soft::g_rawHeap->Allocate(nBytes + 3 * sizeof(size_t), 0, 0, 0);
    auto mem = handle->GetUsableMemAddress();

    size_t* p = (size_t*)mem;
    *p = MALLOC_SIGN_BEGIN;

    p = (size_t*)((byte*)(handle - 1) + handle->TotalSize() - sizeof(size_t));
    *p = MALLOC_SIGN_END;

    auto ret = ((size_t*)mem) + 2;
    return ret;
}

void rheap::free(void* ptr)
{
    size_t* p = ((size_t*)ptr) - 2;
    auto handle = ((ManagedHandle*)p) - 1;

    assert(*p == MALLOC_SIGN_BEGIN);

    p = (size_t*)((byte*)(handle - 1) + handle->TotalSize() - sizeof(size_t));
    assert(*p == MALLOC_SIGN_END);

    soft::g_rawHeap->Deallocate(handle);
}
#endif // _DEBUG

void* rheap::internal::OperatorNew(size_t _Size)
{
    if (!soft::g_rawHeap)
    {
        soft::MemoryInitialize();
    }

#ifdef _DEBUG
    auto handle = soft::g_rawHeap->Allocate(_Size + 2 * sizeof(size_t), 0, 0, 0);
    auto mem = handle->GetUsableMemAddress();
    
    size_t* p = (size_t*)mem;
    *p = NEW_DELETE_OPERATOR_SIGN_BEGIN;

    p = (size_t*)((byte*)(handle - 1) + handle->TotalSize() - sizeof(size_t));
    *p = NEW_DELETE_OPERATOR_SIGN_END;

    auto ret = ((size_t*)mem) + 1;
    return ret;
#else
    return soft::g_rawHeap->Allocate(_Size, 0, 0, 0)->GetUsableMemAddress();
#endif // _DEBUG
}

void rheap::internal::OperatorDelete(void* ptr) noexcept
{
    if (soft::g_rawHeap)
    {
#ifdef _DEBUG
        size_t* p = ((size_t*)ptr) - 1;
        auto handle = ((ManagedHandle*)p) - 1;

        assert(*p == NEW_DELETE_OPERATOR_SIGN_BEGIN);

        p = (size_t*)((byte*)(handle - 1) + handle->TotalSize() - sizeof(size_t));
        assert(*p == NEW_DELETE_OPERATOR_SIGN_END);

        soft::g_rawHeap->Deallocate(handle);
#else
        soft::g_rawHeap->Deallocate((soft::ManagedHandle*)ptr - 1);
#endif // _DEBUG
    }
    else
    {
        // case of static initalizer
        if (ptr) 
            std::cout << "[   WARN   ]\tinvalid delete call\n";
    }
}

NAMESPACE_MEMORY_END