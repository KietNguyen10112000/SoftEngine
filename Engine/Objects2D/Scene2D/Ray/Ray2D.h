#pragma once

#include "Core/TypeDef.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

struct Ray2D
{
	Vec2 begin = {};
	Vec2 direction = {};
	float length = 0;
	float length2 = 0;

	Ray2D() {};

	Ray2D(const Vec2& _begin, const Vec2& _end)
	{
		begin = _begin;
		auto d = _end - _begin;
		length = d.Length();
		direction = d / length;
		length2 = length * length;
	}
};

NAMESPACE_END