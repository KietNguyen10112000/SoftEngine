#pragma once
#include "Fundamental.h"

namespace math
{

// oriented box
class Box
{
public:
	Vec3 m_position = {};
	Vec3 m_d1;
	Vec3 m_d2;
	Vec3 m_d3;

public:
	/*inline static Box From(const AABox& aaBox)
	{

	}*/

	inline Box(const Vec3& position, const Vec3& d1, const Vec3& d2, const Vec3& d3) 
		: m_position(position), m_d1(d1), m_d2(d2), m_d3(d3) {}

};

}