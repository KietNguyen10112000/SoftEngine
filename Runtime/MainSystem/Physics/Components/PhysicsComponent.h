#pragma once

#include "MainSystem/MainSystemInfo.h"
#include "Common/Base/MainComponent.h"

#include "PHYSICS_TYPE.h"

#include "Math/Math.h"

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

protected:
	physx::PxActor* m_pxActor = nullptr;

public:
	virtual ~PhysicsComponent();

	virtual PHYSICS_TYPE GetPhysicsType() const = 0;

};

NAMESPACE_END