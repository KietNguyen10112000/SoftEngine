#pragma once

#include "Fundamental.h"

#include <cmath>

namespace math
{

class Rect2D
{
public:
	//Vec2 m_topLeft;
	//Vec2 m_bottomRight;

	//     vec1
	//o----------->
	//|
	//vec2
	//|
	//V
	Vec2 m_point;
	Vec2 m_vec1;
	Vec2 m_vec2;

public:
	inline Rect2D() {};
	inline Rect2D(float x, float y, float w, float h)
	{
		//m_topLeft = { x,y };
		//m_bottomRight = { x + w, y + h };
		m_point = { x,y };
		m_vec1 = { w, 0 };
		m_vec2 = { 0, h };
	}

public:
	inline auto Width() const { return m_vec1.Length(); }
	inline auto Height() const { return m_vec2.Length(); }
	inline auto Center() const { return m_point + (m_vec1 + m_vec2) / 2.0f; }
	inline auto Diagonal() const { return m_vec1 + m_vec2; }

	inline auto Area() const { return m_vec1.Length() * m_vec2.Length(); }

	//"can" but not sure
	inline bool CanOverlap(const Rect2D& rect) const
	{
		using std::min;
		using std::max;

		const Rect2D* rect1 = this;
		const Rect2D* rect2 = &rect;

		auto p1 = rect1->m_point;
		auto p2 = rect1->m_point + rect1->m_vec1;
		auto p3 = rect1->m_point + rect1->m_vec2;
		auto p4 = p3 + rect1->m_vec1;

		auto _p1 = rect2->m_point;
		auto _p2 = rect2->m_point + rect2->m_vec1;
		auto _p3 = rect2->m_point + rect2->m_vec2;
		auto _p4 = _p3 + rect2->m_vec1;

		auto rectALeft = min(min(min(p1.x, p2.x), p3.x), p4.x);
		auto rectARight = max(max(max(p1.x, p2.x), p3.x), p4.x);
		auto rectATop = min(min(min(p1.y, p2.y), p3.y), p4.y);
		auto rectABottom = max(max(max(p1.y, p2.y), p3.y), p4.y);

		auto rectBLeft = min(min(min(_p1.x, _p2.x), _p3.x), _p4.x);
		auto rectBRight = max(max(max(_p1.x, _p2.x), _p3.x), _p4.x);
		auto rectBTop = min(min(min(_p1.y, _p2.y), _p3.y), _p4.y);
		auto rectBBottom = max(max(max(_p1.y, _p2.y), _p3.y), _p4.y);

		if (rectALeft >= rectBRight || rectBLeft >= rectARight)
			return false;

		if (rectABottom <= rectBTop || rectBBottom <= rectATop)
			return false;

		return true;
	}

	inline bool IsOverlap(const Rect2D& rect) const
	{
		auto area1 = Area();
		auto area2 = rect.Area();

		const Rect2D* rect1 = 0;
		const Rect2D* rect2 = 0;

		if (area1 > area2)
		{
			rect1 = &rect;
			rect2 = this;
		}
		else
		{
			rect2 = &rect;
			rect1 = this;
		}

		auto p1 = rect1->m_point;
		auto p2 = rect1->m_point + rect1->m_vec1;
		auto p3 = rect1->m_point + rect1->m_vec2;
		auto p4 = p3 + rect1->m_vec1;

		auto _p1 = rect2->m_point;
		auto _p2 = rect2->m_point + rect2->m_vec1;
		auto _p3 = rect2->m_point + rect2->m_vec2;
		auto _p4 = _p3 + rect2->m_vec1;

		return Point2DInTriangle(p1, _p1, _p2, _p3) || Point2DInTriangle(p2, _p1, _p2, _p3)
			|| Point2DInTriangle(p3, _p1, _p2, _p3) || Point2DInTriangle(p4, _p1, _p2, _p3)
			|| Point2DInTriangle(p1, _p4, _p2, _p3) || Point2DInTriangle(p2, _p4, _p2, _p3)
			|| Point2DInTriangle(p3, _p4, _p2, _p3) || Point2DInTriangle(p4, _p4, _p2, _p3);
	}

	//self rotation
	void Rotate(float radAngle)
	{
		Vec2 pivot = Center();

		auto cosAngle = cos(radAngle);
		auto sinAngle = sin(radAngle);

		float rX = pivot.x + (m_point.x - pivot.x) * cosAngle - (m_point.y - pivot.y) * sinAngle;
		float rY = pivot.y + (m_point.x - pivot.x) * sinAngle + (m_point.y - pivot.y) * cosAngle;

		m_point = { rX,rY };

		rX = (m_vec1.x) * cosAngle - (m_vec1.y) * sinAngle;
		rY = (m_vec1.x) * sinAngle + (m_vec1.y) * cosAngle;

		m_vec1 = { rX,rY };

		rX = (m_vec2.x) * cosAngle - (m_vec2.y) * sinAngle;
		rY = (m_vec2.x) * sinAngle + (m_vec2.y) * cosAngle;

		m_vec2 = { rX,rY };
	}

	inline void GetPoints(Vec2* output) const
	{
		output[0] = m_point;
		output[1] = m_point + m_vec1;
		output[2] = m_point + m_vec2;
		output[3] = m_point + m_vec1 + m_vec2;
	}

};


}