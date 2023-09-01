#pragma once

namespace math
{

inline bool Line::Intersect(const Plane& plane, Vec3& intersectPoint) const noexcept
{
	auto vec = plane.VecDistance(m_point);

	auto length = vec.Length();

	if (length == 0) 
	{
		intersectPoint = m_point;
		return true;
	}

	auto dot = vec.Normal().Dot(m_direction);

	if (dot == 0)
	{
		return false;
	}

	intersectPoint = m_point - (m_direction * length) / dot;
	return true;
}

}