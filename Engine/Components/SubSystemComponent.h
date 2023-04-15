#pragma once

#include "Core/TypeDef.h"

#include "Core/Memory/Memory.h"

#include "Math/AABox.h"

NAMESPACE_BEGIN

class GameObject;

class SubSystemComponent
{
protected:
	friend class GameObject;

	// the script component belong to
	GameObject* m_object = nullptr;

public:
	// called when this component is added to object
	virtual void OnComponentAdded() = 0;

	// called when this component is removed from object
	virtual void OnComponentRemoved() = 0;

	// called when object contains this component added to scene
	virtual void OnComponentAddedToScene() = 0;

	// called when object contains this component removed from scene
	virtual void OnComponentRemovedFromScene() = 0;

	// main component is allowed to modify GameObject's properties
	virtual void SetAsMain() = 0;

	// extra one will read from GameObject's properties and write to its own extra buffer
	virtual void SetAsExtra() = 0;

	///
	/// resolve conflict of main component and extra component on GameObject's properties
	/// eg: 
	///	+ Script modifies transform
	/// + Physics modifies transform
	/// + Animation modifies transform
	///
	virtual void ResolveBranch() = 0;
	virtual bool IsNewBranch() = 0;

	// override this method if modifies AABB during process
	inline virtual math::AABox GetLocalAABB()
	{
		return math::AABox();
	}

public:
#undef GetObject
	inline GameObject* GetObject()
	{
		return m_object;
	}

};

NAMESPACE_END