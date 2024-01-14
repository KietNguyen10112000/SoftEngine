#include "RigidBody.h"

#include "Scene/GameObject.h"

#include "PxPhysicsAPI.h"

using namespace physx;

NAMESPACE_BEGIN

void RigidBody::OnTransformChanged()
{
	auto gameObject = GetGameObject();

	auto& globalTransform = gameObject->ReadGlobalTransformMat();

	//auto pxRigidBody = m_pxActor->is<PxRigidBody>();

	//assert(pxRigidBody && "something wrong here!");

	PxRigidActor* pxRigidBody = m_pxActor->is<PxRigidActor>();

	//auto pxTransform = pxRigidBody->getGlobalPose();

	if (::memcmp(&m_lastGlobalTransform, &globalTransform, sizeof(Mat4)) != 0)
	{
		Vec3 scale;
		Vec3 pos;
		Quaternion rot;
		globalTransform.Decompose(scale, rot, pos);

		PxTransform pxTransform;
		pxTransform.p = reinterpret_cast<PxVec3&>(pos);
		pxTransform.q.x = rot.x;
		pxTransform.q.y = rot.y;
		pxTransform.q.z = rot.z;
		pxTransform.q.w = rot.w;

		pxRigidBody->setGlobalPose(pxTransform);

		m_lastGlobalTransform = globalTransform;
	}
}

NAMESPACE_END