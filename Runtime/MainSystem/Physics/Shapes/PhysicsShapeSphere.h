#pragma once

#include "PhysicsShape.h"

#include "Math/Math.h"

#include "Core/Memory/SmartPointers.h"

NAMESPACE_BEGIN

class PhysicsMaterial;

class PhysicsShapeSphere : public PhysicsShape
{
public:
	PhysicsShapeSphere(float radius, const SharedPtr<PhysicsMaterial>& material, bool isExclusive = false);

	inline virtual PHYSICS_SHAPE_TYPE GetType() const 
	{
		return PHYSICS_SHAPE_TYPE_SPHERE;
	};
};

NAMESPACE_END