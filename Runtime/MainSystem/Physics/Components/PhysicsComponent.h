#pragma once

#include "MainSystem/MainSystemInfo.h"
#include "Common/Base/MainComponent.h"

#include "PHYSICS_TYPE.h"
#include "PHYSICS_FLAG.h"

#include "Math/Math.h"

#include <bitset>

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