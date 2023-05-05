#pragma once

#include "Fundamental.h"

namespace math
{

class Transform2D
{
public:
	float m_rotation;
	Vec2  m_scale = { 1,1 };
	Vec2  m_translation;

public:
	inline Mat3 ToTransformMatrix() const
	{
		return Mat3::Scaling(m_scale) * Mat3::Rotation(m_rotation) * Mat3::Translation(m_translation);
	}

	inline bool Equals(const Transform2D& transform)
	{
		return m_scale			== transform.m_scale 
			&& m_translation	== transform.m_translation
			&& m_rotation		== transform.m_rotation;
	}

	inline bool operator==(const Transform2D& transform)
	{
		return Equals(transform);
	}

	inline bool operator!=(const Transform2D& transform)
	{
		return m_scale != transform.m_scale
			|| m_translation != transform.m_translation
			|| m_rotation != transform.m_rotation;
	}

	inline Transform2D& Transform(const Transform2D& transform)
	{
		m_scale *= transform.m_scale;
		m_rotation += transform.m_rotation;
		m_translation += transform.m_translation;
	}

	inline static void Joint(const Transform2D& transform1, const Transform2D& transform2, Transform2D& output)
	{
		output.m_scale = transform1.m_scale * transform2.m_scale;
		output.m_translation = transform1.m_translation + transform2.m_translation;
		output.m_rotation = transform1.m_rotation + transform2.m_rotation;
	}

public:
	inline auto& Scale()
	{
		return m_scale;
	}

	inline auto& Translation()
	{
		return m_translation;
	}

	inline auto& Rotation()
	{
		return m_rotation;
	}

	inline auto& GetScale() const
	{
		return m_scale;
	}

	inline auto& GetTranslation() const
	{
		return m_translation;
	}

	inline auto& GetRotation() const
	{
		return m_rotation;
	}

};

}