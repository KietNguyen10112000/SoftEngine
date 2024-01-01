#pragma once

#include "PhysicsShape.h"

#include "Math/Math.h"

#include "Core/Memory/SmartPointers.h"

NAMESPACE_BEGIN

class PhysicsMaterial;

class PhysicsShapePlane : public PhysicsShape
{
public:
	PhysicsShapePlane(const SharedPtr<PhysicsMaterial>& material, bool isExclusive = false);

	inline virtual PHYSICS_SHAPE_TYPE GetType() const
	{
		return PHYSICS_SHAPE_TYPE_PLANE;
	};
};

NAMESPACE_END