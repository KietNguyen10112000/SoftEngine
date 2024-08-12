#include "SphericalJoint.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "../Components/RigidBody.h"

#include "MainSystem/MainSystemTaskPacking.h"
#include "MainSystem/Physics/PhysicsSystem.h"
#include "Scene/GameObject.h"

using namespace physx;

NAMESPACE_BEGIN

SphericalJoint::SphericalJoint(const Handle<RigidBody>& body0, const Transform& localFrame0, const Handle<RigidBody>& body1, const Transform& localFrame1)
{
	InitJoint(PxSphericalJointCreate, body0, localFrame0, body1, localFrame1);
}

void SphericalJoint::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void SphericalJoint::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void SphericalJoint::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void SphericalJoint::SerializeToJson(Serializer* serializer, json& j) const
{
	Joint::SerializeToJson(serializer, j);

	auto joint = (PxSphericalJoint*)m_pxJoint;
	auto limit = joint->getLimitCone();
	auto p = (Joint::BaseLimit*)&limit;
	p->SerializeToJson(j);
	j["YAngleLimit"] = limit.yAngle;
	j["ZAngleLimit"] = limit.zAngle;
}

void SphericalJoint::DeserializeFromJson(Serializer* serializer, const json& j)
{
	InitJoint(PxSphericalJointCreate, serializer, j);

	if (j.contains("YAngleLimit"))
	{
		auto joint = (PxSphericalJoint*)m_pxJoint;
		PxJointLimitCone limit = { 0,0 };
		auto p = (Joint::BaseLimit*)&limit;
		p->DeserializeFromJson(j);
		limit.yAngle = j["YAngleLimit"];
		limit.zAngle = j["ZAngleLimit"];
		joint->setLimitCone(limit);
	}
}

Handle<ClassMetadata> SphericalJoint::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void SphericalJoint::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

void SphericalJoint::SetLimit(const SphericalJoint::Limit& limit)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(m_body0.Get(),
		PhysicsSystem, AsyncTaskRunnerST, limit,
		{
			auto joint = (PxSphericalJoint*)self->m_pxJoint;
			PxJointLimitCone pxLimit = PxJointLimitCone(limit.yLimitAngle,limit.zLimitAngle);
			*(PxJointLimitParameters*)&pxLimit = reinterpret_cast<const PxJointLimitParameters&>(limit);
			joint->setLimitCone(pxLimit);
		}
	);
}

SphericalJoint::Limit SphericalJoint::GetLimit() const
{
	auto joint = (PxSphericalJoint*)m_pxJoint;
	auto limit = joint->getLimitCone();
	SphericalJoint::Limit ret;
	ret.yLimitAngle = limit.yAngle;
	ret.zLimitAngle = limit.zAngle;
	*(PxJointLimitParameters*)&ret = reinterpret_cast<const PxJointLimitParameters&>(limit);
	return ret;
}

NAMESPACE_END