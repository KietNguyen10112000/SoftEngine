#include "Joint.h"

#include "PhysX/PhysX.h"

using namespace physx;

NAMESPACE_BEGIN

Joint::~Joint()
{
}

void Joint::InitJoint(physx::PxJoint* pxJoint, const Handle<PhysicsComponent>& actor0, const Handle<PhysicsComponent>& actor1)
{
	m_actor0 = actor0;
	m_actor1 = actor1;
}

NAMESPACE_END