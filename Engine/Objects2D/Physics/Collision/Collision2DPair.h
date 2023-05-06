#pragma once

#include "Core/TypeDef.h"

#include "Collision2DResult.h"

NAMESPACE_BEGIN

class Physics2D;

struct Collision2DPair
{
	size_t refCount;

	ID id;

	Physics2D* A;
	Physics2D* B;

	Collision2DResult result;

	inline bool IsA(Physics2D* physics)
	{
		return physics == A;
	}

	inline auto GetAnotherOf(Physics2D* physics)
	{
		return IsA(physics) ? B : A;
	}

	inline Vec2 GetNormal(Physics2D* physics)
	{
		return IsA(physics) ? result.AB : (-result.AB);
	}
};

NAMESPACE_END