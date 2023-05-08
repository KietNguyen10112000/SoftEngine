#pragma once

#include <cassert>

#include "TypeDef.h"

//#include "Mark.h"
#include "Trace.h"

#include "ManagedHandle.h"
#include "ManagedLocalScope.h"

NAMESPACE_MEMORY_BEGIN


template <typename T>
class ManagedPointer;

namespace gc
{
	template <template <typename> class P, typename T>
	void RegisterRoot(P<T>* arr);
}

namespace mheap
{
	template <typename T, typename ...Args>
	ManagedPointer<T> NewArray(size_t n, Args&&... args);
}

#define MANAGED_PTR_FRIEND()										\
template <															\
	typename Derived,												\
	template<typename>												\
	class PtrType,													\
	typename Base													\
> friend ManagedPointer<Derived> DynamicCast(const PtrType<Base>& ptr);	\
template <															\
	typename Derived,												\
	template<typename>												\
	class PtrType,													\
	typename Base													\
> friend ManagedPointer<Derived> StaticCast(const PtrType<Base>& ptr);		\
template <															\
	typename Derived,												\
	template<typename>												\
	class PtrType,													\
	typename Base													\
> friend ManagedPointer<Derived> ReinterpretCast(const PtrType<Base>& ptr);


// change member of ManagedPointer need to change SIZE_OF_MANAGED_PTR (Memory/TypeDef.h)
template <typename T>
class ManagedPointer : public Traceable<ManagedPointer<T>>
{
protected:
	template<typename _T>
	friend class ManagedPointer;

	friend class ManagedPage;
	friend class ManagedPool;
	friend class ManagedHeap;
	friend class Tracer;
	template <typename T, typename ...Args>
	friend ManagedPointer<T> mheap::NewArray(size_t n, Args&&... args);

	MANAGED_PTR_FRIEND();

	template <template <typename> class P, typename T>
	friend void gc::RegisterRoot(P<T>* arr);

	//using Base::m_block;
	//using Base::m_ptr;

protected:
	// ptr to first object's byte
	byte* m_block = 0;

	// ptr to object byte (polymorphism, multiple inheritance)
	T* m_ptr = 0;

	// offset from "this" to first byte of block contains "this"
	size_t m_offsetToSelf = -1;

public:
	ManagedPointer()
	{
		OnConstructor();
	}

	template <typename Derived>
	ManagedPointer(const ManagedPointer<Derived>& r)
	{
		OnConstructor();
		*this = r;
	}

	ManagedPointer(const ManagedPointer<T>& r)
	{
		OnConstructor();
		*this = r;
	}

	ManagedPointer(nullptr_t)
	{
		OnConstructor();
	}

	ManagedPointer(T* ptr)
	{
		OnConstructor();
		*this = ptr;
	}

	~ManagedPointer()
	{
		//Reset();
		OnDestructor();
	}

protected:
	inline void Reset()
	{
		m_block = 0;
		m_ptr = 0;
	}

	inline void OnConstructor()
	{
		//ManagedLocalScope::Transaction(&m_block);
		ManagedLocalScope::Push(&m_block);
	}

	inline void OnDestructor()
	{
		//Transaction(&m_block);
		ManagedLocalScope::Pop(&m_block);
		Reset();
	}

public:
	inline void Set(byte* block, T* ptr)
	{
		if (m_offsetToSelf != -1)
		{
			ManagedHandle* lhsSelfBlock = (ManagedHandle*)((byte*)this - m_offsetToSelf) - 1;
			ManagedHandle* rhsSelfBlock = (ManagedHandle*)((byte*)block) - 1;
			if (lhsSelfBlock->IsNonTrackedStableObject() && !rhsSelfBlock->IsStableObject())
			{
				// cross boundary
				ManagedLocalScope::TrackCrossBoundary(lhsSelfBlock->GetUsableMemAddress());
				lhsSelfBlock->MakeStableObjectTracked();
			}
		}

		ManagedLocalScope::Transaction(&m_block, block);

		//m_block = block;
		m_ptr = ptr;
	}

	template <typename Derived>
	inline void operator=(const ManagedPointer<Derived>& r);
	inline void operator=(const ManagedPointer<T>& r);
	inline void operator=(ManagedHandle* handle);

	/*using Base::operator->;
	using Base::operator[];
	using Base::operator=;
	using Base::Get;
	using Base::Field;
	using Base::Base;*/

protected:
	ManagedHandle* GetHandle() const
	{
		return (ManagedHandle*)(Get()) - 1;
	}

	friend class Tracer;
	friend class TraceTable;
	void Trace(Tracer* tracer)
	{
		tracer->TraceManagedPtr(*this);
	}

public:

	inline T* Get() const
	{
		return m_ptr;
	}

	/*inline void Set(byte* block, T* ptr)
	{
		ManagedLocalScope::Transaction(&m_block, (byte*)ptr);
		m_block = block;
		m_ptr = ptr;
	}*/

	// to save a pointer to a field of this pointer
	template <auto f>
	inline auto Field() const
	{
		assert(!IsNull());

		using P = decltype(f);

		using R = typename member_pointer_value<P>::type;

		auto handle = GetHandle();

		ManagedPointer<R> ret;
		ret.Set(handle->GetUsableMemAddress(), &(Get()->*f));

		//ret.m_block = handle->GetUsableMemAddress();
		//ret.m_ptr = &(Get()->*f);
		return ret;
	}

	inline bool IsNull() const
	{
		return m_ptr == 0;
	}

	inline T* operator->() const
	{
		return Get();
	}

	template <class _Ty2 = T, std::enable_if_t<!std::disjunction_v<std::is_array<_Ty2>, std::is_void<_Ty2>>, int> = 0>
	inline _Ty2& operator*() const
	{
		return *Get();
	}

	template <class _Ty2 = T, std::enable_if_t<!std::disjunction_v<std::is_array<_Ty2>, std::is_void<_Ty2>>, int> = 0>
	_Ty2& operator[](size_t i) const
	{
		return *(Get() + i);
	}

	/*const T& operator[](size_t i) const
	{
		return *(Get() + i);
	}*/

	inline void operator=(nullptr_t)
	{
		Reset();
	}

	inline void operator=(T* ptr)
	{
		if constexpr (std::is_final_v<T>)
		{
			//assert(dynamic_cast<void*>(ptr) == ptr);
			Set((byte*)ptr, ptr);
		}
		else
		{
			void* p = dynamic_cast<void*>(ptr);
			Set((byte*)p, ptr);
		}
	}

	inline operator T*()
	{
		return Get();
	}

	inline operator const T* () const
	{
		return Get();
	}

	/*operator ManagedPointer<byte>() const
	{
		ManagedPointer<byte> ret;
		ret.Set(m_block, (byte*)m_ptr);
		return ret;
	}*/

};

template <typename T>
using Local = ManagedPointer<T>;

template <typename T>
using LocalPtr = ManagedPointer<T>;

template<typename T>
template<typename Derived>
inline void ManagedPointer<T>::operator=(const ManagedPointer<Derived>& r)
{
	static_assert((std::is_base_of_v<T, Derived> || std::is_void<T>::value) && "");

	if (r.IsNull())
	{
		Reset();
		return;
	}

	//ManagedLocalScope::Transaction(&m_block, r.m_block);
	//m_block = r.m_block;
	//m_ptr = r.m_ptr;
	Set(r.m_block, r.m_ptr);
}

template<typename T>
inline void ManagedPointer<T>::operator=(const ManagedPointer<T>& r)
{
	if (r.IsNull())
	{
		Reset();
		return;
	}

	//ManagedLocalScope::Transaction(&m_block, r.m_block);
	//m_block = r.m_block;
	//m_ptr = r.m_ptr;
	Set(r.m_block, r.m_ptr);
}

template<typename T>
inline void ManagedPointer<T>::operator=(ManagedHandle* handle)
{
	assert(handle != 0);
	auto p = handle->GetUsableMemAddress();
	//ManagedLocalScope::Transaction(&m_block, p);
	//m_block = handle->GetUsableMemAddress();
	//m_ptr = (T*)handle->GetUsableMemAddress();
	Set(p, p);
}


template <
	typename Derived,
	template<typename>
	class PtrType,
	typename Base
>
inline ManagedPointer<Derived> DynamicCast(const PtrType<Base>& ptr)
{
	static_assert(std::is_base_of_v<ManagedPointer<Base>, PtrType<Base>>, "Type Check!");

	if (ptr.IsNull()) return nullptr;

	ManagedPointer<Derived> ret;
	Base* base = ptr.Get();
	auto derived = dynamic_cast<Derived*>(base);

	if (derived)
	{
		//ManagedLocalScope::Transaction(&ret.m_block, ptr.m_block);
		//ret.m_block = ptr.m_block;
		//ret.m_ptr = derived;
		ret.Set(ptr.m_block, derived);
	}

	return ret;
}


template <
	typename Derived,
	template<typename>
	class PtrType,
	typename Base
>
inline ManagedPointer<Derived> StaticCast(const PtrType<Base>& ptr)
{
	static_assert(std::is_base_of_v<ManagedPointer<Base>, PtrType<Base>>, "Type Check!");

	if (ptr.IsNull()) return nullptr;

	ManagedPointer<Derived> ret;
	Base* base = ptr.Get();
	auto derived = static_cast<Derived*>(base);

	if (derived)
	{
		//ManagedLocalScope::Transaction(&ret.m_block, ptr.m_block);
		//ret.m_block = ptr.m_block;
		//ret.m_ptr = derived;
		ret.Set(ptr.m_block, derived);
	}

	return ret;

}


template <
	typename Derived,
	template<typename>
	class PtrType,
	typename Base
>
inline ManagedPointer<Derived> ReinterpretCast(const PtrType<Base>& ptr)
{
	static_assert(std::is_base_of_v<ManagedPointer<Base>, PtrType<Base>>, "Type Check!");

	if (ptr.IsNull()) return nullptr;

	ManagedPointer<Derived> ret;
	Base* base = ptr.Get();
	auto derived = reinterpret_cast<Derived*>(base);

	if (derived)
	{
		//ManagedLocalScope::Transaction(&ret.m_block, ptr.m_block);
		//ret.m_block = ptr.m_block;
		//ret.m_ptr = derived;
		ret.Set(ptr.m_block, derived);
	}

	return ret;
}

//#undef MANAGED_PTR_FRIEND_FUNCTION_DECL

template <typename T>
using Ptr = ManagedPointer<T>;

template <typename T>
using Pointer = ManagedPointer<T>;

template <typename T>
using Handle = ManagedPointer<T>;

template <typename T>
using Member = ManagedPointer<T>;

template <typename T>
using Field = ManagedPointer<T>;

NAMESPACE_MEMORY_END