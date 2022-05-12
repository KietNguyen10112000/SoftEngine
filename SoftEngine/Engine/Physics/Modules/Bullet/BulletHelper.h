#pragma once

#include "Math/Math.h"

#include "Bullet3Common/b3Vector3.h"
#include "Bullet3Common/b3Matrix3x3.h"

class BulletHelper
{
public:
	inline static Vec3 btVector3ToVec3(const btVector3& btvec3)
	{
		Vec3 ret = *(Vec3*)&btvec3;
		return ret;
	};

	inline static btVector3 Vec3TobtVector3(const Vec3 vec3)
	{
		return btVector3(vec3.x, vec3.y, vec3.z);
	};

	// =))
	inline static Mat4x4 btMatrix3x3ToMat4x4(const btMatrix3x3& btMat)
	{
		Mat4x4 ret;
		auto* dest = (btMatrix3x3*)&ret.m[0][0];
		*dest = btMat;
		return ret;
	};

	inline static btMatrix3x3 Mat4x4TobtMatrix3x3(const Mat4x4& mat)
	{
		btMatrix3x3 ret;
		auto* src = (btMatrix3x3*)&mat.m[0][0];
		ret = *src;
		return ret;
	};

};