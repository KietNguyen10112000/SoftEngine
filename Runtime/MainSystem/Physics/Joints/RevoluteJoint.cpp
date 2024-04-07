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

	auto joint = PxRevoluteJointCreate(*px,
		a0->is<PxRigidActor>(),
		PhysXUtils::ToPxTransform(localFrame0),
		a1->is<PxRigidActor>(),
		PhysXUtils::ToPxTransform(localFrame1)
	);

	/*joint->setLimit(PxJointAngularLimitPair(-PxPi / 4, PxPi / 4));
	joint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);

	joint->setDriveVelocity(10.0f);
	joint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, true);*/

	InitJoint(joint, body0, body1);
}

NAMESPACE_END