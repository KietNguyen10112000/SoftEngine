#pragma once

#include "Fundamental.h"

namespace math
{

class Transform
{
private:
	Vec4 m_scale = {};
	Quaternion m_rotation = {};
	Vec4 m_translation = {};

public:
	inline Mat4 ToTransformMatrix() const
	{
		assert(m_scale.w == 0);
		assert(m_translation.w == 1);
		return Mat4::Scaling(m_scale.xyz()) * Mat4::Translation(m_translation.xyz()) * Mat4::Rotation(m_rotation);
	}

	inline bool Equals(const Transform& transform)
	{
		return m_scale			== transform.m_scale 
			&& m_translation	== transform.m_translation
			&& m_rotation		== transform.m_rotation;
	}

public:
	inline auto& Scale() const
	{
		return m_scale;
	}

	inline auto& Translation() const
	{
		return m_translation;
	}

	inline auto& Rotation() const
	{
		return m_rotation;
	}

};

}