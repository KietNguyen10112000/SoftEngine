#include "PhysicsDefaultMaterial.h"

NAMESPACE_BEGIN

PhysicsDefaultMaterial::PhysicsDefaultMaterial()
{
	m_material = std::make_shared<PhysicsMaterial>(0.5f, 0.5f, 0.5f);
}

NAMESPACE_END