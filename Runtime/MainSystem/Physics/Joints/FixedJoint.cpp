#include "FixedJoint.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "../Components/RigidBody.h"

using namespace physx;

NAMESPACE_BEGIN

FixedJoint::FixedJoint(const Handle<RigidBody>& body0, const Transform& localFrame0, const Handle<RigidBody>& body1, const Transform& localFrame1)
{
	InitJoint(PxFixedJointCreate, body0, localFrame0, body1, localFrame1);
}

void FixedJoint::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void FixedJoint::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void FixedJoint::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void FixedJoint::SerializeToJson(Serializer* serializer, json& j) const
{
	Joint::SerializeToJson(serializer, j);
}

void FixedJoint::DeserializeFromJson(Serializer* serializer, const json& j)
{
	InitJoint(PxFixedJointCreate, serializer, j);
}

Handle<ClassMetadata> FixedJoint::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void FixedJoint::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END