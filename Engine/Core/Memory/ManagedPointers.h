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

	MANAGED_PTR_FRIEND();

	template <template <typename> class P, typename T>
	friend void gc::RegisterRoot(P<T>* arr);

	//using Base::m_block;
	//using Base::m_ptr;

protected:
	byte* m_block = 0;
	T* m_ptr = 0;

public:
	ManagedPointer() 
	{
		OnConstructor();
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
	void Trace(Tracer* tracer)
	{
		tracer->TraceManagedPtr(*this);
	}

public:

	inline T* Get() const
	{
		return m_ptr;
	}

	inline void Set(byte* block, T* ptr)
	{
		ManagedLocalScope::Transaction(&m_block, (byte*)ptr);
		m_block = block;
		m_ptr = ptr;
	}

	// to save a pointer to a field of this pointer
	template <auto f>
	inline auto Field() const
	{
		assert(!IsNull());

		using P = decltype(f);

		using R = typename member_pointer_value<P>::type;

		ManagedPointer<R> ret;

		auto handle = GetHandle();
		ret.m_block = handle->GetUsableMemAddress();
		ret.m_ptr = &(Get()->*f);
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

	inline T& operator*() const
	{
		return *Get();
	}

	T& operator[](size_t i)
	{
		return *(Get() + i);
	}

	const T& operator[](size_t i) const
	{
		return *(Get() + i);
	}

	inline void operator=(nullptr_t)
	{
		Reset();
	}

};

template <typename T>
using Local = ManagedPointer<T>;

template <typename T>
using LocalPtr = ManagedPointer<T>;

template<typename T>
inline void ManagedPointer<T>::operator=(const ManagedPointer<T>& r)
{
	if (r.IsNull())
	{
		Reset();
		return;
	}

	ManagedLocalScope::Transaction(&m_block, r.m_block);
	m_block = r.m_block;
	m_ptr = r.m_ptr;
}

template<typename T>
inline void ManagedPointer<T>::operator=(ManagedHandle* handle)
{
	assert(handle != 0);
	ManagedLocalScope::Transaction(&m_block, handle->GetUsableMemAddress());
	m_block = handle->GetUsableMemAddress();
	m_ptr = (T*)handle->GetUsableMemAddress();
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
		ManagedLocalScope::Transaction(&ret.m_block, ptr.m_block);
		ret.m_block = ptr.m_block;
		ret.m_ptr = derived;
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
		ManagedLocalScope::Transaction(&ret.m_block, ptr.m_block);
		ret.m_block = ptr.m_block;
		ret.m_ptr = derived;
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
		ManagedLocalScope::Transaction(&ret.m_block, ptr.m_block);
		ret.m_block = ptr.m_block;
		ret.m_ptr = derived;
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