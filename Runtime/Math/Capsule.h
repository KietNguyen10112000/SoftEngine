#pragma once
#include "Fundamental.h"

namespace math
{

class Capsule
{
public:
	constexpr static Vec3 DEFAULT_UP_AXIS = Vec3::Y_AXIS;

	Vec3 m_up = DEFAULT_UP_AXIS;
	Vec3 m_center = {};
	float m_height = 0;
	float m_radius = 0;

public:
	Capsule(const Vec3& up, const Vec3& center, float height, float radius) : m_up(up.Normal()), m_center(center), m_height(height), m_radius(radius) {};
	Capsule(const Vec3& center, float height, float radius) : m_center(center), m_height(height), m_radius(radius) {};

};

}