#include "Joint.h"

#include "PhysX/PhysX.h"

using namespace physx;

NAMESPACE_BEGIN

Joint::~Joint()
{
	if (m_pxJoint)
	{
		m_pxJoint->release();
		m_pxJoint = nullptr;
	}
}

void Joint::InitJoint(physx::PxJoint* pxJoint, const Handle<RigidBody>& body0, const Handle<RigidBody>& body1)
{
	m_body0 = body0;
	m_body1 = body1;
}

NAMESPACE_END