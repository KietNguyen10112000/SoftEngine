#pragma once

#include "Math/Math.h"

class PhysicsObject
{
public:
	// known scale, common physics engine is not allowed modify scale when processing
	Vec3 m_scale = { 1,1,1 };
	int m_dynamic = 0;

public:
	inline virtual ~PhysicsObject() {};

public:
	///
	/// calculation base on m_scale
	/// 
	virtual void SetTransform(const Mat4x4& transform) = 0;
	virtual Mat4x4 GetTransform() = 0;

	virtual void ApplyForce(const Vec3& position, const Vec3& force) = 0;

public:
	virtual void SetScale(const Vec3& scale) { m_scale = scale; };
	inline auto GetScale() const { return m_scale; };

public:
	inline bool IsDynamic() const { return m_dynamic == 1; };

};