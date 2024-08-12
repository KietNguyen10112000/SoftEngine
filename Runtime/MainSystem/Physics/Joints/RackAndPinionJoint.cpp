#include "RackAndPinionJoint.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "../Components/RigidBody.h"

using namespace physx;

NAMESPACE_BEGIN

RackAndPinionJoint::RackAndPinionJoint(const Handle<RigidBody>& body0, const Transform& localFrame0, const Handle<RigidBody>& body1, const Transform& localFrame1)
{
	InitJoint(PxRackAndPinionJointCreate, body0, localFrame0, body1, localFrame1);
}

void RackAndPinionJoint::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void RackAndPinionJoint::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void RackAndPinionJoint::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void RackAndPinionJoint::SerializeToJson(Serializer* serializer, json& j) const
{
	Joint::SerializeToJson(serializer, j);
}

void RackAndPinionJoint::DeserializeFromJson(Serializer* serializer, const json& j)
{
	InitJoint(PxRackAndPinionJointCreate, serializer, j);
}

Handle<ClassMetadata> RackAndPinionJoint::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void RackAndPinionJoint::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END