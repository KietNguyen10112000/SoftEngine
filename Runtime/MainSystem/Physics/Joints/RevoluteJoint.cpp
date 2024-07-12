#include "RevoluteJoint.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "../Components/RigidBody.h"

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
}

void RevoluteJoint::DeserializeFromJson(Serializer* serializer, const json& j)
{
	InitJoint(PxRevoluteJointCreate, serializer, j);
}

Handle<ClassMetadata> RevoluteJoint::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void RevoluteJoint::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END