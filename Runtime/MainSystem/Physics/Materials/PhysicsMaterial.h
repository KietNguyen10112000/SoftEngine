#pragma once

#include "Core/TypeDef.h"

#include "Common/Base/Serializable.h"

#include "../PhysicsClasses.h"

namespace physx
{
	class PxMaterial;
}

NAMESPACE_BEGIN

class PhysicsMaterial : public Serializable
{
	SERIALIZABLE_CLASS(PhysicsMaterial, SERIALIZABLE_MEM_SHARED);
protected:

	PHYSICS_FRIEND_CLASSES();

	physx::PxMaterial* m_pxMaterial = nullptr;

public:
	inline PhysicsMaterial() {};
	PhysicsMaterial(float staticFriction, float dynamicFriction, float restitution);
	virtual ~PhysicsMaterial();

public:
	float GetStaticFriction();
	void SetStaticFriction(float staticFriction);

	float GetDynamicFriction();
	void SetDynamicFriction(float dynamicFriction);

	float GetRestitution();
	void SetRestitution(float restitution);


	// Inherited via Serializable
	void CloneFrom(Serializer* serializer, Serializable* another) override;

	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;

	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;

	void SerializeToJson(Serializer* serializer, json& j) const override;

	void DeserializeFromJson(Serializer* serializer, const json& j) override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

};

NAMESPACE_END