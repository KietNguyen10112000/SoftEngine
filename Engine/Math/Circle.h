#pragma once

#include "Fundamental.h"

namespace math
{

class Circle
{
public:
	Vec2 m_center = {};
	float m_radius = 0;

public:
	Circle(const Vec2& center, float radius) : m_center(center), m_radius(radius) {};

public:
	inline void Transform(const Mat3& mat)
	{
		auto r = Vec3(m_radius + m_center.x, m_center.y, 1.0f);
		r = r * mat;
		r /= r.z;

		auto v = Vec3(m_center, 1.0f) * mat;
		v /= v.z;
		m_center = v.xy();

		m_radius = (m_center - r.xy()).Length();
	}

	/*inline bool Intersect(const Vec2& p1, const Vec2& p2) const
	{
		auto line = Line2D::FromPoints(p1, p2);
		auto d = (p2 - p1).Normalize();

		if (line.Distance(m_center) > m_radius)
		{
			return false;
		}
		
		auto v1 = d.Dot(p1);
		auto v2 = d.Dot(p2);
		auto v3 = d.Dot(m_center);

		auto c1 = (v1 + v2) / 2.0f;
		auto e1 = std::abs(v1 - v2) / 2.0f;
		auto& c2 = v3;
		auto& e2 = m_radius;

		if (e1 + e2 - std::abs(c1 - c2) < 0)
		{
			return false;
		}

		return true;
	}*/

	inline bool Intersect(const Vec2& p1, const Vec2& p2, Line2D& lp1p2, float& outputD, float& outputOverlap) const
	{
		//outputLine = Line2D::FromPoints(p1, p2);
		lp1p2 = Line2D::FromPoints(p1, p2);
		auto d = (p2 - p1).Normalize();
		auto& line = lp1p2;

		auto side = line.ValueOf(m_center);
		outputD = std::abs(side);

		if (outputD > m_radius)
		{
			return false;
		}

		auto dist2 = side * side;
		auto r2 = m_radius * m_radius;

		auto n = line.normal;
		if (side > 0)
		{
			n = -n;
		}

		auto offset = std::sqrt(r2 - dist2);
		auto p = m_center + n * m_radius;

		auto tangen = Vec2(-n.y, n.x);
		auto _p1 = p + offset * tangen;
		auto _p2 = p - offset * tangen;

		auto v1 = d.Dot(_p1);
		auto v2 = d.Dot(_p2);
		auto v3 = d.Dot(p1);
		auto v4 = d.Dot(p2);

		auto c1 = (v1 + v2) / 2.0f;
		auto e1 = std::abs(v1 - v2) / 2.0f;
		auto c2 = (v3 + v4) / 2.0f;
		auto e2 = std::abs(v3 - v4) / 2.0f;

		outputOverlap = e1 + e2 - std::abs(c1 - c2);
		if (outputOverlap < 0)
		{
			return false;
		}

		return true;
	}

};

}