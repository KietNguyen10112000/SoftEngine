#pragma once

#include "./Math.h"

namespace Math
{

class Transform
{
public:
	Vec3 m_scale;
	Vec4 m_rotation;
	Vec3 m_position;

public:
	Transform() : m_rotation(0, 0, 0, 1) {};

	Transform(
		const Vec3& scale,
		const Vec4& rotation,
		const Vec3& position
	) : m_scale(scale), m_rotation(rotation), m_position(position) {};

	Transform(const Mat4x4& mat)
	{
		mat.Decompose(&m_scale, &m_rotation, &m_position);
	};

	inline auto& Scale() const { return m_scale; };
	inline auto& Rotation() const { return m_rotation; };
	inline auto& Position() const { return m_position; };

public:
	inline Mat4x4 ToMatrix() const
	{
		Mat4x4 ret;
		ret.SetRotation(m_rotation);
		ret.MulScaleComponent(m_scale);
		ret.SetPosition(m_position);
		return ret;
	};

	inline Transform& FromMatrix(const Mat4x4& mat)
	{
		mat.Decompose(&m_scale, &m_rotation, &m_position);
		return *this;
	};

};

}