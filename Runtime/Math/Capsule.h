#pragma once
#include "Fundamental.h"

namespace math
{

class Capsule
{
public:
	Vec3 m_center = {};
	float m_height = 0;
	float m_radius = 0;

public:
	Capsule(const Vec3& center, float height, float radius) : m_center(center), m_height(height), m_radius(radius) {};

};

}