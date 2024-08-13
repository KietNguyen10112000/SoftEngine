#include "RevoluteJoint.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "../Components/RigidBody.h"

#include "MainSystem/MainSystemTaskPacking.h"
#include "MainSystem/Physics/PhysicsSystem.h"
#include "Scene/GameObject.h"

using namespace physx;

NAMESPACE_BEGIN

RevoluteJoint::RevoluteJoint(const Handle<RigidBody>& body0, const Transform& localFrame0, const Handle<RigidBody>& body1, const Transform& localFrame1)
{
	InitJoint(PxRevoluteJointCreate, body0, localFrame0, body1, localFrame1);
}

void RevoluteJoint::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void RevoluteJoint::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void RevoluteJoint::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void RevoluteJoint::SerializeToJson(Serializer* serializer, json& j) const
{
	Joint::SerializeToJson(serializer, j);

	auto joint = (PxRevoluteJoint*)m_pxJoint;
	auto limit = joint->getLimit();
	auto p = (Joint::BaseLimit*)&limit;
	p->SerializeToJson(j);
	j["LowerLimit"] = limit.lower;
	j["UpperLimit"] = limit.upper;
	j["EnableLimit"] = IsEnableLimit();
}

void RevoluteJoint::DeserializeFromJson(Serializer* serializer, const json& j)
{
	InitJoint(PxRevoluteJointCreate, serializer, j);

	if (j.contains("LowerLimit"))
	{
		auto joint = (PxRevoluteJoint*)m_pxJoint;
		PxJointAngularLimitPair limit = { 0,0 };
		auto p = (Joint::BaseLimit*)&limit;
		p->DeserializeFromJson(j);
		limit.lower = j["LowerLimit"];
		limit.upper = j["UpperLimit"];
		joint->setLimit(limit);

		bool enableLimit = j["EnableLimit"];
		if (enableLimit)
		{
			SetEnableLimit(enableLimit);
		}
	}
}

Handle<ClassMetadata> RevoluteJoint::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void RevoluteJoint::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

void RevoluteJoint::SetLimit(const RevoluteJoint::Limit& limit)
{
	//auto joint = (PxRevoluteJoint*)m_pxJoint;
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST, limit,
		{
			auto joint = (PxRevoluteJoint*)self->m_pxJoint;
			PxJointAngularLimitPair pxLimit = PxJointAngularLimitPair(limit.lowerLimit, limit.upperLimit);
			*(PxJointLimitParameters*)&pxLimit = reinterpret_cast<const PxJointLimitParameters&>(limit);
			joint->setLimit(pxLimit);
		}
	);
}

RevoluteJoint::Limit RevoluteJoint::GetLimit() const
{
	auto joint = (PxRevoluteJoint*)m_pxJoint;
	auto limit = joint->getLimit();
	RevoluteJoint::Limit ret;
	ret.upperLimit = limit.upper;
	ret.lowerLimit = limit.lower;
	*(PxJointLimitParameters*)&ret = reinterpret_cast<const PxJointLimitParameters&>(limit);
	return ret;
}

void RevoluteJoint::SetEnableLimit(bool enable)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST, enable,
		{
			auto joint = (PxRevoluteJoint*)self->m_pxJoint;
			joint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, enable);
		}
	);
}

bool RevoluteJoint::IsEnableLimit() const
{
	auto joint = (PxRevoluteJoint*)m_pxJoint;
	return joint->getRevoluteJointFlags() & PxRevoluteJointFlag::eLIMIT_ENABLED;
}

void RevoluteJoint::SetDriveVelocity(float v)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST, v,
		{
			auto joint = (PxRevoluteJoint*)self->m_pxJoint;
			joint->setDriveVelocity(v);
		}
	);
}

float RevoluteJoint::GetDriveVelocity() const
{
	auto joint = (PxRevoluteJoint*)m_pxJoint;
	return joint->getDriveVelocity();
}

void RevoluteJoint::SetEnableDriveVelocity(bool enable)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST, enable,
		{
			auto joint = (PxRevoluteJoint*)self->m_pxJoint;
			joint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, enable);
			self->WakeUpBodies();
		}
	);
}

bool RevoluteJoint::IsEnableDriveVelocity() const
{
	auto joint = (PxRevoluteJoint*)m_pxJoint;
	return joint->getRevoluteJointFlags() & PxRevoluteJointFlag::eDRIVE_ENABLED;
}

void RevoluteJoint::SetDriveForceLimit(float v)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST, v,
		{
			auto joint = (PxRevoluteJoint*)self->m_pxJoint;
			joint->setDriveForceLimit(v);
		}
	);
}

float RevoluteJoint::GetDriveForceLimit() const
{
	auto joint = (PxRevoluteJoint*)m_pxJoint;
	return joint->getDriveForceLimit();
}

NAMESPACE_END