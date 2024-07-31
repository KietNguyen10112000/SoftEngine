#include "PhysicsMaterial.h"

#include "PxPhysicsAPI.h"

#include "PhysX/PhysX.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsMaterial::PhysicsMaterial(float staticFriction, float dynamicFriction, float restitution)
{
	m_pxMaterial = PhysX::Get()->GetPxPhysics()->createMaterial(staticFriction, dynamicFriction, restitution);
}

PhysicsMaterial::~PhysicsMaterial()
{
	if (m_pxMaterial)
		m_pxMaterial->release();

	m_pxMaterial = nullptr;
}

float PhysicsMaterial::GetStaticFriction()
{
	return m_pxMaterial->getStaticFriction();
}

void PhysicsMaterial::SetStaticFriction(float staticFriction)
{
	m_pxMaterial->setStaticFriction(staticFriction);
}

float PhysicsMaterial::GetDynamicFriction()
{
	return m_pxMaterial->getDynamicFriction();
}

void PhysicsMaterial::SetDynamicFriction(float dynamicFriction)
{
	m_pxMaterial->setDynamicFriction(dynamicFriction);
}

float PhysicsMaterial::GetRestitution()
{
	return m_pxMaterial->getRestitution();
}

void PhysicsMaterial::SetRestitution(float restitution)
{
	m_pxMaterial->setRestitution(restitution);
}

void PhysicsMaterial::CloneFrom(Serializer* serializer, Serializable* another)
{
	this->~PhysicsMaterial();

	auto src = (PhysicsMaterial*)another;

	new (this) PhysicsMaterial(
		src->m_pxMaterial->getStaticFriction(), 
		src->m_pxMaterial->getDynamicFriction(), 
		src->m_pxMaterial->getRestitution()
	);
}

void PhysicsMaterial::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void PhysicsMaterial::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void PhysicsMaterial::SerializeToJson(Serializer* serializer, json& j) const
{
	j["StaticFriction"] = m_pxMaterial->getStaticFriction();
	j["DynamicFriction"] = m_pxMaterial->getDynamicFriction();
	j["Restitution"] = m_pxMaterial->getRestitution();
}

void PhysicsMaterial::DeserializeFromJson(Serializer* serializer, const json& j)
{
	assert(m_pxMaterial == nullptr);

	new (this) PhysicsMaterial(j["StaticFriction"], j["DynamicFriction"], j["Restitution"]);
}

Handle<ClassMetadata> PhysicsMaterial::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void PhysicsMaterial::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END