#include "PhysicsShapePlane.h"

#include "PhysX/PhysX.h"

#include "../Materials/PhysicsMaterial.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsShapePlane::PhysicsShapePlane(const SharedPtr<PhysicsMaterial>& material, bool isExclusive)
{
	auto physics = PhysX::Get()->GetPxPhysics();
	auto& m = *(material->m_pxMaterial);
	m_pxShape = physics->createShape(PxPlaneGeometry(), m, isExclusive);

	m_pxShape->userData = this;
	m_meterial = material;
}

NAMESPACE_END