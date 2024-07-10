#pragma once

#include "PhysicsComponent.h"

NAMESPACE_BEGIN

class PhysicsShape;

// same as PxPairFlag
struct PhysicsCollisionPairFlag
{
	enum Enum
	{
		SOLVE_CONTACT = (1 << 0),
		MODIFY_CONTACTS = (1 << 1),
		NOTIFY_TOUCH_FOUND = (1 << 2),
		NOTIFY_TOUCH_PERSISTS = (1 << 3),
		NOTIFY_TOUCH_LOST = (1 << 4),
		NOTIFY_TOUCH_CCD = (1 << 5),
		NOTIFY_THRESHOLD_FORCE_FOUND = (1 << 6),
		NOTIFY_THRESHOLD_FORCE_PERSISTS = (1 << 7),
		NOTIFY_THRESHOLD_FORCE_LOST = (1 << 8),
		NOTIFY_CONTACT_POINTS = (1 << 9),
		DETECT_DISCRETE_CONTACT = (1 << 10),
		DETECT_CCD_CONTACT = (1 << 11),
		PRE_SOLVER_VELOCITY = (1 << 12),
		POST_SOLVER_VELOCITY = (1 << 13),
		CONTACT_EVENT_POSE = (1 << 14),
		NEXT_FREE = (1 << 15),
		CONTACT_DEFAULT = SOLVE_CONTACT | DETECT_DISCRETE_CONTACT,
		TRIGGER_DEFAULT = NOTIFY_TOUCH_FOUND | NOTIFY_TOUCH_LOST | DETECT_DISCRETE_CONTACT
	};
};

class RigidBody : public PhysicsComponent
{
public:
	using ContactReportFilterCallback = void (*)(
		GameObject* self, PhysicsShape* selfShape, PHYSICS_TYPE selfType,
		GameObject* another, PhysicsShape* anotherShape, PHYSICS_TYPE anotherType,
		size_t& pairFlags
		);

private:
	friend class PhysXSimulationFilterCallback;
	ContactReportFilterCallback m_contactFilterCallback = nullptr;

protected:
	//Mat4 m_lastGlobalTransform;

	std::vector<SharedPtr<PhysicsShape>> m_shapes;

	inline RigidBody() {};

public:
	void OnTransformChanged() override;

protected:
	virtual void OnDrawDebug() override;

	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;

public:
	void SetContactFilterCallback(ContactReportFilterCallback callback);

};

NAMESPACE_END