#pragma once
#include "Core/TypeDef.h"

#include "Core/Memory/Memory.h"
#include "Core/Thread/Spinlock.h"
#include "Core/Structures/Structures.h"

#include "EventListener.h"

NAMESPACE_BEGIN

class EventListener;

class EventListenerContainer : Traceable<EventListenerContainer>
{
private:
	friend class EventManager;
	friend class GameObject;

	UnorderedList<Handle<EventListener>> m_listeners;

	// ensure 8 bytes aligned
	union
	{
		spinlock m_lock = {};
		size_t align;
	};
private:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_listeners);
	}

private:
	inline void RemoveImpl(EventListener* listener)
	{
		m_listeners.Remove(listener->m_id);
		listener->Reset();
	}

	inline void AddImpl(ID type, EventListener* listener)
	{
		listener->m_type = type;
		listener->m_container = this;
		listener->m_id = m_listeners.Add(listener);
	}

public:
	//inline EventListenerContainer() {};

	inline void Add(ID type, const Handle<EventListener>& listener)
	{
		m_lock.lock();
		AddImpl(type, listener.Get());
		m_lock.unlock();
	}

	inline void Add(const Handle<EventListener>& listener)
	{
		listener->m_container = this;
		listener->m_id = m_listeners.Add(listener);
	}

	void Remove(const Handle<EventListener>& listener);

	inline bool try_lock()
	{
		return m_lock.try_lock();
	}

	inline void lock()
	{
		m_lock.lock();
	}

	inline void unlock()
	{
		m_lock.unlock();
	}


};

NAMESPACE_END