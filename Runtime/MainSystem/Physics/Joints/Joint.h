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

class API Joint : public Serializable
{
protected:
	MAIN_SYSTEM_FRIEND_CLASSES();
	friend class PhysXSimulationCallback;

	physx::PxJoint* m_pxJoint = nullptr;

private:
	Handle<RigidBody> m_body0 = nullptr;
	Handle<RigidBody> m_body1 = nullptr;
	uint32_t m_idx0 = uint32_t(INVALID_ID);
	uint32_t m_idx1 = uint32_t(INVALID_ID);
	RigidBody* m_component = nullptr;

	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_body0);
		tracer->Trace(m_body1);
	}

public:
	struct BaseLimit
	{
		float restitution = 0.0f;
		float bounceThreshold = 0.0f;
		float stiffness = 0.0f;
		float damping = 0.0f;

		void SerializeToJson(json& j) const;
		void DeserializeFromJson(const json& j);
	};

private:
	void CommitJointToBodies();
	void RemoveJointFromBodies();

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

protected:
	void WakeUpBodies();

	inline auto* GetComponent()
	{
		return m_component;
	}

public:
	bool IsBroken() const;
	void Break();

	void SetBreakForce(float force, float torque);
	float GetBreakForce() const;
	float GetBreakTorque() const;

	Transform GetLocalFrame(RigidBody* body) const;
	void SetLocalFrame(RigidBody* body, const Transform& transform);

	Transform GetGlobalTransform() const;

	inline RigidBody* GetBody0()
	{
		return m_body0;
	}

	inline RigidBody* GetBody1()
	{
		return m_body1;
	}

	inline RigidBody* GetAnotherBody(RigidBody* body)
	{
		return body == m_body0 ? m_body1 : m_body0;
	}
};

NAMESPACE_END