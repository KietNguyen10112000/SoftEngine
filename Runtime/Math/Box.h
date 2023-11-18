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
	inline static Box From(const AABox& aaBox)
	{

	}

};

}