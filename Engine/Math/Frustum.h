#pragma once
#include "Fundamental.h"

#include "Plane.h"

namespace math
{

class Frustum
{
public:
	union
	{
		struct
		{
			Plane m_nearFace;
			Plane m_farFace;

			Plane m_topFace;
			Plane m_bottomFace;

			Plane m_rightFace;
			Plane m_leftFace;
		};

		// planes[i].normal > < planes[i + 1].normal
		Plane m_planes[6];
	};
    

public:
	inline Frustum(const Mat4& projection)
	{
		Vec3 corners[8];
		GetFrustumCornersFromProjectionMatrix(corners, projection);

		//0-----1
		//|     |
		//3-----2
		auto* nearPlane = &corners[0];
		auto* farPlane = &corners[4];

		m_nearFace = Plane(nearPlane[0], nearPlane[1], nearPlane[2]);
		MakeHull(m_nearFace, farPlane[0]);
		m_farFace = Plane(farPlane[0], farPlane[1], farPlane[2]);
		MakeHull(m_farFace, nearPlane[0]);

		m_leftFace = Plane(nearPlane[0], nearPlane[3], farPlane[0]);
		MakeHull(m_leftFace, nearPlane[1]);
		m_rightFace = Plane(nearPlane[1], nearPlane[2], farPlane[1]);
		MakeHull(m_rightFace, nearPlane[0]);

		m_topFace = Plane(nearPlane[0], nearPlane[1], farPlane[0]);
		MakeHull(m_topFace, nearPlane[3]);
		m_bottomFace = Plane(nearPlane[3], nearPlane[2], farPlane[3]);
		MakeHull(m_bottomFace, nearPlane[0]);
	}

private:
	//// make angle between plane1's normal and plane2's normal <is < PI / 2
	//inline void MakeHull(Plane& p1, Plane& p2)
	//{
	//	auto n1 = p1.GetNormal();
	//	auto n2 = p2.GetNormal();

	//	auto dot = n1.Dot(n2);

	//	if (dot <= 0)
	//	{
	//		return;
	//	}

	//	constexpr static float sign1[3] = { -1,  1, -1 };
	//	constexpr static float sign2[3] = {  1, -1, -1 };

	//	for (size_t i = 0; i < 3; i++)
	//	{
	//		auto tn1 = sign1[i] * n1;
	//		auto tn2 = sign2[i] * n2;
	//		
	//		if (tn1.Dot(tn2) <= 0)
	//		{
	//			p1.normal = tn1;
	//			p2.normal = tn2;
	//			return;
	//		}
	//	}
	//}

	inline void MakeHull(Plane& p1, const Vec3& p)
	{
		auto n1 = p1.GetNormal();

		if (p1.ValueOf(p) >= 0)
		{
			return;
		}

		p1.normal = -n1;
	}

public:
	inline static void GetFrustumCornersFromProjectionMatrix(Vec3 (&corners)[8], const Mat4& projection)
	{
		Vec4 hcorners[8];

		// NDC near plane
		hcorners[0] = Vec4(-1, 1, 0, 1);
		hcorners[1] = Vec4(1, 1, 0, 1);
		hcorners[2] = Vec4(1, -1, 0, 1);
		hcorners[3] = Vec4(-1, -1, 0, 1);

		// NDC far plane
		hcorners[4] = Vec4(-1, 1, 1, 1);
		hcorners[5] = Vec4(1, 1, 1, 1);
		hcorners[6] = Vec4(1, -1, 1, 1);
		hcorners[7] = Vec4(-1, -1, 1, 1);

		Mat4 inverseProj = projection.GetInverse();

		for (int i = 0; i < 8; i++) {
			hcorners[i] = hcorners[i] * inverseProj;
			hcorners[i] = hcorners[i] * (1.0f / hcorners[i].w);

			corners[i] = Vec3(hcorners[i].x, hcorners[i].y, hcorners[i].z);
		}
	}

	inline static void GetProjectionMatrixFromFrustumCorners(Mat4& out, Vec3 (&corners)[8])
	{
		// TODO
		assert(0 && "TODO");
	}

	inline static void GetBoundingSphereFromFrustumCorners(Vec3 (&corners)[8], Vec3& outCenter, float& outRadius)
	{
		//0-----1
		//|     |
		//3-----2
		auto* nearPlane = &corners[0];
		auto* farPlane = &corners[4];

		auto& p1 = nearPlane[0];
		auto& p2 = nearPlane[2];

		auto& p3 = farPlane[0];
		auto& p4 = farPlane[2];

		auto c1 = (p1 + p3) / 2.0f;
		auto c2 = (p3 + p4) / 2.0f;

		Plane plane1 = Plane(c1, p3 - p1);
		Plane plane2 = Plane(c2, p4 - p3);

		//plane contains p1, p2, p3
		Plane plane3 = Plane(p1, p2, p3);

		//must contains p4
		//assert(IsFloatEqual(plane3.Value(p4), 0, 0.001f));

		Line line;
		plane1.Intersect(plane2, line);

		Vec3 center;
		line.Intersect(plane3, center);

		auto radius = (center - p1).Length();
		outCenter = center;
		outRadius = radius;
	}

public:
	inline Frustum& Transform(const Mat4& mat)
	{
		for (auto& p : m_planes)
		{
			p.Transform(mat);
		}

		return *this;
	}


	// convex hull algorithm
	inline bool IsOverlap(const AABox& aabox) const
	{
		// source from https://www.flipcode.com/archives/Frustum_Culling.shtml

		Vec3 vCorner[8];
		//int iTotalIn = 0;

		// get the corners of the box into the vCorner array
		aabox.GetPoints(vCorner);

		// test all 8 corners against the 6 sides 
		// if all points are behind 1 specific plane, we are out
		// if we are in with all points, then we are fully in
		for (int p = 0; p < 6; ++p) 
		{
			int iInCount = 8;
			//int iPtIn = 1;

			for (int i = 0; i < 8; ++i) 
			{
				// test this point against the planes
				if (m_planes[p].ValueOf(vCorner[i]) < 0)
				{
					//iPtIn = 0;
					--iInCount;
				}
			}

			// were all the points outside of plane p?
			if (iInCount == 0)
			{
				return false;
			}

			// check if they were all on the right side of the plane
			//iTotalIn += iPtIn;
		}

		// so if iTotalIn is 6, then all are inside the view
		//if (iTotalIn == 6)
		//	return(IN);

		// we must be partly in then otherwise
		return true;
	}

};

}