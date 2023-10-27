#pragma once

#include "Core/TypeDef.h"

#include "Core/Structures/STD/STDContainers.h"
#include "Core/Structures/Raw/UnorderedList.h"

NAMESPACE_BEGIN

template <typename T, size_t NUM_EVENT, typename _EventCodeEnumType, typename _UserValueType>
class EventDispatcher
{
public:
	using Callback = void(*)(T* target, int argc, void** argv, _UserValueType userValue);

private:
	friend typename T;

	struct Listener
	{
		Callback callback;
		_UserValueType userValue;
	};

	struct ListenerID
	{
		_EventCodeEnumType evtCode;
		ID refId;
	};

	raw::UnorderedList<Listener> m_dispatchers[NUM_EVENT] = {};
	spinlock m_dispatcherLocks[NUM_EVENT] = {};
	raw::UnorderedList<ListenerID> m_listeners;
	spinlock m_lock;
	bool m_padd[3];
	T* m_target;

public:
	EventDispatcher(T* target) : m_target(target)
	{
		//m_dispatchers.resize(numEvent);
		//m_dispatcherLocks.resize(numEvent);
	}

private:
	inline void Dispatch(_EventCodeEnumType evtCode, int argc, void** argv)
	{
		m_dispatcherLocks[listenerId.evtCode].lock();
		m_dispatchers[evtCode].ForEach(
			[=](Listener& listener)
			{
				listener.callback(m_target, argc, argv, listener.userPtr);
			}
		);
	}

	template <typename ...Args>
	inline void Dispatch(_EventCodeEnumType evtCode, Args&&... args)
	{
		void* pointers[sizeof...(Args)] = { (void*)args... };
		Dispatch(evtCode, (int)(sizeof...(Args)), (void**)pointers);
	}

public:
	inline ID AddListener(_EventCodeEnumType evtCode, Callback cb, _UserValueType userValue = {})
	{
		m_lock.lock();
		auto& dispatcher = m_dispatchers[evtCode];
		m_dispatcherLocks[evtCode].lock();
		auto id = dispatcher.Add({ cb, userValue, evtCode });
		m_dispatcherLocks[evtCode].unlock();
		auto ret = m_listeners.Add({ evtCode, id });
		m_lock.unlock();
		return ret;
	}

	// remove what returned by AddListener()
	inline void RemoveListener(ID id)
	{
		m_lock.lock();
		auto& listenerId = m_listeners.Get(id);
		m_dispatcherLocks[listenerId.evtCode].lock();
		m_dispatchers[listenerId.evtCode].Remove(listenerId.refId);
		m_dispatcherLocks[listenerId.evtCode].unlock();
		m_listeners.Remove(id);
		m_lock.unlock();
	}

};

NAMESPACE_END