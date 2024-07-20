#include "PhysicsShapeSphere.h"

#include "PhysX/PhysX.h"

#include "../Materials/PhysicsMaterial.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsShapeSphere::PhysicsShapeSphere(float radius, const SharedPtr<PhysicsMaterial>& material)
{
	auto physics = PhysX::Get()->GetPxPhysics();
	auto& m = *(material->m_pxMaterial);
	m_pxShape = physics->createShape(PxSphereGeometry(radius), m, false);

	m_pxShape->userData = this;
	m_meterial = material;
}

void PhysicsShapeSphere::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void PhysicsShapeSphere::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void PhysicsShapeSphere::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void PhysicsShapeSphere::SerializeToJson(Serializer* serializer, json& j) const
{
	auto geo = (PxSphereGeometry*)&m_pxShape->getGeometry();
	j["Redius"] = geo->radius;
	PhysicsShape::SerializeToJson(serializer, j);
}

void PhysicsShapeSphere::DeserializeFromJson(Serializer* serializer, const json& j)
{
	assert(m_pxShape == nullptr);
	this->~PhysicsShapeSphere();
	new (this) PhysicsShapeSphere(j["Radius"], GetDeserializedMaterial(serializer, j));
	PhysicsShape::DeserializeFromJson(serializer, j);
}

Handle<ClassMetadata> PhysicsShapeSphere::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void PhysicsShapeSphere::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END