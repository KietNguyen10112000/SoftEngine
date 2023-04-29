#pragma once

#include "Collider.h"
#include "ColliderId.h"

NAMESPACE_BEGIN

class BoxCollider : public Collider
{
protected:
	Vec3 m_dimensions;

public:
	BoxCollider(const Vec3& dims) : m_dimensions(dims) {};

	virtual void Collide(const Sphere& sphere, Manifold* output) override {}

	virtual void Collide(const Box& box, Manifold* output) override 
	{

	}

	virtual void Collide(const Frustum& frustum, Manifold* output) {}

	virtual AABox GetLocalAABB() override
	{
		return AABox({ 0,0,0 }, m_dimensions);
	}

};

NAMESPACE_END