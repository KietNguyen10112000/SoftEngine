#include "RigidBody.h"

#include "Scene/GameObject.h"

#include "PxPhysicsAPI.h"

using namespace physx;

NAMESPACE_BEGIN

void RigidBody::OnTransformChanged()
{
	auto gameObject = GetGameObject();

	auto& globalTransform = gameObject->ReadGlobalTransformMat();

	auto pxRigidBody = m_pxActor->is<PxRigidBody>();

	assert(pxRigidBody && "something wrong here!");

	auto pxTransform = pxRigidBody->getGlobalPose();

}

NAMESPACE_END