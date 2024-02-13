#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/SmartPointers.h"

#include "PHYSICS_SHAPE_TYPE.h"

#include "../PhysicsClasses.h"

namespace physx
{
	class PxShape;
}

NAMESPACE_BEGIN

class PhysicsMaterial;

class PhysicsShape
{
protected:
	PHYSICS_FRIEND_CLASSES();

	physx::PxShape* m_pxShape = nullptr;

	SharedPtr<PhysicsMaterial> m_meterial;

public:
	virtual ~PhysicsShape();

public:
	virtual PHYSICS_SHAPE_TYPE GetType() const = 0;

	inline const auto& GetFirstMaterial()
	{
		return m_meterial;
	}

};

NAMESPACE_END