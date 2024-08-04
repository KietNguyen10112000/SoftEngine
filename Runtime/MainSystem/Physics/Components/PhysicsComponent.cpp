#include "PhysicsComponent.h"

#include "PxPhysicsAPI.h"

#include "Scene/GameObject.h"

#include "MainSystem/MainSystemTaskPacking.h"
#include "MainSystem/Physics/PhysicsSystem.h"

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

	if (flag == PHYSICS_FLAG_COLLISION_RESULT)
	{
		MAIN_SYSTEM_TASK_COMMON_1(PhysicsSystem, AsyncTaskRunnerST, value,
			{
				/*if (self->m_collisionResult)
				{
					m_collisionResult->Clear();
				}*/

				self->OnPhysicsFlagSetted(PHYSICS_FLAG_COLLISION_RESULT, value);
				if (self->HasPhysicsFlag(PHYSICS_FLAG_COLLISION_RESULT) && !self->m_collisionResult)
				{
					self->m_collisionResult = new PhysicsCollisionResult();
				}
			}
		);
	}

}

void PhysicsComponent::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void PhysicsComponent::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void PhysicsComponent::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void PhysicsComponent::SerializeToJson(Serializer* serializer, json& j) const
{
	j["PhysicsFlags"] = m_physicsFlag;
}

void PhysicsComponent::DeserializeFromJson(Serializer* serializer, const json& j)
{
	m_physicsFlag = j["PhysicsFlags"];

	for (size_t i = 0; i < sizeof(m_physicsFlag) * 8; i++)
	{
		auto enable = ((m_physicsFlag & (1ull << i)) >> i);
		SetPhysicsFlag(PHYSICS_FLAG((1ull << i)), enable);
	}
}

NAMESPACE_END