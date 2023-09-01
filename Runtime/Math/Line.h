#pragma once
#include "Fundamental.h"

namespace math
{

// 3d line
class Line
{
private:
	friend class Plane;

public:
	Vec3 m_point;
	Vec3 m_direction;

public:
	inline Line() {};

	inline Line(const Vec3& point, const Vec3& direction) : m_point(point), m_direction(direction)
	{
		m_direction.Normalize();
	};

	inline Line(void* padding, const Vec3& point1, const Vec3& point2) : m_point(point1)
	{
		m_direction = (point2 - point1).Normalize();
	};


public:
	inline bool Intersect(const Plane& plane, Vec3& intersectPoint) const noexcept;

};

}