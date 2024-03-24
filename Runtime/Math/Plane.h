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
	inline Plane& Transform(const Mat4& mat)
	{
		auto n = Vec4(m_normal, 0.0f) * mat;
		auto p = Vec4(GetPoint(), 1.0f) * mat;
		p /= p.w;

		m_normal = n.xyz().Normalize();
		d = -normal.Dot(p.xyz());

		return *this;
	}

	inline Plane& Inverse()
	{
		m_normal = -m_normal;
		d = -d;
		return *this;
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
		/*if (x != 0) return Vec3(-w / x, 0, 0);
		else if (y != 0) return Vec3(0, -w / y, 0);
		return Vec3(0, 0, -w / z);*/

		return -d * m_normal;
	};

	inline auto ValueOf(const Vec3& point) const
	{
		return point.Dot(normal) + d;
	}

	inline auto Distance(const Vec3& point) const
	{
		return std::abs(ValueOf(point));
	}

	//plane must be normlize
	//get vector that translate from the nearest point on plane of point to point 
	inline Vec3 VecDistance(const Vec3& point) const noexcept
	{
		return m_normal * Distance(point);
	}

	inline int SideOf(const Vec3& point) const
	{
		const auto val = ValueOf(point);
		return (0 < val) - (val < 0);
	}

	inline bool Project(const Vec3& point, const Vec3& direction, Vec3& output) const
	{
		auto line = Line(point, direction);
		return line.Intersect(*this, output);
	}

	// the matrix that project any point onto the plane
	// ProjectedPoint = (Vec4(Point, 1.0f) * ProjectionMatrix).xyz()
	inline bool GetProjectionMatrix(const Vec3& direction, Mat4& output) const
	{
		// p + (d / std::abs(dot)) * plane.ValueOf(p);

		auto dot = std::abs(normal.Dot(direction));

		if (dot == 0)
		{
			return false;
		}

		auto v = direction / dot;
		
		auto m = Mat4(
			v.x * a, v.y * a, v.z * a, 0,
			v.x * b, v.y * b, v.z * b, 0,
			v.x * c, v.y * c, v.z * c, 0,
			v.x * d, v.y * d, v.z * d, 0
		);

		auto i = Mat4(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 0
		);

		m = i + m;

		output = m;

		return true;
	}
};

}