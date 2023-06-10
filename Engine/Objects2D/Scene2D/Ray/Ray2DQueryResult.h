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

	inline void StoreIfSameSide(const Ray2D& ray, const Vec2& v)
	{
		auto d = v - ray.begin;
		auto length2 = d.Length2();
		// same side, length < ray's length
		if (d.Dot(ray.direction) > 0 && length2 <= ray.length2)
		{
			points.push_back({ v, length2 });
		}
	}
};

NAMESPACE_END