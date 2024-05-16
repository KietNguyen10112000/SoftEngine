#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"

#include "Common/Base/Serializable.h"

NAMESPACE_BEGIN

namespace raw 
{
	template <typename _C>
	class AsyncTaskRunnerForMainComponent;
}

#define COMPONENT_CLASS(className) SERIALIZABLE_CLASS(className)

class MainComponent : public Serializable
{
private:
	friend class GameObject;
	friend class DoubleBVH;
	friend class Scene;
	friend class ModifiedRecorder;

	template <typename _C>
	friend class raw::AsyncTaskRunnerForMainComponent;

	bool m_recorded = false;
	bool m_padd[7];

protected:
	struct DoubleBVHId
	{
		ID bvhId = INVALID_ID;
		ID ulistId = INVALID_ID;
	};

	DoubleBVHId m_doubleBVHId[2] = {};
	GameObject* m_object = nullptr;
	GameObject* m_committedObject = nullptr;

	std::atomic<void*> m_forAsyncTaskRunner[2] = {0};

public:
	// called when object contains this component added to scene
	virtual void OnComponentAdded() = 0;

	// called when object contains this component removed from scene
	virtual void OnComponentRemoved() = 0;

	virtual void OnTransformChanged() = 0;

	virtual AABox GetGlobalAABB() = 0;

	inline virtual void OnDrawDebug() {};
	inline virtual void Wake() {};

public:
	inline GameObject* GetCommittedObject()
	{
		return m_committedObject;
	}

	inline GameObject* GetGameObject()
	{
		return GetCommittedObject();
	}

	inline GameObject* GetCurrentObject()
	{
		return m_object;
	}

};

NAMESPACE_END