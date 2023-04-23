#pragma once

#include "Core/TypeDef.h"

#include "Math/Math.h"

#include "../Collision/Manifold.h"

NAMESPACE_BEGIN

class Collider
{
public:
	virtual AABox GetLocalAABB() = 0;
	virtual void Collide(const Sphere& sphere,			Manifold* output) {};
	virtual void Collide(const Box& box,				Manifold* output) {};
	virtual void Collide(const Frustum& frustum,		Manifold* output) {};

};

NAMESPACE_END