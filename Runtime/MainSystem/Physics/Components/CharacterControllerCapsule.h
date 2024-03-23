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

	CharacterControllerCapsule(Scene* scene, const CharacterControllerCapsuleDesc& desc);

protected:
	virtual void OnDrawDebug() override;

public:
	// Inherited via CharacterController
	void Serialize(Serializer* serializer) override;

	void Deserialize(Serializer* serializer) override;

	void CleanUp() override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

	void OnComponentAdded() override;

	void OnComponentRemoved() override;

	AABox GetGlobalAABB() override;

};

NAMESPACE_END