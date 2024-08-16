#include "D6Joint.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "../Components/RigidBody.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "MainSystem/MainSystemTaskPacking.h"
#include "MainSystem/Physics/PhysicsSystem.h"

using namespace physx;

NAMESPACE_BEGIN

const static physx::PxD6Axis::Enum D6Joint_AXIS_To_Px_AXIS[] = {
	PxD6Axis::eX,
	PxD6Axis::eY,
	PxD6Axis::eZ,

	PxD6Axis::eTWIST,
	PxD6Axis::eSWING1,
	PxD6Axis::eSWING2,
};

const static physx::PxD6Motion::Enum D6Joint_MOTION_To_Px_MOTION[] = {
	PxD6Motion::eLOCKED,
	PxD6Motion::eLIMITED,
	PxD6Motion::eFREE,
};

const static D6Joint::MOTION_AXIS::ENUM Px_AXIS_To_D6Joint_AXIS[] = {
	D6Joint::MOTION_AXIS::X,
	D6Joint::MOTION_AXIS::Y,
	D6Joint::MOTION_AXIS::Z,

	D6Joint::MOTION_AXIS::TWIST_X,
	D6Joint::MOTION_AXIS::SWING_Y,
	D6Joint::MOTION_AXIS::SWING_Z,
};

const static D6Joint::MOTION_TYPE::ENUM Px_MOTION_To_D6Joint_MOTION[] = {
	D6Joint::MOTION_TYPE::LOCKED,
	D6Joint::MOTION_TYPE::LIMITED,
	D6Joint::MOTION_TYPE::FREE,
};

D6Joint::D6Joint(const Handle<RigidBody>& body0, const Transform& localFrame0, const Handle<RigidBody>& body1, const Transform& localFrame1)
{
	InitJoint(PxD6JointCreate, body0, localFrame0, body1, localFrame1);
}

void D6Joint::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void D6Joint::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void D6Joint::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void D6Joint::SerializeToJson(Serializer* serializer, json& j) const
{
	Joint::SerializeToJson(serializer, j);

	static auto SerializeLinearLimit = [](json& j, const char* name, const LinearLimit& limit)
	{
		auto& jobj = j[name];
		limit.SerializeToJson(jobj);
		jobj["Lower"] = limit.lower;
		jobj["Upper"] = limit.upper;
	};

	static auto SerializeSwingLimit = [](json& j, const char* name, const SwingLimit& limit)
	{
		auto& jobj = j[name];
		limit.SerializeToJson(jobj);
		jobj["YLimitAngle"] = limit.yLimitAngle;
		jobj["ZLimitAngle"] = limit.zLimitAngle;
	};

	static auto SerializeTwistLimit = [](json& j, const char* name, const TwistLimit& limit)
	{
		auto& jobj = j[name];
		limit.SerializeToJson(jobj);
		jobj["LowerLimit"] = limit.lowerLimit;
		jobj["UpperLimit"] = limit.upperLimit;
	};

	static auto SerializeDistanceLimit = [](json& j, const char* name, const DistanceLimit& limit)
	{
		auto& jobj = j[name];
		limit.SerializeToJson(jobj);
		jobj["Distance"] = limit.distance;
	};

	static auto SerializeDriveLimit = [](json& j, const char* name, const DriveLimit& limit)
	{
		auto& jobj = j[name];
		jobj["Stiffness"] = limit.stiffness;
		jobj["Damping"] = limit.damping;
		jobj["ForceLimit"] = limit.forceLimit;
		jobj["IsAcceleration"] = limit.isAcceleration;
	};

	{
		j["MotionXType"] = GetMotion(MOTION_AXIS::X);
		SerializeLinearLimit(j, "MotionXLimit", GetLinearLimit(MOTION_AXIS::X));
	}

	{
		j["MotionYType"] = GetMotion(MOTION_AXIS::Y);
		SerializeLinearLimit(j, "MotionYLimit", GetLinearLimit(MOTION_AXIS::Y));
	}

	{
		j["MotionZType"] = GetMotion(MOTION_AXIS::Z);
		SerializeLinearLimit(j, "MotionZLimit", GetLinearLimit(MOTION_AXIS::Z));
	}

	{
		j["SwingYType"] = GetMotion(MOTION_AXIS::SWING_Y);
		j["SwingZType"] = GetMotion(MOTION_AXIS::SWING_Z);
		SerializeSwingLimit(j, "SwingLimit", GetSwingLimit());
	}

	{
		j["TwistXType"] = GetMotion(MOTION_AXIS::TWIST_X);
		SerializeTwistLimit(j, "TwistLimit", GetTwistLimit());
	}

	{
		SerializeDistanceLimit(j, "DistanceLimit", GetDistanceLimit());
	}

	{
		{
			SerializeDriveLimit(j, "DriveLimitX", GetDrive(DRIVE_TYPE::X));
			SerializeDriveLimit(j, "DriveLimitY", GetDrive(DRIVE_TYPE::Y));
			SerializeDriveLimit(j, "DriveLimitZ", GetDrive(DRIVE_TYPE::Z));
			SerializeDriveLimit(j, "DriveLimitSwing", GetDrive(DRIVE_TYPE::SWING));
			SerializeDriveLimit(j, "DriveLimitTwist", GetDrive(DRIVE_TYPE::TWIST));
			SerializeDriveLimit(j, "DriveLimitSlerp", GetDrive(DRIVE_TYPE::SLERP));
		}

		Vec3 linear, angular;
		GetDriveVelocity(linear, angular);
		j["DriveLinearVelocity"] = linear;
		j["DriveAngularVelocity"] = angular;
		j["DrivePosition"] = GetDrivePosition();
	}
}

void D6Joint::DeserializeFromJson(Serializer* serializer, const json& j)
{
	InitJoint(PxD6JointCreate, serializer, j);

	if (!j.contains("MotionXType"))
	{
		return;
	}

	static auto DeserializeLinearLimit = [](const json& j, const char* name, LinearLimit& limit)
	{
		auto& jobj = j[name];
		limit.DeserializeFromJson(jobj);
		limit.lower = jobj["Lower"];
		limit.upper = jobj["Upper"];
	};

	static auto DeserializeSwingLimit = [](const json& j, const char* name, SwingLimit& limit)
	{
		auto& jobj = j[name];
		limit.DeserializeFromJson(jobj);
		limit.yLimitAngle = jobj["YLimitAngle"];
		limit.zLimitAngle = jobj["ZLimitAngle"];
	};

	static auto DeserializeTwistLimit = [](const json& j, const char* name, TwistLimit& limit)
	{
		auto& jobj = j[name];
		limit.DeserializeFromJson(jobj);
		limit.lowerLimit = jobj["LowerLimit"];
		limit.upperLimit = jobj["UpperLimit"];
	};

	static auto DeserializeDistanceLimit = [](const json& j, const char* name, DistanceLimit& limit)
	{
		auto& jobj = j[name];
		limit.DeserializeFromJson(jobj);
		limit.distance = jobj["Distance"];
	};

	static auto DeserializeDriveLimit = [](const json& j, const char* name, DriveLimit& limit)
	{
		auto& jobj = j[name];
		limit.stiffness = jobj["Stiffness"];
		limit.damping = jobj["Damping"];
		limit.forceLimit = jobj["ForceLimit"];
		limit.isAcceleration = jobj["IsAcceleration"];
	};

	{
		SetMotion(MOTION_AXIS::X, j["MotionXType"]);

		LinearLimit limit;
		DeserializeLinearLimit(j, "MotionXLimit", limit);
		SetLinearLimit(MOTION_AXIS::X, limit);
	}

	{
		SetMotion(MOTION_AXIS::Y, j["MotionYType"]);

		LinearLimit limit;
		DeserializeLinearLimit(j, "MotionYLimit", limit);
		SetLinearLimit(MOTION_AXIS::Y, limit);
	}

	{
		SetMotion(MOTION_AXIS::Z, j["MotionZType"]);

		LinearLimit limit;
		DeserializeLinearLimit(j, "MotionZLimit", limit);
		SetLinearLimit(MOTION_AXIS::Z, limit);
	}

	{
		SetMotion(MOTION_AXIS::SWING_Y, j["SwingYType"]);
		SetMotion(MOTION_AXIS::SWING_Z, j["SwingZType"]);

		SwingLimit limit;
		DeserializeSwingLimit(j, "SwingLimit", limit);
		SetSwingLimit(limit);
	}

	{
		SetMotion(MOTION_AXIS::TWIST_X, j["TwistXType"]);

		TwistLimit limit;
		DeserializeTwistLimit(j, "TwistLimit", limit);
		SetTwistLimit(limit);
	}

	{
		DistanceLimit limit;
		DeserializeDistanceLimit(j, "DistanceLimit", limit);
		SetDistanceLimit(limit);
	}

	if (j.contains("DriveLimitX"))
	{
		{
			DriveLimit limit;
			DeserializeDriveLimit(j, "DriveLimitX", limit);
			SetDrive(DRIVE_TYPE::X, limit); 
			DeserializeDriveLimit(j, "DriveLimitY", limit);
			SetDrive(DRIVE_TYPE::Y, limit);
			DeserializeDriveLimit(j, "DriveLimitZ", limit);
			SetDrive(DRIVE_TYPE::Z, limit);
			DeserializeDriveLimit(j, "DriveLimitSwing", limit);
			SetDrive(DRIVE_TYPE::SWING, limit);
			DeserializeDriveLimit(j, "DriveLimitTwist", limit);
			SetDrive(DRIVE_TYPE::TWIST, limit);
			DeserializeDriveLimit(j, "DriveLimitSlerp", limit);
			SetDrive(DRIVE_TYPE::SLERP, limit);
		}

		SetDriveVelocity(j["DriveLinearVelocity"], j["DriveAngularVelocity"]);
		SetDrivePosition(j["DrivePosition"]);
	}
}

Handle<ClassMetadata> D6Joint::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void D6Joint::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

//void D6Joint::Test()
//{
//	auto joint = (PxD6Joint*)m_pxJoint;
//	joint->setMotion(physx::PxD6Axis::eSWING1, physx::PxD6Motion::eLIMITED);
//	joint->setMotion(physx::PxD6Axis::eSWING2, physx::PxD6Motion::eLIMITED);
//	//joint->setMotion(physx::PxD6Axis::eTWIST, physx::PxD6Motion::eLIMITED);
//
//	joint->setSwingLimit(physx::PxJointLimitCone(PI / 4.f, PI / 4.f));
//	joint->setTwistLimit(physx::PxJointAngularLimitPair(-PI / 8.f, PI / 8.f));
//
//	joint->setDistanceLimit();
//}

void D6Joint::SetMotion(const MOTION_AXIS::ENUM& motion, const MOTION_TYPE::ENUM& type)
{
	physx::PxD6Axis::Enum axis = D6Joint_AXIS_To_Px_AXIS[motion];
	physx::PxD6Motion::Enum limit = D6Joint_MOTION_To_Px_MOTION[type];
	MAIN_SYSTEM_TASK_IMPL_COMMON_2(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST, axis, limit,
		{
			auto joint = (PxD6Joint*)self->m_pxJoint;
			joint->setMotion(axis, limit);
		}
	);
}

D6Joint::MOTION_TYPE::ENUM D6Joint::GetMotion(const MOTION_AXIS::ENUM& axis) const
{
	auto joint = (PxD6Joint*)m_pxJoint;
	physx::PxD6Axis::Enum pxAxis = D6Joint_AXIS_To_Px_AXIS[axis];
	return Px_MOTION_To_D6Joint_MOTION[joint->getMotion(pxAxis)];
}

void D6Joint::SetSwingLimit(const SwingLimit& limit)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST, limit,
		{
			auto joint = (PxD6Joint*)self->m_pxJoint;
			PxJointLimitCone pxLimit(0,0);
			*(Joint::BaseLimit*)&pxLimit = limit;
			pxLimit.yAngle = limit.yLimitAngle;
			pxLimit.zAngle = limit.zLimitAngle;
			joint->setSwingLimit(pxLimit);
		}
	);
}

D6Joint::SwingLimit D6Joint::GetSwingLimit() const
{
	auto joint = (PxD6Joint*)m_pxJoint;
	auto pxLimit = joint->getSwingLimit();

	D6Joint::SwingLimit ret;
	*(Joint::BaseLimit*)&ret = *(Joint::BaseLimit*)&pxLimit;
	ret.yLimitAngle = pxLimit.yAngle;
	ret.zLimitAngle = pxLimit.zAngle;
	return ret;
}

void D6Joint::SetTwistLimit(const TwistLimit& limit)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST, limit,
		{
			PxJointAngularLimitPair pxLimit(0,0);
			*(Joint::BaseLimit*)&pxLimit = limit;
			pxLimit.upper = limit.upperLimit;
			pxLimit.lower = limit.lowerLimit;
			auto joint = (PxD6Joint*)self->m_pxJoint;
			joint->setTwistLimit(pxLimit);
		}
	);
}

D6Joint::TwistLimit D6Joint::GetTwistLimit() const
{
	auto joint = (PxD6Joint*)m_pxJoint;
	auto pxLimit = joint->getTwistLimit();

	D6Joint::TwistLimit ret;
	*(Joint::BaseLimit*)&ret = *(Joint::BaseLimit*)&pxLimit;
	ret.upperLimit = pxLimit.upper;
	ret.lowerLimit = pxLimit.lower;
	return ret;
}

void D6Joint::SetDistanceLimit(const DistanceLimit& limit)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST, limit,
		{
			PxJointLinearLimit pxLimit(0);
			*(Joint::BaseLimit*)&pxLimit = limit;
			pxLimit.value = limit.distance;
			auto joint = (PxD6Joint*)self->m_pxJoint;
			joint->setDistanceLimit(pxLimit);
		}
	);
}

D6Joint::DistanceLimit D6Joint::GetDistanceLimit() const
{
	auto joint = (PxD6Joint*)m_pxJoint;
	auto pxLimit = joint->getDistanceLimit();

	D6Joint::DistanceLimit ret;
	*(Joint::BaseLimit*)&ret = *(Joint::BaseLimit*)&pxLimit;
	ret.distance = pxLimit.value;
	return ret;
}

void D6Joint::SetLinearLimit(const MOTION_AXIS::ENUM& axis, const LinearLimit& limit)
{
	/*auto joint = (PxD6Joint*)m_pxJoint;
	joint*/
	MAIN_SYSTEM_TASK_IMPL_COMMON_2(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST, limit, axis,
		{
			PxJointLinearLimitPair pxLimit(PhysX::Get()->GetPxPhysics()->getTolerancesScale(), 0,0);
			*(Joint::BaseLimit*)&pxLimit = limit;
			pxLimit.upper = limit.upper;
			pxLimit.lower = limit.lower;
			auto joint = (PxD6Joint*)self->m_pxJoint;
			physx::PxD6Axis::Enum pxAxis = D6Joint_AXIS_To_Px_AXIS[axis];
			joint->setLinearLimit(pxAxis, pxLimit);
		}
	);
}

D6Joint::LinearLimit D6Joint::GetLinearLimit(const MOTION_AXIS::ENUM& axis) const
{
	auto joint = (PxD6Joint*)m_pxJoint;
	auto pxLimit = joint->getLinearLimit(D6Joint_AXIS_To_Px_AXIS[axis]);

	D6Joint::LinearLimit ret;
	*(Joint::BaseLimit*)&ret = *(Joint::BaseLimit*)&pxLimit;
	ret.upper = pxLimit.upper; 
	ret.lower = pxLimit.lower;
	return ret;
}

void D6Joint::SetDrive(const DRIVE_TYPE::ENUM& type, const DriveLimit& limit)
{
	//const PxD6JointDrive drive = PxD6JointDrive(limit.stiffness, limit.damping, limit.forceLimit, limit.isAcceleration);
	//m_pxJoint->setConstraintFlag(PxConstraintFlag::eDRIVE_LIMITS_ARE_FORCES, true);
	//auto joint = (PxD6Joint*)m_pxJoint;
	MAIN_SYSTEM_TASK_IMPL_COMMON_2(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST, type, limit,
		{
			const PxD6JointDrive drive = PxD6JointDrive(limit.stiffness, limit.damping, limit.forceLimit, limit.isAcceleration);
			auto joint = (PxD6Joint*)self->m_pxJoint;
			joint->setDrive(PxD6Drive::Enum(type), drive);
		}
	);
}

D6Joint::DriveLimit D6Joint::GetDrive(const DRIVE_TYPE::ENUM& type) const
{
	auto joint = (PxD6Joint*)m_pxJoint;
	auto pxLimit = joint->getDrive(PxD6Drive::Enum(type));
	DriveLimit ret;
	ret.stiffness = pxLimit.stiffness;
	ret.damping = pxLimit.damping;
	ret.forceLimit = pxLimit.forceLimit;
	ret.isAcceleration = pxLimit.flags & PxD6JointDriveFlag::eACCELERATION;
	return ret;
}

void D6Joint::SetDrivePosition(const Transform& localOfBody0)
{
	auto pxTransform = PhysXUtils::ToPxTransform(localOfBody0);
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST, pxTransform,
		{
			auto joint = (PxD6Joint*)self->m_pxJoint;
			joint->setDrivePosition(pxTransform);
		}
	);
}

Transform D6Joint::GetDrivePosition() const
{
	auto joint = (PxD6Joint*)m_pxJoint;
	return PhysXUtils::ToTransform(joint->getDrivePosition());
}

void D6Joint::SetDriveVelocity(const Vec3& linear, const Vec3& angular)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_2(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST, linear, angular,
		{
			auto joint = (PxD6Joint*)self->m_pxJoint;
			joint->setDriveVelocity(PhysXUtils::ToPxVec3(linear), PhysXUtils::ToPxVec3(angular));
		}
	);
}

void D6Joint::GetDriveVelocity(Vec3& linear, Vec3& angular) const
{
	auto joint = (PxD6Joint*)m_pxJoint;
	joint->getDriveVelocity(reinterpret_cast<PxVec3&>(linear), reinterpret_cast<PxVec3&>(angular));
}

NAMESPACE_END