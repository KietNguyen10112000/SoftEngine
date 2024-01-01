#include "PhysicsMaterial.h"

#include "PxPhysicsAPI.h"

#include "PhysX/PhysX.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsMaterial::PhysicsMaterial(float staticFriction, float dynamicFriction, float restitution)
{
	m_pxMaterial = PhysX::Get()->GetPxPhysics()->createMaterial(staticFriction, dynamicFriction, restitution);
}

PhysicsMaterial::~PhysicsMaterial()
{
	if (m_pxMaterial)
		m_pxMaterial->release();

	m_pxMaterial = nullptr;
}

float PhysicsMaterial::GetStaticFriction()
{
	return m_pxMaterial->getStaticFriction();
}

void PhysicsMaterial::SetStaticFriction(float staticFriction)
{
	m_pxMaterial->setStaticFriction(staticFriction);
}

float PhysicsMaterial::GetDynamicFriction()
{
	return m_pxMaterial->getDynamicFriction();
}

void PhysicsMaterial::SetDynamicFriction(float dynamicFriction)
{
	m_pxMaterial->setDynamicFriction(dynamicFriction);
}

float PhysicsMaterial::GetRestitution()
{
	return m_pxMaterial->getRestitution();
}

void PhysicsMaterial::SetRestitution(float restitution)
{
	m_pxMaterial->setRestitution(restitution);
}

NAMESPACE_END