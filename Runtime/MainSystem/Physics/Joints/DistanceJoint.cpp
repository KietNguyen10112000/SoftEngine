#include "DistanceJoint.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "../Components/RigidBody.h"

using namespace physx;

NAMESPACE_BEGIN

DistanceJoint::DistanceJoint(const Handle<RigidBody>& body0, const Transform& localFrame0, const Handle<RigidBody>& body1, const Transform& localFrame1)
{
	InitJoint(PxDistanceJointCreate, body0, localFrame0, body1, localFrame1);
}

void DistanceJoint::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void DistanceJoint::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void DistanceJoint::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void DistanceJoint::SerializeToJson(Serializer* serializer, json& j) const
{
	Joint::SerializeToJson(serializer, j);
}

void DistanceJoint::DeserializeFromJson(Serializer* serializer, const json& j)
{
	InitJoint(PxDistanceJointCreate, serializer, j);
}

Handle<ClassMetadata> DistanceJoint::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void DistanceJoint::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END