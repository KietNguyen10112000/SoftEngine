#pragma once
#include "Core/TypeDef.h"

#include "EventListenerContainer.h"

#include "Core/Structures/Managed/ConcurrentList.h"

NAMESPACE_BEGIN

class EventListener;

// user-defined event manager
class EventManager : Traceable<EventManager>
{
private:
	friend class EventListenerContainer;
	friend class Engine;

	Ptr<ConcurrentListC<EventListenerContainer>> m_listeners;
	size_t m_maxEventCode;

private:
	static void RecordRemove(EventListenerContainer* container, const Handle<EventListener>& listener);
	static void FlushRecoredRemoveListeners();
	static void Initialize();
	static void Finalize();

private:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_listeners);
	}

public:
	// maxEventCode <= 64000
	EventManager(ID maxEventCode);
	~EventManager();

public:
	// thead-safe methods
	inline void AddEventListener(ID eventCode, const Handle<EventListener>& listener)
	{
		listener->m_code = eventCode;
		m_listeners[eventCode].Add(listener);
	}

	inline void RemoveEventListener(const Handle<EventListener>& listener)
	{
		listener->Destroy();
	}

};

NAMESPACE_END