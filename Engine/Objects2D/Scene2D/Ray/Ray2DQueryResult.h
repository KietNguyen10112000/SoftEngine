#pragma once

#include "Core/TypeDef.h"

#include "Math/Math.h"

#include "Core/Structures/STD/STDContainers.h"

NAMESPACE_BEGIN

struct Ray2DQueryResult
{
	struct PointOnRay
	{
		Vec2 point;
		float length2;
		float padd;
	};

	std::Vector<PointOnRay> points;

	Ray2DQueryResult(size_t capacity)
	{
		points.reserve(capacity);
	}

	inline void Clear()
	{
		points.clear();
	}
};

NAMESPACE_END