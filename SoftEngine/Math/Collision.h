#pragma once
#include <cmath>
#include <DirectXCollision.h>

#include "Math.h"

namespace Math
{

#define IsFloatEqual(f1, f2, epsilon) (std::abs(f1 - f2) < epsilon)

//typedef Vec4 Plane3D;

typedef Vec3 Plane2D;

class Line3D
{
private:
	friend class Plane3D;

public:
	Vec3 m_point;
	Vec3 m_direction;

public:
	inline Line3D() {};

	inline Line3D(const Vec3& point, const Vec3& direction) : m_point(point), m_direction(direction)
	{
		m_direction.Normalize();
	};

	inline Line3D(void* padding, const Vec3& point1, const Vec3& point2) : m_point(point1)
	{
		m_direction = (point2 - point1).Normalize();
	};


public:
	Vec3 Intersect(const Plane3D& plane, bool* done = 0) const noexcept;

public:
	inline std::string ToString() const noexcept
	{
		std::string ret = "Line3D[point = ";
		ret += m_point.ToString();
		ret += ", direction = " + m_direction.ToString() + "]";
		return ret;
	};

	inline friend std::ostream& operator<<(std::ostream& os, const Line3D& line)
	{
		os << line.ToString();
		return os;
	};

};


//ax + by + cz + d = 0;
//(a,b,c,d) ~ (x,y,z,w)
class Plane3D : public XMFLOAT4
{
public:
	inline Plane3D() :XMFLOAT4(0, 0, 0, 0) {};
	inline Plane3D(const Vec3& v1, const Vec3& v2, const Vec3& v3)
	{
		XMStoreFloat4(this, XMPlaneFromPoints(XMLoadFloat3(&v1), XMLoadFloat3(&v2), XMLoadFloat3(&v3)));
		Normalize();
	};

	inline Plane3D(const Vec3& point, const Vec3& normal)
	{
		XMStoreFloat4(this, XMPlaneFromPointNormal(XMLoadFloat3(&point), XMLoadFloat3(&normal)));
		Normalize();
	};

public:
	inline Plane3D& Normalize()
	{
		XMStoreFloat4(this, XMPlaneNormalize(XMLoadFloat4(this)));
		return *this;
	};

	inline Vec3 GetNormal() const noexcept
	{
		return Vec3(x, y, z);
	};

	//get a point on Plane
	inline Vec3 GetPoint() const noexcept
	{
		if (x != 0) return Vec3(-w / x, 0, 0);
		else if (y != 0) return Vec3(0, -w / y, 0);
		return Vec3(0, 0, -w / z);
	};

public:
	inline Line3D Intersect(const Plane3D& plane, bool* done = 0) const noexcept
	{
		Line3D ret;

		//assume that n1, n2 normalized
		auto n1 = GetNormal();
		auto n2 = plane.GetNormal();

		if (n1 == n2)
		{
			if (done) *done = false;
			return {};
		}

		ret.m_direction = CrossProduct(n1, n2).Normalize();

		Line3D line = Line3D(GetPoint(), CrossProduct(n1, ret.m_direction));
		ret.m_point = line.Intersect(plane);

		return ret;
	};

	inline float Distance(const Vec3& point) const noexcept
	{
		return std::abs(DotProduct(Vec4(point, 1.0f), (*(Vec4*)this)));
	};

	//just cal value of Plane with an specify point
	inline float Value(const Vec3& point) const noexcept
	{
		return Distance(point);
	};

	//plane must be normlize
	//get vector that translate from the nearest point on plane of point to point 
	inline Vec3 VecDistance(const Vec3& point) const noexcept
	{
		auto n = GetNormal();
		auto dis = DotProduct(Vec4(point, 1.0f), (*(Vec4*)this));
		return n * dis;
	}

public:
	inline std::string ToString() const noexcept
	{
		std::string ret = "Plane3D[point = ";
		ret += GetPoint().ToString();
		ret += ", normal = " + GetNormal().ToString() + "]";
		return ret;
	};

	inline friend std::ostream& operator<<(std::ostream& os, const Plane3D& plane)
	{
		os << plane.ToString();
		return os;
	};

};

inline Vec3 Line3D::Intersect(const Plane3D& plane, bool* done) const noexcept
{
	auto vec = plane.VecDistance(m_point);

	auto length = vec.Length();

	if (length == 0) return m_point;

	auto dot = DotProduct(vec.Normal(), m_direction);

	if (dot == 0)
	{
		if (done) *done = false;
		return Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	}
	else
	{
		return m_point - (m_direction * length) / dot;
	}
}

inline Plane3D PlaneFrom(const Vec3& v1, const Vec3& v2, const Vec3& v3) noexcept
{
	Plane3D ret;
	XMStoreFloat4(&ret, XMPlaneFromPoints(XMLoadFloat3(&v1), XMLoadFloat3(&v2), XMLoadFloat3(&v3)));
	return ret;
}

inline Plane3D PlaneFrom(const Vec3& point, const Vec3& normal) noexcept
{
	Plane3D ret;
	XMStoreFloat4(&ret, XMPlaneFromPointNormal(XMLoadFloat3(&point), XMLoadFloat3(&normal)));
	return ret;
}

inline Vec3 PlaneToPlane(const Plane3D& p1, const Plane3D& p2) noexcept
{

}




};