#pragma once

#include "PhysicsShape.h"

#include "Math/Math.h"

#include "Core/Memory/SmartPointers.h"

NAMESPACE_BEGIN

class PhysicsMaterial;

class PhysicsShapeCapsule : public PhysicsShape
{
private:
	SERIALIZABLE_CLASS(PhysicsShapeCapsule, SERIALIZABLE_MEM_SHARED);

	friend class CharacterControllerCapsule;
	PhysicsShapeCapsule(void* pxShape, float height, float radius, const SharedPtr<PhysicsMaterial>& material);

	inline static SharedPtr<PhysicsShapeCapsule> 
		MakeDummy(void* pxShape, float height, float radius, const SharedPtr<PhysicsMaterial>& material)
	{
		struct make_shared_enabler : public PhysicsShapeCapsule 
		{
			make_shared_enabler(void* pxShape, float height, float radius, const SharedPtr<PhysicsMaterial>& material)
				: PhysicsShapeCapsule(pxShape, height, radius, material) {};
		};
		return std::make_shared<make_shared_enabler>(pxShape, height, radius, material);
	}

public:
	inline PhysicsShapeCapsule() {};
	PhysicsShapeCapsule(float height, float radius, const SharedPtr<PhysicsMaterial>& material);


	inline virtual PHYSICS_SHAPE_TYPE GetType() const
	{
		return PHYSICS_SHAPE_TYPE_CAPSULE;
	};

protected:
	// Inherited via PhysicsShape
	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;
	Handle<ClassMetadata> GetMetadata(size_t sign) override;
	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

public:
	void SetHeight(float h);
	float GetHeight() const;

	void SetRadius(float r);
	float GetRadius() const;

};

NAMESPACE_END