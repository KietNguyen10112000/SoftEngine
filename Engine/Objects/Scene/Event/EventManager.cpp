#include "EventManager.h"

#include "Core/Structures/Raw/ConcurrentList.h"

NAMESPACE_BEGIN

struct ListenerRemoveRecord
{
	EventListenerContainer* container;
	EventListener* listener;
};

raw::ConcurrentList<ListenerRemoveRecord>* g_recoredRemoveListeners = nullptr;

void EventManager::RecordRemove(EventListenerContainer* container, const Handle<EventListener>& listener)
{
	g_recoredRemoveListeners->Add({ container, listener.Get() });
}

void EventManager::FlushRecoredRemoveListeners()
{
	g_recoredRemoveListeners->ForEach(
		[](const ListenerRemoveRecord& record)
		{
			EventListenerContainer* container = record.container;
			EventListener* listener = record.listener;
			container->RemoveImpl(listener);
		}
	);
	g_recoredRemoveListeners->Clear();
}

void EventManager::Initialize()
{
	if (g_recoredRemoveListeners) return;

	g_recoredRemoveListeners = rheap::New<std::remove_pointer<decltype(g_recoredRemoveListeners)>::type>();
}

void EventManager::Finalize()
{
	if (g_recoredRemoveListeners == 0) return;

	rheap::Delete(g_recoredRemoveListeners);
	g_recoredRemoveListeners = 0;
}

EventManager::EventManager(ID maxEventCode)
{
	m_maxEventCode = maxEventCode;
	m_listeners = mheap::NewArray<ConcurrentListC<EventListenerContainer>>(maxEventCode);
}

EventManager::~EventManager()
{
	for (size_t i = 0; i < m_maxEventCode; i++)
	{
		auto& listeners = m_listeners[i];
		listeners.ForEachContainer([](EventListenerContainer& container)
			{
				auto& _listeners = container.m_listeners;
				_listeners.ForEach([](Handle<EventListener>& listener) 
					{
						listener->Reset();
					}
				);
			}
		);
	}
}

NAMESPACE_END