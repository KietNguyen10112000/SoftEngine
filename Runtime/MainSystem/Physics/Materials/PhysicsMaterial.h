#pragma once

#include "Core/TypeDef.h"

#include "../PhysicsClasses.h"

namespace physx
{
	class PxMaterial;
}

NAMESPACE_BEGIN

class PhysicsMaterial
{
protected:
	PHYSICS_FRIEND_CLASSES();

	physx::PxMaterial* m_pxMaterial = nullptr;

public:
	PhysicsMaterial(float staticFriction, float dynamicFriction, float restitution);
	virtual ~PhysicsMaterial();

public:
	float GetStaticFriction();
	void SetStaticFriction(float staticFriction);

	float GetDynamicFriction();
	void SetDynamicFriction(float dynamicFriction);

	float GetRestitution();
	void SetRestitution(float restitution);

};

NAMESPACE_END