#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"

#include "Math/Math.h"

#include "Common/Base/Serializable.h"

namespace physx
{
	class PxJoint;
	class PxActor;
}

NAMESPACE_BEGIN

class PhysicsComponent;
class RigidBody;

class Joint : public Serializable
{
protected:
	MAIN_SYSTEM_FRIEND_CLASSES();

	physx::PxJoint* m_pxJoint = nullptr;
	Handle<RigidBody> m_body0 = nullptr;
	Handle<RigidBody> m_body1 = nullptr;
	uint32_t m_idx0 = uint32_t(INVALID_ID);
	uint32_t m_idx1 = uint32_t(INVALID_ID);

	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_body0);
		tracer->Trace(m_body1);
	}

private:
	void CommitJointToBodies();

protected:
	void InitJoint(void* pxInitFunc, const Handle<RigidBody>& body0, const Transform& localFrame0, const Handle<RigidBody>& body1, const Transform& localFrame1);
	void InitJoint(void* pxInitFunc, Serializer* serializer, const json& j);

	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;

public:
	virtual ~Joint();

};

NAMESPACE_END