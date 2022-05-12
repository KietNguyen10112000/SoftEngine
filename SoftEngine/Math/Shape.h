#pragma once

#include "./Math.h"

namespace Math
{


/// 
/// just shape, not oriented geometry
/// 

class SphereShape
{
public:
	float m_radius;

public:
	inline SphereShape(float radius) : m_radius(radius) {};

public:
	inline auto& Radius() { return m_radius; };

};

class CapsuleShape
{
public:
	float m_radius;
	float m_height;

public:
	inline CapsuleShape(float radius, float height) : m_radius(radius), m_height(height) {};

public:
	inline auto& Radius() { return m_radius; };
	inline auto& Height() { return m_height; };

};

class BoxShape
{
public:
	Vec3 m_dimensions;

public:
	inline BoxShape(float x, float y, float z) : m_dimensions(x, y, z) {};
	inline BoxShape(const Vec3& dimensions) : m_dimensions(dimensions) {};

public:
	inline auto& Dimensions() { return m_dimensions; };

	inline auto GetWidth() const { return m_dimensions.x; };
	inline auto GetHeight() const { return m_dimensions.y; };
	inline auto GetLength() const { return m_dimensions.z; };

};


};