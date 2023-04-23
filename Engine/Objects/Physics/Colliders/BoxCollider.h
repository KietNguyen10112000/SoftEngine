#pragma once

#include "Collider.h"
#include "ColliderId.h"

NAMESPACE_BEGIN

class BoxCollider : public Collider
{
protected:
	Vec3 m_dimensions;

public:
	virtual void Collide(const Sphere& sphere, Manifold* output) override {}

	virtual void Collide(const Box& box, Manifold* output) override 
	{

	}

	virtual void Collide(const Frustum& frustum, Manifold* output) {}

	virtual AABox GetLocalAABB() override
	{
		return 0;
	}

};

NAMESPACE_END