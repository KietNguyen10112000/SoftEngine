#pragma once

#include "Fundamental.h"

#include <algorithm>

namespace math
{

/// 
/// center position
/// 
class AABox
{
public:
	Vec3 m_center = { 0, 0, 0 };
	Vec3 m_halfDimensions = { 0, 0, 0 };

public:
	inline AABox() {};

	inline AABox(const Vec3& center, const Vec3& dimensions) 
		: m_center(center), m_halfDimensions(dimensions / 2.0) {};

	inline AABox(const Vec3* points, size_t count)
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

		m_halfDimensions = Vec3(maxX - minX, maxY - minY, maxZ - minZ) / 2.0f;
		m_center = Vec3(minX, minY, minZ) + m_halfDimensions;
	};

	//Y as up axis
	inline void GetPoints(Vec3* output) const
	{
		const auto dx = m_halfDimensions.x;
		const auto dy = m_halfDimensions.y;
		const auto dz = m_halfDimensions.z;

		//1st plane, upper plane
		output[0] = m_center + Vec3(-dx	,	dy,		-dz	);
		output[1] = m_center + Vec3( dx	,	dy,		-dz	);
		output[2] = m_center + Vec3(-dx	,	dy,		 dz	);
		output[3] = m_center + Vec3( dx	,	dy,		 dz	);

		//2nd plane
		output[4] = m_center + Vec3(-dx	,	-dy,	-dz	);
		output[5] = m_center + Vec3( dx	,	-dy,	-dz	);
		output[6] = m_center + Vec3(-dx	,	-dy,	dz	);
		output[7] = m_center + Vec3( dx	,	-dy,	dz	);
	};

	inline void Joint(const AABox& right)
	{
		Vec3 points[16];
		right.GetPoints(points);
		GetPoints(&points[8]);
		FromPoints(points, 16);
	};

	inline void Joint(const AABox& right, Vec3* buffer)
	{
		right.GetPoints(buffer);
		GetPoints(&buffer[8]);
		FromPoints(buffer, 16);
	};

	inline void Joint(const AABox* aabbs, size_t count, Vec3* buffer)
	{
		Vec3 points[16];

		for (size_t i = 0; i < count; i++)
		{
			Joint(aabbs[i], points);
		}
	};

	inline AABox MakeJointed(const AABox& right) const
	{
		AABox ret = *this;
		ret.Joint(right);
		return ret;
	}

	inline bool operator==(const AABox& right) const
	{
		return m_center == right.m_center && m_halfDimensions == right.m_halfDimensions;
	}

public:
	inline AABox MakeTransform(const Mat4& mat) const
	{
		AABox ret = *this;
		ret.Transform(mat);
		return ret;
	}

	inline void Transform(const Mat4& mat)
	{
		auto dimensions = m_halfDimensions * 2.0f;
		Vec3 pos = m_center - m_halfDimensions;
		Vec3 v1 = Vec3(dimensions.x, 0, 0);
		Vec3 v2 = Vec3(0, dimensions.y, 0);
		Vec3 v3 = Vec3(0, 0, dimensions.z);

		auto temp = Vec4(pos, 1.0f) * mat;
		pos = temp.xyz() / temp.w;
		v1 = (Vec4(v1, 0.0f) * mat).xyz();
		v2 = (Vec4(v2, 0.0f) * mat).xyz();
		v3 = (Vec4(v3, 0.0f) * mat).xyz();

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
		return m_halfDimensions != Vec3(0, 0, 0);
	};


	inline bool IsOverlap(const AABox& aabox) const
	{
		const auto& a = *this;
		const auto& b = aabox;

		if (std::abs(a.m_center[0] - b.m_center[0]) > (a.m_halfDimensions[0] + b.m_halfDimensions[0])) return false;
		if (std::abs(a.m_center[1] - b.m_center[1]) > (a.m_halfDimensions[1] + b.m_halfDimensions[1])) return false;
		if (std::abs(a.m_center[2] - b.m_center[2]) > (a.m_halfDimensions[2] + b.m_halfDimensions[2])) return false;

		return true;
	}

public:
	inline auto GetHalfDimensions() const { return m_halfDimensions; };
	inline auto GetDimensions() const { return m_halfDimensions * 2.0f; };
	inline auto GetCenter() const { return m_center; };

public:
	template <typename Fn>
	inline static AABox From(size_t count, Fn GET_POSITION_FUNC)
	{
		using std::max;
		using std::min;

		AABox ret = {};

		float
			maxX = -FLT_MAX, minX = FLT_MAX,
			maxY = -FLT_MAX, minY = FLT_MAX,
			maxZ = -FLT_MAX, minZ = FLT_MAX;

		for (size_t i = 0; i < count; i++)
		{
			auto& point = GET_POSITION_FUNC(i);

			maxX = max(maxX, point.x);
			minX = min(minX, point.x);

			maxY = max(maxY, point.y);
			minY = min(minY, point.y);

			maxZ = max(maxZ, point.z);
			minZ = min(minZ, point.z);
		}

		ret.m_halfDimensions = Vec3(maxX - minX, maxY - minY, maxZ - minZ) / 2.0f;
		ret.m_center = Vec3(minX, minY, minZ) + ret.m_halfDimensions;

		return ret;
	}

};

}