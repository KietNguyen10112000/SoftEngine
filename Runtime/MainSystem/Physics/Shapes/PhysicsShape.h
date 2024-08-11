#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/SmartPointers.h"

#include "Common/Base/Serializable.h"

#include "PHYSICS_SHAPE_TYPE.h"

#include "../PhysicsClasses.h"

namespace physx
{
	class PxShape;
}

NAMESPACE_BEGIN

class PhysicsMaterial;
class RigidBody;

class API PhysicsShape : public Serializable, public std::enable_shared_from_this<PhysicsShape>
{
	SERIALIZABLE_CLASS(PhysicsShape, SERIALIZABLE_MEM_SHARED);
protected:
	friend class PhysicsSystem;
	friend class PhysicsShapeUtils;
	PHYSICS_FRIEND_CLASSES();

	physx::PxShape* m_pxShape = nullptr;

	SharedPtr<PhysicsMaterial> m_meterial;

	// 0: not in frame
	// 1: in frame
	// 2: in frame but lost contact
	byte m_inFrameType[8] = {};

	RigidBody* m_attachedRigidBody = nullptr;
	size_t m_attachedRigidBodyCount = 0;

public:
	virtual ~PhysicsShape();

	void SetTransform(const Transform& transform);

private:
	SharedPtr<PhysicsMaterial> GetDeserializedMaterial(Serializer* serializer, const json& j);

public:
	virtual PHYSICS_SHAPE_TYPE GetType() const = 0;

	inline const auto& GetFirstMaterial()
	{
		return m_meterial;
	}

	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;
	Handle<ClassMetadata> GetMetadata(size_t sign) override;

public:
	void SetLocalTransform(const Transform& transform);
	Transform GetLocalTransform() const;

	void SetCollisionMask(uint32_t mask);
	uint32_t GetCollisionMask() const;

	bool IsEnableFamilyNoCollide();
	void SetFamilyNoCollide(bool enable);

	virtual void ScaleBy(float scale) = 0;

};

NAMESPACE_END