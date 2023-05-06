#pragma once

#include "Core/TypeDef.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

struct Collision2DResult
{
	float	penetration = 0;

	union
	{
		// normal toward from A collider to B collider
		Vec2	normal = {};
		Vec2 AB;
	};
	
	inline bool HasCollision() const
	{
		return penetration != 0;
	}
};

NAMESPACE_END