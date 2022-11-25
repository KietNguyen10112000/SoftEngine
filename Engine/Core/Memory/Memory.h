#pragma once

#include "TypeDef.h"
#include "ManagedHeap.h"
#include "ManagedPointers.h"

//#define MEMORY_RHEAP_ALLOW_MULTIPLE_INHERITANCE 1

NAMESPACE_MEMORY_BEGIN

API void MemoryInitialize();
API void MemoryFinalize();

// managed heap
namespace mheap
{
	namespace internal
	{
		API ManagedHandle* Allocate(size_t nBytes, TraceTable* table, byte** managedLocalBlock);

		// only stable object can be deallocate
		API void Deallocate(ManagedHandle* handle);

		API ManagedHeap* Get();

		API byte GetStableValue();
		API void SetStableValue(byte value);

		API void FreeStableObjects(byte stableValue, void* userPtr, void(*callback)(void*, ManagedHeap*, ManagedHandle*));
	}

	// without calling ctor
	template <typename T>
	Local<T> Allocate(size_t n)
	{
		Local<T> ret;
		byte** p = (byte**)&ret;
		auto handle = internal::Allocate(sizeof(T) * n, TraceTable::Get<T>(), p);
		assert(handle != 0);
		*(p + 1) = *p;
		return ret;
	}

	template <typename T, typename ...Args>
	Local<T> NewArray(size_t n, Args&&... args)
	{
		Local<T> ret = Allocate<T>(n);
		T* objs = ret.Get();

		ManagedLocalScope::AddCheckpoint();

		for (size_t i = 0; i < n; i++)
		{
			new (objs + i) T(std::forward<Args>(args)...);
		}

		ManagedLocalScope::JumpbackToCheckpoint();

		return ret;
	}

	template <typename T, typename ...Args>
	Local<T> New(Args&&... args)
	{
		return NewArray<T>(1, std::forward<Args>(args)...);
	}
}


// underlying component should use raw heap
// raw heap
namespace rheap
{
	namespace internal
	{
		API ManagedHandle* Allocate(size_t nBytes);
		API void Deallocate(ManagedHandle* handle);
		API ManagedHeap* Get();
	}

	inline void* malloc(size_t nBytes)
	{
		return internal::Allocate(nBytes)->GetUsableMemAddress();
	}

	inline void free(void* p)
	{
		internal::Deallocate((ManagedHandle*)p - 1);
	}

	// follow the C standard
	inline void* realloc(void* p, size_t nBytes)
	{
		ManagedHandle* handle = (ManagedHandle*)p - 1;

		if (handle->UsableSize() >= nBytes)
		{
			return p;
		}

		auto newP = rheap::malloc(nBytes);
		::memcpy(newP, p, handle->ObjectSize());
		rheap::free(p);

		return newP;
	}

	template <typename T, typename ...Args>
	T* NewArray(size_t n, Args&&... args)
	{
		auto handle = internal::Allocate(sizeof(T) * n);

		assert(handle != 0);

	#if defined (MEMORY_RHEAP_ALLOW_MULTIPLE_INHERITANCE)
		// not allow trace on this heap
		handle->traceTable = TraceTable::Get<T>();
	#else
		handle->traceTable = 0;
	#endif

		T* objs = (T*)handle->GetUsableMemAddress();

		for (size_t i = 0; i < n; i++)
		{
			new (objs + i) T(std::forward<Args>(args)...);
		}

		return objs;
	}

	template <typename T, typename ...Args>
	T* New(Args&&... args)
	{
		return NewArray<T>(1, std::forward<Args>(args)...);
	}

#if defined (MEMORY_RHEAP_ALLOW_MULTIPLE_INHERITANCE)
	template <typename T>
	void Delete(T* ptr)
	{
		auto p = dynamic_cast<void*>(ptr);
		ManagedHandle* handle = (ManagedHandle*)p - 1;
		TraceTable* table = handle->traceTable;

		auto stride = table->instanceSize;
		auto dtor = table->dtor;

		if (dtor)
		{
			auto it = (byte*)p;
			auto end = handle->End();

			while (it != end)
			{
				dtor(it);
				it += stride;
			}
		}
		
		internal::Deallocate(handle);
	}
#else
	template <typename T>
	void Delete(T* p)
	{
		if constexpr (std::is_polymorphic<T>::value)
		{
			assert(dynamic_cast<void*>(p) == (void*)p);
		}

		ManagedHandle* handle = (ManagedHandle*)p - 1;

		auto it = p;
		auto end = (T*)handle->End();

		while (it != end)
		{
			it->~T();
			it++;
		}

		internal::Deallocate(handle);
	}
#endif

}

NAMESPACE_MEMORY_END


// global new, delete operator
void* operator new(size_t _Size);
void operator delete(void* ptr) noexcept;