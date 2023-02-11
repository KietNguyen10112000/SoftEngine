#pragma once
#include "Core/TypeDef.h"

#include "Core/Structures/Raw/ConcurrentList.h"

#include "BuiltinEvent.h"

NAMESPACE_BEGIN

class BuiltinEventManager;
using EventCallback = void(*)(BuiltinEventManager*, ID, ID, void*);
using EventCallbackParam = void*;

class EventListener;
class GameObject;
class Scene;

// the event manager used by sub-systems
class BuiltinEventManager
{
protected:
	constexpr static size_t NUM_SPACE = 4;
	constexpr static size_t NUM_OBJECT_SPACE = 16;

	struct Callback
	{
		EventCallback func;
		EventCallbackParam param;
	};

	struct TaskParam
	{
		BuiltinEventManager* mgr;
		size_t index;
	};

	raw::ConcurrentList<EventListener*, NUM_SPACE> 
		m_listeners[BUILTIN_EVENT_SUIT::COUNT][BUILTIN_EVENT::COUNT] = {};

	raw::ConcurrentList<Callback, NUM_SPACE>
		m_callbacks[BUILTIN_EVENT_SUIT::COUNT][BUILTIN_EVENT::COUNT] = {};

	raw::ConcurrentList<GameObject*, NUM_OBJECT_SPACE>
		m_objects[BUILTIN_EVENT_SUIT::COUNT][BUILTIN_EVENT::COUNT] = {};


	TaskParam m_taskParams[BUILTIN_EVENT_SUIT::COUNT] = {};

	Scene* m_scene;

public:
	inline BuiltinEventManager(Scene* scene) : m_scene(scene) {};

public:
	void FlushAllObjectEvents();

public:
	// thread-safe methods
	inline void AddEventCallback(BUILTIN_EVENT::Enum event, BUILTIN_EVENT_SUIT::Enum suit,
		EventCallback callback, EventCallbackParam param)
	{
		m_callbacks[suit][event].Add({ callback, param });
	}

	inline void AddEventListener(BUILTIN_EVENT::Enum event, BUILTIN_EVENT_SUIT::Enum suit,
		EventListener* listener)
	{
		m_listeners[suit][event].Add(listener);
	}

	inline void DispatchObjectEvent(BUILTIN_EVENT::Enum event, BUILTIN_EVENT_SUIT::Enum suit,
		GameObject* object)
	{
		m_objects[suit][event].Add(object);
	}

};

NAMESPACE_END