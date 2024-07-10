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

class PhysicsShape : public Serializable, public std::enable_shared_from_this<PhysicsShape>
{
protected:
	friend class PhysicsSystem;
	PHYSICS_FRIEND_CLASSES();

	physx::PxShape* m_pxShape = nullptr;

	SharedPtr<PhysicsMaterial> m_meterial;

	// 0: not in frame
	// 1: in frame
	// 2: in frame but lost contact
	byte m_inFrameType[8] = {};

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

};

NAMESPACE_END