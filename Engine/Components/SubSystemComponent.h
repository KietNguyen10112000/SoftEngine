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

	// called when this component is removed from object
	virtual void OnComponentRemoved(GameObject* object) = 0;

	// called when object contains this component added to scene
	virtual void OnComponentAddedToScene(GameObject* object) = 0;

	// called when object contains this component removed from scene
	virtual void OnComponentRemovedFromScene(GameObject* object) = 0;

	// main component is allowed to modify GameObject's properties
	virtual void SetAsMain(GameObject* object) = 0;

	// extra one will read from GameObject's properties and write to its own extra buffer
	virtual void SetAsExtra(GameObject* object) = 0;

	///
	/// resolve conflict of main component and extra component on GameObject's properties
	/// eg: 
	///	+ Script modifies transform
	/// + Physics modifies transform
	/// + Animation modifies transform
	///
	virtual void ResolveConflict(GameObject* object) = 0;
	virtual bool IsConflict() = 0;
};

NAMESPACE_END