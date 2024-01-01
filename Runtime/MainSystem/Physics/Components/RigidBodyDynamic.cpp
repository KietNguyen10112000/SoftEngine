#include "RigidBodyDynamic.h"

#include "Scene/GameObject.h"

#include "PxPhysicsAPI.h"

using namespace physx;

NAMESPACE_BEGIN

void RigidBodyDynamic::OnTransformChanged()
{
	auto gameObject = GetGameObject();

	auto& globalTransform = gameObject->ReadGlobalTransformMat();

	auto pxRigidBody = m_pxActor->is<PxRigidBody>();
	auto pxTransform = pxRigidBody->getGlobalPose();


}

NAMESPACE_END