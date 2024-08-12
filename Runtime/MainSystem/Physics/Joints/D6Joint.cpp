#include "D6Joint.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "../Components/RigidBody.h"

using namespace physx;

NAMESPACE_BEGIN

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

NAMESPACE_END