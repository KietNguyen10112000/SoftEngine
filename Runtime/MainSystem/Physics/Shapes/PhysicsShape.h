#pragma once

#include "Core/TypeDef.h"

#include "PHYSICS_SHAPE_TYPE.h"

#include "../PhysicsClasses.h"

namespace physx
{
	class PxShape;
}

NAMESPACE_BEGIN

class PhysicsShape
{
protected:
	PHYSICS_FRIEND_CLASSES();

	physx::PxShape* m_pxShape = nullptr;

public:
	virtual ~PhysicsShape();

public:
	virtual PHYSICS_SHAPE_TYPE GetType() const = 0;

};

NAMESPACE_END