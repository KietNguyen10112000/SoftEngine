#pragma once
#include "Core/TypeDef.h"

#include "Core/Thread/Spinlock.h"

NAMESPACE_BEGIN

class Scene;

class EventListener
{
private:
	friend class Scene;
	friend class GameObject;
	friend class EventListenerContainer;
	friend class EventManager;

	ID m_id = 0;
	union
	{
		ID m_type = 0;
		ID m_code;
	};
	EventListenerContainer* m_container = 0;

private:
	inline void Reset()
	{
		m_id = 0;
		m_container = 0;
		m_type = -1;
	}

public:
	virtual ~EventListener() = default;

public:
	virtual void OnInvoke(Scene* scene) = 0;

public:
	void Destroy();

};

NAMESPACE_END