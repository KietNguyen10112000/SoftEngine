#pragma once

#include "Core/TypeDef.h"

#include "Core/Memory/Memory.h"

NAMESPACE_BEGIN

class GameObject;

class SubSystemComponent
{
public:
	// called when this component is added to object
	virtual void OnComponentAdded(GameObject* object) = 0;

	// called when object contains this component added to scene
	virtual void OnComponentAddedToScene(const Handle<GameObject>& object) = 0;

};

NAMESPACE_END