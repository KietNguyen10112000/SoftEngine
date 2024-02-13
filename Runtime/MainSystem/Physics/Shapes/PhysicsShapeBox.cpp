#include "PhysicsShapeBox.h"

#include "PhysX/PhysX.h"

#include "../Materials/PhysicsMaterial.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsShapeBox::PhysicsShapeBox(const Vec3& dimensions, const SharedPtr<PhysicsMaterial>& material, bool isExclusive)
{
	auto physics = PhysX::Get()->GetPxPhysics();
	auto& m = *(material->m_pxMaterial);
	m_pxShape = physics->createShape(PxBoxGeometry(dimensions.x / 2.0f, dimensions.y / 2.0f, dimensions.z / 2.0f), m, isExclusive);

	m_pxShape->userData = this;
	m_meterial = material;
}

NAMESPACE_END