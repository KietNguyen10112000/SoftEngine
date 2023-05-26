#pragma once

#include "Fundamental.h"

namespace math
{

class Line2D
{
public:
	union
	{
		Vec2 normal;
		Vec2 m_normal;

		struct
		{
			float a, b;
		};
	};

	union
	{
		float distance;
		float m_distance;
		float c;
	};

	inline Line2D(const Vec2& point, const Vec2& normal)
	{
		m_normal = normal.Normal();
		c = -m_normal.Dot(point);
	}

	inline static Line2D FromPoints(const Vec2& p1, const Vec2& p2)
	{
		return Line2D(p1, Vec2(p2.y - p1.y, p1.x - p2.x));
	}

	inline static Line2D FromPointAndDirection(const Vec2& p, const Vec2& direction)
	{
		return Line2D(p, Vec2(-direction.y, direction.x));
	}

	inline bool Intersect(const Line2D& line, Vec2& output)
	{
		float determinant = a * line.b - line.a * b;
		if (std::abs(determinant) < 0.00001f)
		{
			// Lines are parallel
			return false;
		}

		output.x = (-c * line.b + line.c * b) / determinant;
		output.y = (-a * line.c + line.a * c) / determinant;

		return true;
	}

	inline auto ValueOf(const Vec2& point)
	{
		return point.Dot(normal) + c;
	}

	inline void Initialize(const Vec2& point, const Vec2& normal)
	{
		m_normal = normal.Normal();
		c = -m_normal.Dot(point);
	}

	inline void InitializeFromPoints(const Vec2& p1, const Vec2& p2)
	{
		Initialize(p1, Vec2(p2.y - p1.y, p1.x - p2.x));
	}
};


}
