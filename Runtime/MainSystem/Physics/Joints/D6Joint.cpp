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

D6Joint::D6Joint(const Handle<RigidBody>& body0, const Transform& localFrame0, const Handle<RigidBody>& body1, const Transform& localFrame1)
{
	InitJoint(PxD6JointCreate, body0, localFrame0, body1, localFrame1);

	MAIN_SYSTEM_TASK_IMPL_COMMON_0(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST,
		{
			self->Test();
		}
	);
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
}

void D6Joint::DeserializeFromJson(Serializer* serializer, const json& j)
{
	InitJoint(PxD6JointCreate, serializer, j);
}

Handle<ClassMetadata> D6Joint::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void D6Joint::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

void D6Joint::Test()
{
	auto joint = (PxD6Joint*)m_pxJoint;
	joint->setMotion(physx::PxD6Axis::eSWING1, physx::PxD6Motion::eLIMITED);
	joint->setMotion(physx::PxD6Axis::eSWING2, physx::PxD6Motion::eLIMITED);
	//joint->setMotion(physx::PxD6Axis::eTWIST, physx::PxD6Motion::eLIMITED);

	joint->setSwingLimit(physx::PxJointLimitCone(PI / 4.f, PI / 4.f));
	//joint->setTwistLimit(physx::PxJointAngularLimitPair(-PI / 8.f, PI / 8.f));
}

NAMESPACE_END