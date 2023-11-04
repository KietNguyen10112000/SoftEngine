#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Structures/Managed/UnorderedList.h"
#include "Core/Structures/String.h"

#include <map>

NAMESPACE_BEGIN

// all public methods are thread safe
class GenericDictionary
{
private:
	UnorderedLinkedList<Handle<void>> m_storage;
	spinlock m_lock;
	bool padd[3];

	// hold id to m_storage
	std::map<String, ID> m_dict;

	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_storage);
	}

public:
	template <typename T>
	inline void Store(String name, const Handle<T>& handle)
	{
		// multiple inheritance is not allowed
		if constexpr (std::is_polymorphic_v<T>)
		{
			assert(dynamic_cast<void*>(handle.Get()) == (void*)handle.Get());
		}

		m_lock.lock();

#ifdef _DEBUG
		auto it = m_dict.find(name);
		assert(it == m_dict.end());
#endif // _DEBUG

		auto id = m_storage.Add(handle);
		m_dict.insert({ name, id });
		m_lock.unlock();
	}

	template <typename T>
	inline const Handle<T> Get(String name)
	{
		m_lock.lock();
		
		auto it = m_dict.find(name);
		if (it == m_dict.end())
		{
			m_lock.unlock();
			return nullptr;
		}

		// this is why multiple inheritance is not allowed
		auto ret = StaticCast<T>(m_storage.Get(it->second));

		m_lock.unlock();
		return ret;
	}

	inline bool Has(String name)
	{
		m_lock.lock();
		auto ret = (m_dict.find(name) != m_dict.end());
		m_lock.unlock();
		return ret;
	}

	inline void Remove(String name)
	{
		m_lock.lock();

		auto it = m_dict.find(name);
		if (it == m_dict.end())
		{
			m_lock.unlock();
			return;
		}

		m_storage.Remove(it->second);
		m_lock.unlock();
	}

	inline void Clear()
	{
		m_lock.lock();
		m_storage.Clear();
		m_dict.clear();
		m_lock.unlock();
	}

};

NAMESPACE_END