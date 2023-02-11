#include "EventListener.h"

#include "EventListenerContainer.h"

NAMESPACE_BEGIN

void EventListener::Destroy()
{
	Handle<EventListener> _this;
	_this.Set((byte*)this, this);
	m_container->Remove(_this);
}

NAMESPACE_END