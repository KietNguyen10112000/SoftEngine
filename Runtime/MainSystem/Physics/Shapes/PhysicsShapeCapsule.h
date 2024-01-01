#pragma once

#include "PhysicsShape.h"

#include "Math/Math.h"

#include "Core/Memory/SmartPointers.h"

NAMESPACE_BEGIN

class PhysicsMaterial;

class PhysicsShapeCapsule : public PhysicsShape
{
public:
	PhysicsShapeCapsule(float height, float radius, const SharedPtr<PhysicsMaterial>& material, bool isExclusive = false);

	inline virtual PHYSICS_SHAPE_TYPE GetType() const
	{
		return PHYSICS_SHAPE_TYPE_CAPSULE;
	};
};

NAMESPACE_END