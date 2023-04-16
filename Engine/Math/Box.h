#pragma once
#include "Fundamental.h"

namespace math
{

// oriented box
class Box
{
public:
	Vec3 m_position = {};
	Vec3 d1;
	Vec3 d2;
	Vec3 d3;

public:
	inline static Box From(const AABox& aaBox)
	{

	}

};

}