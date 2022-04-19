#pragma once

#include "./Math.h"

namespace Math
{

//top-left (smallest x,y,z) position
class AABB
{
public:
	Vec3 m_position = { 0, 0, 0 };
	Vec3 m_dimensions = { 0, 0, 0 };

public:
	inline AABB() {};

	inline AABB(const Vec3* points, size_t count)
	{
		FromPoints(points, count);
	};

public:
	inline void FromPoints(const Vec3* points, size_t count)
	{
		using std::max;
		using std::min;

		float
			maxX = -FLT_MAX, minX = FLT_MAX,
			maxY = -FLT_MAX, minY = FLT_MAX,
			maxZ = -FLT_MAX, minZ = FLT_MAX;

		for (size_t i = 0; i < count; i++)
		{
			auto& point = points[i];

			maxX = max(maxX, point.x);
			minX = min(minX, point.x);

			maxY = max(maxY, point.y);
			minY = min(minY, point.y);

			maxZ = max(maxZ, point.z);
			minZ = min(minZ, point.z);
		}

		m_position = Vec3(minX, minY, minZ);
		m_dimensions = Vec3(maxX - minX, maxY - minY, maxZ - minZ);
	};

	//Y as up axis
	inline void GetPoints(Vec3* output) const
	{
		//1st plane
		output[0] = m_position;
		output[1] = m_position + Vec3(m_dimensions.x, 0, 0);
		output[2] = m_position + Vec3(0, 0, m_dimensions.z);
		output[3] = m_position + Vec3(m_dimensions.x, 0, m_dimensions.z);

		//2nd plane
		output[4] = m_position + Vec3(0, m_dimensions.y, 0);
		output[5] = m_position + Vec3(m_dimensions.x, m_dimensions.y, 0);
		output[6] = m_position + Vec3(0, m_dimensions.y, m_dimensions.z);
		output[7] = m_position + Vec3(m_dimensions.x, m_dimensions.y, m_dimensions.z);
	};

	inline void Joint(const AABB& right)
	{
		Vec3 points[16];
		right.GetPoints(points);
		GetPoints(&points[8]);
		FromPoints(points, 16);
	};

	inline void Joint(const AABB& right, Vec3* buffer)
	{
		right.GetPoints(buffer);
		GetPoints(&buffer[8]);
		FromPoints(buffer, 16);
	};

	inline void Joint(const AABB* aabbs, size_t count, Vec3* buffer)
	{
		Vec3 points[16];

		for (size_t i = 0; i < count; i++)
		{
			Joint(aabbs[i], points);
		}
	};

public:
	inline void Transform(const Mat4x4& mat)
	{
		////Vec4 vecX = { m_dimensions.x, 0, 0, 0 };
		////Vec4 vecY = { 0, m_dimensions.y, 0, 0 };
		////Vec4 vecZ = { 0, 0, m_dimensions.z, 0 };

		//Vec3 points[8];
		//GetPoints(points);

		//for (size_t i = 0; i < 8; i++)
		//{
		//	points[i] = ConvertVector(Vec4(points[i], 1.0f) * mat);
		//}

		//FromPoints(points, 8);

		Vec3 pos = m_position;
		Vec3 v1 = Vec3(m_dimensions.x, 0, 0);
		Vec3 v2 = Vec3(0, m_dimensions.y, 0);
		Vec3 v3 = Vec3(0, 0, m_dimensions.z);

		pos = ConvertVector(Vec4(pos, 1.0f) * mat);
		v1 = ConvertVector(Vec4(v1, 0.0f) * mat);
		v2 = ConvertVector(Vec4(v2, 0.0f) * mat);
		v3 = ConvertVector(Vec4(v3, 0.0f) * mat);

		Vec3 points[8];
		points[0] = pos;
		points[1] = pos + v1;
		points[2] = pos + v2;
		points[3] = pos + v3;

		points[4] = pos + v1 + v2;
		points[5] = pos + v1 + v3;
		points[6] = pos + v2 + v3;
		points[7] = pos + v1 + v2 + v3;

		FromPoints(points, 8);
	};

	inline bool IsValid() const
	{
		return m_dimensions != Vec3(0, 0, 0);
	};

public:
	inline auto& Dimensions() { return m_dimensions; };
	inline auto& Position() { return m_position; };

public:
	template <Vec3& (*GET_POSITION_FUNC)(size_t, void**)>
	inline static AABB From(size_t count, void** arg)
	{
		using std::max;
		using std::min;

		AABB ret = {};

		float
			maxX = -FLT_MAX, minX = FLT_MAX,
			maxY = -FLT_MAX, minY = FLT_MAX,
			maxZ = -FLT_MAX, minZ = FLT_MAX;

		for (size_t i = 0; i < count; i++)
		{
			auto& point = GET_POSITION_FUNC(i, arg);

			maxX = max(maxX, point.x);
			minX = min(minX, point.x);

			maxY = max(maxY, point.y);
			minY = min(minY, point.y);

			maxZ = max(maxZ, point.z);
			minZ = min(minZ, point.z);
		}

		ret.m_position = Vec3(minX, minY, minZ);
		ret.m_dimensions = Vec3(maxX - minX, maxY - minY, maxZ - minZ);

		return ret;
	}

};

}