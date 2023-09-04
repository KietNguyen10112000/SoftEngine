#pragma once

#include "Core/TypeDef.h"
#include "Math/Math.h"

NAMESPACE_BEGIN


class AABBQueryTester
{
public:
	inline virtual ~AABBQueryTester() {};

public:
	virtual bool IsOverlapWithAABB(const AABox& aabb) = 0;

};


NAMESPACE_END