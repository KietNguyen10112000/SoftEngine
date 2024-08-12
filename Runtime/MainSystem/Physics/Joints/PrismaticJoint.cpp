#include "PrismaticJoint.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "../Components/RigidBody.h"

using namespace physx;

NAMESPACE_BEGIN

PrismaticJoint::PrismaticJoint(const Handle<RigidBody>& body0, const Transform& localFrame0, const Handle<RigidBody>& body1, const Transform& localFrame1)
{
	InitJoint(PxPrismaticJointCreate, body0, localFrame0, body1, localFrame1);
}

void PrismaticJoint::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void PrismaticJoint::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void PrismaticJoint::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void PrismaticJoint::SerializeToJson(Serializer* serializer, json& j) const
{
	Joint::SerializeToJson(serializer, j);
}

void PrismaticJoint::DeserializeFromJson(Serializer* serializer, const json& j)
{
	InitJoint(PxPrismaticJointCreate, serializer, j);
}

Handle<ClassMetadata> PrismaticJoint::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void PrismaticJoint::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END