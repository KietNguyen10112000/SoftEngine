#pragma once
#include <cmath>
#include <DirectXCollision.h>

#include "Math.h"

#include <vector>

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



class Frustum
{
public:
	Vec3 m_corners[8];

public:
	inline Frustum() {};
	Frustum(const Mat4x4& projection);
	inline ~Frustum() {};

public:
	static void GetFrustumCorners(std::vector<Vec3>& corners, const Mat4x4& projection);
	static void GetFrustumCorners(Vec3* corners, const Mat4x4& projection);
	static void GetProjectionMatrix(Mat4x4& out, Vec3* corners);

	static void GetBoundingSphere(Vec3* corners, Vec3* outCenter, float* outRadius);

public:
	inline void FromProjectionMatrix(const Mat4x4& projection) { GetFrustumCorners(&m_corners[0], projection); };

};

inline Frustum::Frustum(const Mat4x4& projection)
{
	GetFrustumCorners(&m_corners[0], projection);
}


inline void Frustum::GetFrustumCorners(std::vector<Vec3>& corners, const Mat4x4& projection)
{
	corners.clear();
	corners.resize(8);
	GetFrustumCorners(&corners[0], projection);
}

inline void Frustum::GetFrustumCorners(Vec3* corners, const Mat4x4& projection)
{
	Vec4 hcorners[8];

	hcorners[0] = Vec4(-1, 1, 0, 1);
	hcorners[1] = Vec4(1, 1, 0, 1);
	hcorners[2] = Vec4(1, -1, 0, 1);
	hcorners[3] = Vec4(-1, -1, 0, 1);

	hcorners[4] = Vec4(-1, 1, 1, 1);
	hcorners[5] = Vec4(1, 1, 1, 1);
	hcorners[6] = Vec4(1, -1, 1, 1);
	hcorners[7] = Vec4(-1, -1, 1, 1);

	Mat4x4 inverseProj = GetInverse(projection);

	for (int i = 0; i < 8; i++) {
		hcorners[i] = hcorners[i] * inverseProj;
		hcorners[i] = hcorners[i] * (1 / hcorners[i].w);

		corners[i] = Vec3(hcorners[i].x, hcorners[i].y, hcorners[i].z);
	}
}

//void CalBoundingSphere(std::vector<Vec3>& corners, Vec3* outCenter, float* outRadius)
//{
//	//0-----1
//	//|     |
//	//3-----2
//	auto* farPlane = &corners[4];
//	auto* nearPlane = &corners[0];
//	
//	Vec3 center = {};
//	for (size_t i = 0; i < 8; i++)
//	{
//		center = center + corners[i];
//	}
//	center = center / 8.0f;
//
//	float r = 0;
//	for (size_t i = 0; i < 8; i++)
//	{
//		r = max((center - corners[i]).Length(), r);
//	}
//
//	*outRadius = r;
//	*outCenter = center;
//}

//2nd method
inline void Frustum::GetBoundingSphere(Vec3* corners, Vec3* outCenter, float* outRadius)
{
	//0-----1
	//|     |
	//3-----2
	auto* farPlane = &corners[4];
	auto* nearPlane = &corners[0];

	auto& p1 = nearPlane[0];
	auto& p2 = nearPlane[2];

	auto& p3 = farPlane[0];
	auto& p4 = farPlane[2];

	auto c1 = (p1 + p3) / 2.0f;
	auto c2 = (p3 + p4) / 2.0f;

	Plane3D plane1 = Plane3D(c1, p3 - p1);
	Plane3D plane2 = Plane3D(c2, p4 - p3);

	//plane contains p1, p2, p3
	Plane3D plane3 = Plane3D(p1, p2, p3);

	//must contains p4
	//assert(IsFloatEqual(plane3.Value(p4), 0, 0.001f));

	auto line = plane1.Intersect(plane2);

	auto center = line.Intersect(plane3);

	auto radius = (center - p1).Length();
	*outCenter = center;
	*outRadius = radius;
}


};