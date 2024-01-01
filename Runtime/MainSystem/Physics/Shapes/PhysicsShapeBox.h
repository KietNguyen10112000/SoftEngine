#pragma once

#include "PhysicsShape.h"

#include "Math/Math.h"

#include "Core/Memory/SmartPointers.h"

NAMESPACE_BEGIN

class PhysicsMaterial;

class PhysicsShapeBox : public PhysicsShape
{
public:
	PhysicsShapeBox(const Vec3& dimensions, const SharedPtr<PhysicsMaterial>& material, bool isExclusive = false);

	inline virtual PHYSICS_SHAPE_TYPE GetType() const
	{
		return PHYSICS_SHAPE_TYPE_BOX;
	};
};

NAMESPACE_END