#pragma once

#include "Core/TypeDef.h"

#include "Core/Memory/Memory.h"

NAMESPACE_BEGIN

class GameObject;
class SSCDataConflictSolver;

class SubSystemComponent
{
public:
	// called when this component is added to object
	virtual void OnComponentAdded(GameObject* object) = 0;

	// called when object contains this component added to scene
	virtual void OnComponentAddedToScene(GameObject* object) = 0;

	// main component allowed to modify GameObject's properties
	virtual void OnSetAsMain(GameObject* object) = 0;

	// extra one will read from GameObject's properties and write to its own extra buffer
	virtual void OnSetAsExtra(GameObject* object) = 0;

	///
	/// SSCDataConflictSolver use to resolve conflict of main component and extra component on GameObject's properties
	/// eg: 
	///	+ Script modifies transform
	/// + Physics modifies transform
	/// + Animation modifies transform
	///
	virtual SSCDataConflictSolver* GetSSCDataConflictSolver() = 0;
};

using SSC = SubSystemComponent;

class SSCDataConflictSolver
{
public:
	virtual ~SSCDataConflictSolver() = default;

public:
	virtual void Resolve(GameObject* object, SSC* component) = 0;

};


NAMESPACE_END