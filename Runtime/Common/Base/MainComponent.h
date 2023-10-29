#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"

#include "Common/Base/Serializable.h"

#include "Scene/MODIFICATION_STATE.h"

NAMESPACE_BEGIN

#define COMPONENT_CLASS(className) SERIALIZABLE_CLASS(className)

class MainComponent : public Serializable
{
private:
	friend class GameObject;
	friend class DoubleBVH;
	friend class Scene;

protected:
	struct DoubleBVHId
	{
		ID bvhId = INVALID_ID;
		ID ulistId = INVALID_ID;
	};

	MODIFICATION_STATE::STATE m_modificationState = MODIFICATION_STATE::NONE;

	DoubleBVHId m_doubleBVHId[2] = {};
	GameObject* m_object = nullptr;

public:
	// called when object contains this component added to scene
	virtual void OnComponentAdded() = 0;

	// called when object contains this component removed from scene
	virtual void OnComponentRemoved() = 0;

	virtual void OnTransformChanged() = 0;

	virtual AABox GetGlobalAABB() = 0;

public:
	inline GameObject* GetGameObject()
	{
		return m_object;
	}

};

NAMESPACE_END