#pragma once
#include "Fundamental.h"

#include "Line.h"

namespace math
{

class Plane
{
public:
	union
	{
		Vec3 normal;
		Vec3 m_normal;

		struct
		{
			float a, b, c;
		};

		struct
		{
			float x, y, z;
		};
	};

	union
	{
		float distance;
		float m_distance;
		float d;
		float w;
	};

public:
	inline Plane() {};

	inline Plane(const Vec3& p1, const Vec3& p2, const Vec3& p3)
	{
		auto p12 = p2 - p1;
		auto p13 = p3 - p1;

		normal = p12.Cross(p13).Normalize();
		d = -normal.Dot(p1);
	}

	inline Plane(const Vec3& point, const Vec3& normal)
	{
		m_normal = normal.Normal();
		d = -normal.Dot(point);
	};

public:
	inline Plane& Inverse()
	{
		m_normal = -m_normal;
		d = -d;
	}

	inline bool Intersect(const Plane& plane, Line& intersectLine) const noexcept
	{
		Line& ret = intersectLine;

		//assume that n1, n2 normalized
		auto n1 = GetNormal();
		auto n2 = plane.GetNormal();

		// plane1 // plane 2
		if (n1 == n2)
		{
			return false;
		}

		ret.m_direction = n1.Cross(n2).Normalize();

		Line line = Line(GetPoint(), n1.Cross(ret.m_direction));
		line.Intersect(plane, ret.m_point);

		return true;
	};

public:
	inline Vec3 GetNormal() const
	{
		return normal;
	}

	// get a point on Plane
	inline Vec3 GetPoint() const
	{
		if (x != 0) return Vec3(-w / x, 0, 0);
		else if (y != 0) return Vec3(0, -w / y, 0);
		return Vec3(0, 0, -w / z);
	};

	inline auto ValueAt(const Vec3& point) const
	{
		return point.Dot(normal) + d;
	}

	inline auto Distance(const Vec3& point) const
	{
		return std::abs(ValueAt(point));
	}

	//plane must be normlize
	//get vector that translate from the nearest point on plane of point to point 
	inline Vec3 VecDistance(const Vec3& point) const noexcept
	{
		return m_normal * Distance(point);
	}

	inline int SideOf(const Vec3& point) const
	{
		const auto val = ValueAt(point);
		return (0 < val) - (val < 0);
	}

};

}