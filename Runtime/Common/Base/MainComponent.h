#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

class MainComponent
{
protected:
	friend class GameObject;

	GameObject* m_object = nullptr;

public:
	// called when object contains this component added to scene
	virtual void OnComponentAdded() = 0;

	// called when object contains this component removed from scene
	virtual void OnComponentRemoved() = 0;

	virtual void OnTransformChanged() = 0;

public:
	inline GameObject* GameObject()
	{
		return m_object;
	}

};

NAMESPACE_END