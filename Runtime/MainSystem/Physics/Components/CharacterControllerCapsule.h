#pragma once

#include "CharacterController.h"

#include "Core/Memory/SmartPointers.h"

#include "../Materials/PhysicsMaterial.h"

NAMESPACE_BEGIN

class PhysicsShape;

struct CharacterControllerCapsuleDesc
{
	Capsule capsule = {};
	SharedPtr<PhysicsMaterial> material;
};

class CharacterControllerCapsule : public CharacterController
{
public:
	COMPONENT_CLASS(CharacterControllerCapsule);

	SharedPtr<PhysicsShape> m_shape;

	CharacterControllerCapsuleDesc m_desc;

	inline CharacterControllerCapsule() {};
	CharacterControllerCapsule(const CharacterControllerCapsuleDesc& desc);

private:
	void InitializeCCT(Scene* scene);

protected:
	virtual void OnDrawDebug() override;

	// Inherited via CharacterController
	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;
	Handle<ClassMetadata> GetMetadata(size_t sign) override;
	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

public:
	void OnComponentAdded() override;

	void OnComponentRemoved() override;

	AABox GetGlobalAABB() override;

};

NAMESPACE_END