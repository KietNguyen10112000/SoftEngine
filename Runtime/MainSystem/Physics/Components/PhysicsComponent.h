#pragma once

#include "MainSystem/MainSystemInfo.h"
#include "Common/Base/MainComponent.h"

#include "PHYSICS_TYPE.h"
#include "PHYSICS_FLAG.h"

#include "Math/Math.h"

#include "Scene/DeferredBuffer.h"

//#include <bitset>

namespace physx
{
	class PxActor;
}

NAMESPACE_BEGIN

class API PhysicsComponent : public MainComponent
{
private:
	friend class GameObject;
	MAIN_SYSTEM_FRIEND_CLASSES();
	constexpr static ID COMPONENT_ID = MainSystemInfo::PHYSICS_ID;

private:
	size_t m_physicsFlag = 0;

protected:
	physx::PxActor* m_pxActor = nullptr;

public:
	virtual ~PhysicsComponent();

	virtual PHYSICS_TYPE GetPhysicsType() const = 0;

	inline virtual void OnDrawDebug() {};

protected:
	virtual void OnPhysicsTransformChanged() = 0;

	// called before PhysX fetchResults, use PhysicsSystem::ScheduleUpdate() to schedule update
	inline virtual void OnUpdate(float dt) {};

	// called after PhysX fetchResults, use PhysicsSystem::SchedulePostUpdate() to schedule post update
	inline virtual void OnPostUpdate(float dt) {};

	inline size_t& UpdateId()
	{
		return m_doubleBVHId[0].bvhId;
	}

	inline bool& IsUpdateIdRemoved()
	{
		return *(bool*)&m_doubleBVHId[0].ulistId;
	}

	inline size_t& PostUpdateId()
	{
		return m_doubleBVHId[1].bvhId;
	}

	inline bool& IsPostUpdateIdRemoved()
	{
		return *(bool*)&m_doubleBVHId[1].ulistId;
	}
	
public:
	inline void SetPhysicsFlag(PHYSICS_FLAG flag, bool value)
	{
		if (value)
		{
			m_physicsFlag |= flag;
		}
		else
		{
			m_physicsFlag &= ~flag;
		}
	}

	inline bool HasPhysicsFlag(PHYSICS_FLAG flag) const
	{
		return 0 != (m_physicsFlag & flag);
	}

};

NAMESPACE_END