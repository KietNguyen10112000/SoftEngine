#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Structures/Managed/UnorderedList.h"

NAMESPACE_BEGIN

// all public methods are thread safe
class GenericStorage : public Traceable<GenericStorage>
{
private:
	UnorderedLinkedList<Handle<void>> m_storage;
	spinlock m_lock;
	bool padd[3];

	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_storage);
	}

public:
	template <typename T>
	inline ID Store(const Handle<T>& handle)
	{
		// multiple inheritance is not allowed
		if constexpr (std::is_polymorphic_v<T>)
		{
			assert(dynamic_cast<void*>(handle.Get()) == (void*)handle.Get());
		}

		m_lock.lock();
		auto ret = m_storage.Add(handle);
		m_lock.unlock();
		return ret;
	}

	template <typename T>
	inline const Handle<T> Get(ID id)
	{
		// this is why multiple inheritance is not allowed
		return StaticCast<T>(m_storage.Get(id));
	}

	template <typename T>
	inline const Handle<T> Access(ID id)
	{
		// this is why multiple inheritance is not allowed
		return StaticCast<T>(m_storage.Get(id));
	}

	inline void Remove(ID id)
	{
		m_lock.lock();
		m_storage.Remove(id);
		m_lock.unlock();
	}

};

NAMESPACE_END