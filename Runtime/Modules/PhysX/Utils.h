#pragma once

#include "Core/TypeDef.h"

#include "PhysX.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

class PhysXUtils
{
public:
	inline static physx::PxTransform ToPxTransform(const Transform& transform)
	{
		auto& pos = transform.GetPosition();
		auto& rot = transform.GetRotation();
		return physx::PxTransform(
			physx::PxVec3(pos.x, pos.y, pos.z),
			physx::PxQuat(rot.x, rot.y, rot.z, rot.w)
		);
	}

};

NAMESPACE_END