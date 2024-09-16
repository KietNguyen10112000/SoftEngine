#pragma once

#include "CharacterController.h"

#include "Core/Memory/SmartPointers.h"

#include "../Materials/PhysicsMaterial.h"

NAMESPACE_BEGIN

class PhysicsShape;

struct CharacterControllerCapsuleDesc : public CharacterControllerDesc
{
	Capsule capsule = {};
};

class API CharacterControllerCapsule : public CharacterController
{
public:
	COMPONENT_CLASS(CharacterControllerCapsule);

	struct CLIMB_MODE
	{
		enum ENUM
		{
			DEFAULT,
			CONSTRAINED_STEP_OFFSET
		};
	};

	SharedPtr<PhysicsShape> m_shape;

	CharacterControllerCapsuleDesc m_desc;

	CharacterControllerCapsule();
	CharacterControllerCapsule(const CharacterControllerCapsuleDesc& desc);

	~CharacterControllerCapsule();

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

public:
	void CCTSetRadius(float radius);
	float CCTGetRadius() const;

	void CCTSetHeight(float height);
	float CCTGetHeight() const;

	void CCTSetClimbMode(CLIMB_MODE::ENUM mode);
	CLIMB_MODE::ENUM CCTGetClimbMode() const;

};

NAMESPACE_END