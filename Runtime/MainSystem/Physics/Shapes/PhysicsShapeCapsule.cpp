#include "PhysicsShapeCapsule.h"

#include "PhysX/PhysX.h"

#include "../Materials/PhysicsMaterial.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsShapeCapsule::PhysicsShapeCapsule(void* pxShape, float h, float r, const SharedPtr<PhysicsMaterial>& material, bool isExclusive)
{
	m_pxShape = (PxShape*)pxShape;
	m_pxShape->acquireReference();
	m_pxShape->userData = this;
	m_meterial = material;
}


PhysicsShapeCapsule::PhysicsShapeCapsule(float h, float r, const SharedPtr<PhysicsMaterial>& material, bool isExclusive)
{
	auto physics = PhysX::Get()->GetPxPhysics();
	auto& m = *(material->m_pxMaterial);
	m_pxShape = physics->createShape(PxCapsuleGeometry(r, h / 2.0f), m, isExclusive);

	m_pxShape->userData = this;
	m_meterial = material;
}

NAMESPACE_END