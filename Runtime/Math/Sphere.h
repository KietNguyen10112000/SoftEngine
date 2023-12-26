#pragma once
#include "Fundamental.h"

namespace math
{

class Sphere
{
public:
	Vec3 m_center = {};
	float m_radius = 0;

public:
	Sphere(const Vec3& center, float radius) : m_center(center), m_radius(radius) {};

};

}