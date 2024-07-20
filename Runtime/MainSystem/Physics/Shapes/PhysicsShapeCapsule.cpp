#include "PhysicsShapeCapsule.h"

#include "PhysX/PhysX.h"

#include "../Materials/PhysicsMaterial.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsShapeCapsule::PhysicsShapeCapsule(void* pxShape, float h, float r, const SharedPtr<PhysicsMaterial>& material)
{
	m_pxShape = (PxShape*)pxShape;
	m_pxShape->acquireReference();
	m_pxShape->userData = this;
	m_meterial = material;
}

PhysicsShapeCapsule::PhysicsShapeCapsule(float h, float r, const SharedPtr<PhysicsMaterial>& material)
{
	auto physics = PhysX::Get()->GetPxPhysics();
	auto& m = *(material->m_pxMaterial);
	m_pxShape = physics->createShape(PxCapsuleGeometry(r, h / 2.0f), m, false);

	m_pxShape->userData = this;
	m_meterial = material;
}

void PhysicsShapeCapsule::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void PhysicsShapeCapsule::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void PhysicsShapeCapsule::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void PhysicsShapeCapsule::SerializeToJson(Serializer* serializer, json& j) const
{
	auto geo = (PxCapsuleGeometry*)&m_pxShape->getGeometry();
	j["Height"] = geo->halfHeight * 2.0f;
	j["Radius"] = geo->radius;
	PhysicsShape::SerializeToJson(serializer, j);
}

void PhysicsShapeCapsule::DeserializeFromJson(Serializer* serializer, const json& j)
{
	assert(m_pxShape == nullptr);
	this->~PhysicsShapeCapsule();
	new (this) PhysicsShapeCapsule(j["Height"], j["Radius"], GetDeserializedMaterial(serializer, j));
	PhysicsShape::DeserializeFromJson(serializer, j);
}

Handle<ClassMetadata> PhysicsShapeCapsule::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void PhysicsShapeCapsule::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END