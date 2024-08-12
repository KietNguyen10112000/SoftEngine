#include "GearJoint.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "../Components/RigidBody.h"

using namespace physx;

NAMESPACE_BEGIN

GearJoint::GearJoint(const Handle<RigidBody>& body0, const Transform& localFrame0, const Handle<RigidBody>& body1, const Transform& localFrame1)
{
	InitJoint(PxGearJointCreate, body0, localFrame0, body1, localFrame1);
}

void GearJoint::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void GearJoint::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void GearJoint::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void GearJoint::SerializeToJson(Serializer* serializer, json& j) const
{
	Joint::SerializeToJson(serializer, j);
}

void GearJoint::DeserializeFromJson(Serializer* serializer, const json& j)
{
	InitJoint(PxGearJointCreate, serializer, j);
}

Handle<ClassMetadata> GearJoint::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void GearJoint::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END