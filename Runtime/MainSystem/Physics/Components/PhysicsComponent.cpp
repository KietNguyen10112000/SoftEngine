#include "PhysicsComponent.h"

#include "PxPhysicsAPI.h"

#include "Scene/GameObject.h"

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

bool PhysicsComponent::HasCollisionContactPairsBegin()
{
	return m_collisionResult
		&& GetGameObject()->GetScene()->GetIterationCount() == m_collisionResult->lastActiveIterationCount + 1
		&& m_collisionResult->GetBeginContactPairsCount() != 0;
}

bool PhysicsComponent::HasCollisionContactPairsEnd()
{
	return m_collisionResult
		&& GetGameObject()->GetScene()->GetIterationCount() == m_collisionResult->lastActiveIterationCount + 1
		&& m_collisionResult->GetEndContactPairsCount() != 0;
}

bool PhysicsComponent::HasCollisionContactPairs()
{
	return m_collisionResult
		&& m_collisionResult->GetContactPairsCount() != 0;
}

//bool PhysicsComponent::HasCollisionModified()
//{
//	return m_collisionResult
//		&& GetGameObject()->GetScene()->GetIterationCount() == m_collisionResult->lastActiveIterationCount + 1
//		&& m_collisionResult->collision.Read()->collisionModifiedIds.size() != 0;
//}

bool PhysicsComponent::HasCollisionAnyChanged()
{
	return m_collisionResult
		&& GetGameObject()->GetScene()->GetIterationCount() == m_collisionResult->lastActiveIterationCount + 1;
}

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