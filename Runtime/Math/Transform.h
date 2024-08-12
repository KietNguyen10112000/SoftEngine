#pragma once

#include "Fundamental.h"

namespace math
{

class Transform
{
private:
	Vec4 m_scale = { 1,1,1,0 };
	Quaternion m_rotation = {};
	Vec4 m_translation = {};

public:
	inline static Transform FromTransformMatrix(const Mat4& mat)
	{
		Transform ret = {};
		mat.Decompose(ret.m_scale.xyz(), ret.m_rotation, ret.m_translation.xyz());
		return ret;
	}

	inline Mat4 ToTransformMatrix() const
	{
		assert(m_scale.w == 0);
		assert(m_translation.w == 0);
		return Mat4::Scaling(m_scale.xyz()) * Mat4::Rotation(m_rotation) * Mat4::Translation(m_translation.xyz());
	}

	inline bool Equals(const Transform& transform, float eps = 0.0f) const
	{
		return m_scale.xyz().Equals(transform.m_scale.xyz(), eps)
			&& m_translation.xyz().Equals(transform.m_translation.xyz(), eps)
			&& m_rotation.Equals(transform.m_rotation, eps);
	}

	inline bool operator==(const Transform& transform) const
	{
		return m_scale == transform.m_scale
			&& m_translation == transform.m_translation
			&& m_rotation == transform.m_rotation;
	}

public:
	inline auto& Scale()
	{
		return m_scale.xyz();
	}

	inline auto& Translation()
	{
		return m_translation.xyz();
	}

	inline auto& Rotation()
	{
		return m_rotation;
	}

	inline auto& Position()
	{
		return m_translation.xyz();
	}

	inline auto& GetScale() const
	{
		return m_scale.xyz();
	}

	inline auto& GetTranslation() const
	{
		return m_translation.xyz();
	}

	inline auto& GetRotation() const
	{
		return m_rotation;
	}

	inline auto& GetPosition() const
	{
		return m_translation.xyz();
	}

};

}