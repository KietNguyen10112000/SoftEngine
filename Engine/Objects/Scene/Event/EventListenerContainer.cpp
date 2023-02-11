#include "EventListenerContainer.h"

#include "EventManager.h"

NAMESPACE_BEGIN


void EventListenerContainer::Remove(const Handle<EventListener>& listener)
{
	if (m_lock.try_lock())
	{
		RemoveImpl(listener.Get());

		m_lock.unlock();
		return;
	}

	EventManager::RecordRemove(this, listener);
}


NAMESPACE_END