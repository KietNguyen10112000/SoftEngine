#pragma once

#include "Core/TypeDef.h"

#include "Core/Structures/STD/STDContainers.h"
#include "Core/Structures/Raw/UnorderedList.h"

NAMESPACE_BEGIN

class EventDispatcher
{
public:
	using Callback = void(*)(int argc, void** argv, void* userPtr);

private:
	struct Listener
	{
		Callback callback;
		void* userPtr;
	};

	std::Vector<raw::UnorderedList<Listener>> m_listeners;

public:
	EventDispatcher(size_t numEvent)
	{
		m_listeners.resize(numEvent);
	}

public:
	inline ID AddListener(ID evtCode, Callback cb, void* userPtr)
	{
		return m_listeners[evtCode].Add({ cb, userPtr });
	}

	inline void RemoveListener(ID evtCode, ID id)
	{
		m_listeners[evtCode].Remove(id);
	}

	inline void Dispatch(ID evtCode, int argc, void** argv)
	{
		m_listeners[evtCode].ForEach(
			[=](Listener& listener)
			{
				listener.callback(argc, argv, listener.userPtr);
			}
		);
	}

	template <typename ...Args>
	inline void Dispatch(ID evtCode, Args&&... args)
	{
		void* pointers[sizeof...(Args)] = { (void*)args...};
		Dispatch(evtCode, (int)(sizeof...(Args)), (void**)pointers);
	}

};

NAMESPACE_END