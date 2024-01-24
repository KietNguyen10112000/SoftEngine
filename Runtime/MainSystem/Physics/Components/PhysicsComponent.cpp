#include "PhysicsComponent.h"

#include "PxPhysicsAPI.h"

NAMESPACE_BEGIN

PhysicsComponent::~PhysicsComponent() 
{
	if (m_pxActor)
	{
		m_pxActor->release();
		m_pxActor = nullptr;
	}

	if (m_collisionResult)
	{
		delete m_collisionResult;
		m_collisionResult = nullptr;
	}
};

void PhysicsComponent::SetPhysicsFlag(PHYSICS_FLAG flag, bool value)
{
	if (value)
	{
		m_physicsFlag |= flag;
	}
	else
	{
		m_physicsFlag &= ~flag;
	}

	if (flag == PHYSICS_FLAG_ENABLE_COLLISION) 
	{
		if (m_collisionResult)
		{
			m_collisionResult->Clear();
		}

		if (HasPhysicsFlag(PHYSICS_FLAG_ENABLE_COLLISION))
		{
			m_collisionResult = new PhysicsCollisionResult();
		}
	}

}

NAMESPACE_END