#include "Memory.h"

#include "ManagedHeap.h"
#include "NewMalloc.h"

NAMESPACE_MEMORY_BEGIN

ManagedHeap* g_rawHeap = 0;
ManagedHeap* g_stableHeap = 0;
ManagedHeap* g_gcHeap = 0;

thread_local size_t g_stableValue = 0;

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
        DeleteMalloc(g_rawHeap);
        g_rawHeap = 0;
    }

    if (g_stableHeap)
    {
        DeleteMalloc(g_stableHeap);
        g_stableHeap = 0;
    }

    if (g_gcHeap)
    {
        DeleteMalloc(g_gcHeap);
        g_gcHeap = 0;
    }
}


//=========================================================================================
ManagedHandle* mheap::internal::Allocate(size_t nBytes, TraceTable* table, byte** managedLocalBlock)
{
    if (g_stableValue == 0) return g_gcHeap->Allocate(nBytes, table, managedLocalBlock);

    auto handle = g_stableHeap->Allocate(nBytes, table, managedLocalBlock);
    handle->stableValue = g_stableValue;
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

byte mheap::internal::GetStableValue()
{
    return (byte)g_stableValue;
}

void mheap::internal::SetStableValue(byte value)
{
    g_stableValue = (size_t)value;
}

void mheap::internal::FreeStableObjects(byte stableValue, void* userPtr, void(*callback)(void*, ManagedHeap*, ManagedHandle*))
{
    g_stableHeap->FreeStableObjects(stableValue, userPtr, callback);
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
    return g_rawHeap->Allocate(nBytes, 0, 0);
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

NAMESPACE_MEMORY_END


int MemoryInitForNewOperator()
{
    soft::MemoryInitialize();
    return 0;
}

void* operator new(size_t _Size)
{
    static int _unuse = MemoryInitForNewOperator();
    return soft::g_rawHeap->Allocate(_Size, 0, 0)->GetUsableMemAddress();
}

void operator delete(void* ptr) noexcept
{
    if (soft::g_rawHeap)
        soft::g_rawHeap->Deallocate((soft::ManagedHandle*)ptr - 1);
    else
        // case of static initalizer
        std::cout << "[   WARN   ]\tinvalid delete call\n";
}