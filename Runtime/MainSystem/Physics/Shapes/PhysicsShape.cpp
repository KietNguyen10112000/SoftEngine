#include "PhysicsShape.h"

#include "PxPhysicsAPI.h"

#include "PhysX/Utils.h"

#include "../Materials/PhysicsMaterial.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsShape::~PhysicsShape()
{
	if (m_pxShape)
		m_pxShape->release();

	m_pxShape = nullptr;
}

void PhysicsShape::SetTransform(const Transform& transform)
{
	PxTransform pxTransform;
	pxTransform.p = reinterpret_cast<PxVec3&>(transform.GetPosition());
	pxTransform.q = PxQuat(transform.GetRotation().x, transform.GetRotation().y, transform.GetRotation().z, transform.GetRotation().w);
	m_pxShape->setLocalPose(pxTransform);
}

SharedPtr<PhysicsMaterial> PhysicsShape::GetDeserializedMaterial(Serializer* serializer, const json& j)
{
	SharedPtr<PhysicsMaterial> material;
	serializer->Deserialize(j["Meterial"], material);
	return material;
}

void PhysicsShape::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void PhysicsShape::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void PhysicsShape::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void PhysicsShape::SerializeToJson(Serializer* serializer, json& j) const
{
	j["Transform"] = PhysXUtils::ToTransform(m_pxShape->getLocalPose());
	j["Meterial"] = serializer->Serialize(m_meterial);
}

void PhysicsShape::DeserializeFromJson(Serializer* serializer, const json& j)
{
	serializer->Deserialize(j["Meterial"], m_meterial);
	m_pxShape->setLocalPose(PhysXUtils::ToPxTransform(j["Transform"]));
}

NAMESPACE_END