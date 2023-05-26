#pragma once

#include "Components2D/Physics/Physics2D.h"

NAMESPACE_BEGIN

struct Ray2DQueryInfo : public Ray2DQueryResult
{
	struct SORT_MASK
	{
		enum 
		{
			SORT_POINT				= (1ull << 0),
			SORT_OBJECT				= (1ull << 1),
		};
	};

	struct Object2DResult
	{
		GameObject2D* object;

		// index of Ray2DQueryResult::points
		size_t idxBegin;
		size_t idxEnd;

		PointOnRay* closestPoint;
	};

	std::Vector<Object2DResult> objectResult;

	Ray2DQueryInfo() : Ray2DQueryResult(8*KB)
	{
		objectResult.reserve(1 * KB);
	}

	inline void Clear()
	{
		Ray2DQueryResult::Clear();
		objectResult.clear();
	}

	template <typename FN>
	void ForEachObject(FN fn)
	{
		for (auto& obj : objectResult)
		{
			if (fn(obj.object)) break;
		}
	}
};

NAMESPACE_END