#pragma once

#include "Core/Thread/Spinlock.h"

#include "TypeDef.h"
#include "Page.h"
#include "ManagedPointers.h"
#include "Pool.h"
#include "Trace.h"
#include "ManagedHandle.h"

NAMESPACE_MEMORY_BEGIN


class ManagedPage : public Page
{
protected:
	friend class Heap;

	using Handle = ManagedHandle;

	byte m_id = 0;

public:
	spinlock m_lock;
	AllocatedBlock* m_sweepIt = (AllocatedBlock*)(-1);
	AllocatedBlock* m_sweepBackwardIt = (AllocatedBlock*)(-1);

	void* m_DeallacateCallbackArg = 0;
	void (*m_DeallacateCallback)(void*, Handle*) = 0;

	bool m_isInitialized = false;

public:
	ManagedPage() : Page(true)
	{

	}

	ManagedPage(byte id, size_t size = DEFAULT_SIZE) : Page(size), m_id(id)
	{
		m_isInitialized = true;
		SetupNewGCCycle();
	}

	~ManagedPage()
	{
		if (m_totalAllocatedBytes != 0)
		{
			//std::cout << "Total memory leak: " << m_totalAllocatedBytes << " bytes\n";
			size_t remainDeleteCalls = 0;
			CONSOLE_WARN() << "Memory leak detected\n";
			Page::ForEachAllocatedBlocks(
				[&](void* buffer, size_t bufferSize) 
				{
					//CONSOLE_WARN() << "[" << buffer << "]: " << bufferSize - sizeof(Handle) << " bytes\n";
					remainDeleteCalls++;
				}
			);
			CONSOLE_WARN() << "Missing: " << remainDeleteCalls << " delete calls\n";
		}
		else
		{
			assert(m_totalFreeBlocks == 1);
		}
	}

public:
	inline ManagedHandle* Allocate(size_t n)
	{
		assert(m_isInitialized == true);
		/**
		* ------------------------------------------------------------------------
		* | AllocatedBlock (16 bytes) | Handle (16 bytes) | usable mem (n bytes)
		* ------------------------------------------------------------------------
		**/
		auto handle = (Handle*)Page::Allocate(n + sizeof(Handle));

		AllocatedBlock* block = (AllocatedBlock*)(handle - 1);

		if (!handle) return 0;

		handle->paddingBytes = block->TotalSize() - sizeof(AllocatedBlock*) - sizeof(Handle) - n;
		
		// importance
		if ((byte*)handle < (byte*)m_sweepIt)
		{
			handle->marked = 0;
		}
		else
		{
			handle->marked = 1;
		}


		handle->pageId = m_id;
		handle->poolId = -1;

		return handle;
	}

public:
	inline void SetDeallocateCallback(void* ptr, void(*callback)(void*, ManagedHandle*))
	{
		m_DeallacateCallbackArg = ptr;
		m_DeallacateCallback = callback;
	}

	inline void Deallocate(void* p)
	{
		Handle* handle = ((Handle*)p) - 1;

		if (m_DeallacateCallback)
		{
			m_DeallacateCallback(m_DeallacateCallbackArg, handle);
		}

		Page::Free(handle);
	}

	// iterate over allocated blocks
	// callback = function(Handle* handle, ...)
	// return last block
	template <typename C, typename... Args>
	byte* ForEachAllocatedBlocks(C callback, Args&&... args)
	{
		return Page::ForEachAllocatedBlocks(
			[&](byte* buffer, size_t bufferSize)
			{
				callback((Handle*)buffer, std::forward<Args>(args)...);
			}
		);
	}

public:
	inline bool IsEnoughMemory(size_t size)
	{
		return m_maxSizeFreeBlock && m_maxSizeFreeBlock->TotalSize() >= size + sizeof(AllocatedBlock) + sizeof(Handle);
	}

	inline void SetupNewGCCycle()
	{
		m_sweepIt = (AllocatedBlock*)Page::m_buffer;
	}

	inline byte* Begin()
	{
		return m_buffer;
	}

	inline byte* End()
	{
		return m_buffer + m_size;
	}

//public:
//	template<typename T, typename... Args>
//	inline Ptr<T> NewOperatorImpl(size_t count, Args&&... args)
//	{
//		Ptr<T> ret;
//
//		Handle* handle = 0;
//
//		auto p = ManagedPage::Allocate(sizeof(T) * count, handle);
//		if (!p) return ret;
//		
//		ret.m_pptr = p;
//		ret.m_offset = 0;
//
//		T* obj = *((T**)ret.m_pptr);
//
//		if constexpr (std::is_base_of_v<Traceable<T>, T>)
//		{
//			static_assert(Has_Trace<T>::value, "Traceable must provides trace method \"void Trace(Tracer* tracer);\"");
//			handle->traceTableIdx = Traceable<T>::GetTraceTableIdx();
//		}
//		else
//		{
//			handle->traceTableIdx = -1;
//		}
//
//		for (size_t i = 0; i < count; i++)
//		{
//			new (obj) T(std::forward<Args>(args)...);
//			obj++;
//		}
//
//#ifdef _DEBUG
//		ret.m_view = ret.Get();
//#endif // _DEBUG
//
//		return ret;
//	}
//
//	template <typename T, typename... Args>
//	inline Ptr<T> New(Args&&... args)
//	{
//		return NewOperatorImpl<T>(1, std::forward<Args>(args)...);
//	}
//
//	template <typename T, typename... Args>
//	inline Ptr<T> NewArray(size_t count, Args&&... args)
//	{
//		return NewOperatorImpl<T>(count, std::forward<Args>(args)...);
//	}
//
//	// allow polymorphism
//	template <typename T>
//	inline void Delete(Ptr<T>& ptr)
//	{
//		if (ptr.IsNull()) return;
//
//		T* obj = ptr.Get();
//		obj->~T();
//
//		ManagedPage::Deallocate(*ptr.m_pptr);
//	}
//
//	// not allow polymorphism
//	template <typename T>
//	inline void DeleteArray(Ptr<T>& ptr)
//	{
//		if (ptr.IsNull()) return;
//
//		AllocatedBlock* allocatedBlock = *(ptr.m_pptr) - sizeof(AllocatedBlock) - sizeof(Handle);
//		auto objSize = allocatedBlock->TotalSize() - sizeof(AllocatedBlock) - sizeof(Handle);
//
//		T* obj = ptr.Get();
//
//		size_t count = objSize / sizeof(T);
//
//		for (size_t i = 0; i < count; i++)
//		{
//			obj->~T();
//			obj++;
//		}
//
//		ManagedPage::Deallocate(*ptr.m_pptr);
//	}
//
//public:
//	// without invoking constructor
//	template <typename T>
//	[[nodiscard]] inline Local<T> Allocate(size_t count)
//	{
//		Local<T> ret = nullptr;
//
//		m_lock.lock();
//
//		ManagedLocalScope::s.lockedPage = this;
//
//
//		Handle* handle = ManagedPage::Allocate(sizeof(T) * count);
//
//		if (handle == 0) return ret;
//
//		handle->traceTable = TraceTable::Get<T>();
//
//		ret.m_block = handle->GetUsableMemAddress();
//		ret.m_ptr = (T*)(handle->GetUsableMemAddress());
//
//		ManagedLocalScope::Transaction(&ret.m_block);
//
////#ifdef _DEBUG
//		memset(handle->GetUsableMemAddress(), 0, sizeof(T) * count);
////#endif // _DEBUG
//
//		//ret = handle;
//
//		ManagedLocalScope::s.lockedPage = 0;
//
//		m_lock.unlock();
//
//		return ret;
//	}

	/*template <typename T>
	inline void Free(Ptr<T>& ptr)
	{
		if (ptr.IsNull()) return;
		ManagedPage::Deallocate(*ptr.m_pptr);
	}*/

};


NAMESPACE_MEMORY_END