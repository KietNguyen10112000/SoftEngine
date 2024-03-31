#include "RevoluteJoint.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "../Components/RigidBody.h"

using namespace physx;

NAMESPACE_BEGIN

RevoluteJoint::RevoluteJoint(const Handle<RigidBody>& body0, const Transform& localFrame0, const Handle<RigidBody>& body1, const Transform& localFrame1)
{
	auto px = PhysX::Get()->GetPxPhysics();

	auto a0 = body0->m_pxActor;
	auto a1 = body1->m_pxActor;

	auto j = PxRevoluteJointCreate(*px,
		a0->is<PxRigidActor>(),
		PhysXUtils::ToPxTransform(localFrame0),
		a0->is<PxRigidActor>(),
		PhysXUtils::ToPxTransform(localFrame1)
	);

	InitJoint(j, body0, body1);
}

NAMESPACE_END