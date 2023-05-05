#pragma once

#include "Fundamental.h"

#include <algorithm>

namespace math
{
 
class AARect
{
public:
	Vec2 m_topLeft = { 0, 0 };
	Vec2 m_dimensions = { 0, 0 };

public:
	inline AARect() {};

	inline AARect(const Vec2& topLeft, const Vec2& dimensions)
		: m_topLeft(topLeft), m_dimensions(dimensions) {};

	inline AARect(const Vec2* points, size_t count)
	{
		FromPoints(points, count);
	};

public:
	inline void FromPoints(const Vec2* points, size_t count)
	{
		using std::max;
		using std::min;

		float
			maxX = -FLT_MAX, minX = FLT_MAX,
			maxY = -FLT_MAX, minY = FLT_MAX;

		for (size_t i = 0; i < count; i++)
		{
			auto& point = points[i];

			maxX = max(maxX, point.x);
			minX = min(minX, point.x);

			maxY = max(maxY, point.y);
			minY = min(minY, point.y);
		}

		m_dimensions = Vec2(maxX - minX, maxY - minY);
		m_topLeft = Vec2(minX, minY);
	};

	//Y as up axis
	inline void GetPoints(Vec2* output) const
	{
		output[0] = m_topLeft;
		output[1] = m_topLeft + Vec2(m_dimensions.x,	0);
		output[2] = m_topLeft + Vec2(0, m_dimensions.y);
		output[3] = m_topLeft + Vec2(m_dimensions.x, m_dimensions.y);
	};

	inline void Joint(const AARect& right)
	{
		Vec2 points[8];
		right.GetPoints(points);
		GetPoints(&points[4]);
		FromPoints(points, 8);
	};

	inline void Joint(const AARect& right, Vec2* buffer)
	{
		right.GetPoints(buffer);
		GetPoints(&buffer[4]);
		FromPoints(buffer, 8);
	};

	inline void Joint(const AARect* aabbs, size_t count, Vec3* buffer)
	{
		Vec2 points[8];

		for (size_t i = 0; i < count; i++)
		{
			Joint(aabbs[i], points);
		}
	};

	inline AARect MakeJointed(const AARect& right) const
	{
		AARect ret = *this;
		ret.Joint(right);
		return ret;
	}

	inline bool operator==(const AARect& right) const
	{
		return m_topLeft == right.m_topLeft && m_dimensions == right.m_dimensions;
	}

	inline bool operator!=(const AARect& right) const
	{
		return m_topLeft == right.m_topLeft && m_dimensions == right.m_dimensions;
	}

public:
	inline void Transform(const Mat3& mat)
	{
		auto& dimensions = m_dimensions;
		Vec2 pos = m_topLeft;
		Vec2 v1 = Vec2(dimensions.x, 0);
		Vec2 v2 = Vec2(0, dimensions.y);

		auto temp = Vec3(pos, 1.0f) * mat;
		pos = temp.xy() / temp.z;
		v1 = (Vec3(v1, 0.0f) * mat).xy();
		v2 = (Vec3(v2, 0.0f) * mat).xy();

		Vec2 points[4];
		points[0] = pos;
		points[1] = pos + v1;
		points[2] = pos + v2;
		points[3] = pos + v1 + v2;

		FromPoints(points, 4);
	};

	inline bool IsValid() const
	{
		return m_dimensions != Vec2(0, 0);
	};


	inline bool IsOverlap(const AARect& aaRect) const
	{
		const auto& a = *this;
		const auto& b = aaRect;

		return 
			   (std::abs((a.m_topLeft.x + a.m_dimensions.x / 2.0f)  
				   - (b.m_topLeft.x + b.m_dimensions.x / 2.0f)) * 2.0f < (a.m_dimensions.x + b.m_dimensions.x))
			&& (std::abs((a.m_topLeft.y + a.m_dimensions.y / 2.0f)
				- (b.m_topLeft.y + b.m_dimensions.y / 2.0f)) * 2.0f < (a.m_dimensions.y + b.m_dimensions.y));
	}

public:
	inline auto GetHalfDimensions() const { return m_dimensions / 2.0f; };
	inline auto GetDimensions() const { return m_dimensions; };
	inline auto GetCenter() const { return m_topLeft + m_dimensions / 2.0f; };

};

}