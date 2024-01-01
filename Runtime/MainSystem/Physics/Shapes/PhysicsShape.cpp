#include "PhysicsShape.h"

#include "PxPhysicsAPI.h"

NAMESPACE_BEGIN

PhysicsShape::~PhysicsShape()
{
	if (m_pxShape)
		m_pxShape->release();

	m_pxShape = nullptr;
}

NAMESPACE_END