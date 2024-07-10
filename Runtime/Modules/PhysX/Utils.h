#pragma once

#include "Core/TypeDef.h"

#include "PhysX.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

class PhysXUtils
{
public:
	inline static physx::PxQuat ToPxQuat(const Quaternion& quat)
	{
		return physx::PxQuat(quat.x, quat.y, quat.z, quat.w);
	}

	inline static Quaternion ToQuaternion(const physx::PxQuat& quat)
	{
		return Quaternion(quat.x, quat.y, quat.z, quat.w);
	}

	inline static physx::PxVec3 ToPxVec3(const Vec3& vec)
	{
		return reinterpret_cast<const physx::PxVec3&>(vec);
	}

	inline static Vec3 ToVec3(const physx::PxVec3& vec)
	{
		return reinterpret_cast<const Vec3&>(vec);
	}

	inline static physx::PxTransform ToPxTransform(const Transform& transform)
	{
		auto& pos = transform.GetPosition();
		auto& rot = transform.GetRotation();
		return physx::PxTransform(
			physx::PxVec3(pos.x, pos.y, pos.z),
			physx::PxQuat(rot.x, rot.y, rot.z, rot.w)
		);
	}

	inline static Transform ToTransform(const physx::PxTransform& pxtransform)
	{
		auto ret = Transform();
		ret.Position() = ToVec3(pxtransform.p);
		ret.Rotation() = ToQuaternion(pxtransform.q);
		return ret;
	}

};

NAMESPACE_END