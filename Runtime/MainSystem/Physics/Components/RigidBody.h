#pragma once

#include "PhysicsComponent.h"

NAMESPACE_BEGIN

class PhysicsShape;
class Joint;

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

class API RigidBody : public PhysicsComponent
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

	friend class Joint;
	MAIN_SYSTEM_FRIEND_CLASSES();

protected:
	//Mat4 m_lastGlobalTransform;

	Array<Handle<Joint>> m_joints;
	std::vector<SharedPtr<PhysicsShape>> m_shapes;

	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_joints);
	}

	inline RigidBody() {};

private:
	void SetupCollisionStruct();

public:
	void OnTransformChanged() override;

protected:
	virtual void OnDrawDebug() override;
	virtual void OnPhysicsFlagSetted(PHYSICS_FLAG flag, bool value) override;

	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;
	Handle<ClassMetadata> GetMetadata(size_t sign) override;

private:
	void AddShapeImpl(const SharedPtr<PhysicsShape>& shape);
	void RemoveShapeImpl(PhysicsShape* shape);

public:
	void SetContactFilterCallback(ContactReportFilterCallback callback);

	void AddShape(const SharedPtr<PhysicsShape>& shape);
	void RemoveShape(PhysicsShape* shape);

	void ScaleBy(float scale);

	void SetCollisionMaskForAllShapes(uint32_t mask);
	void SetFamilyNoCollideForAllShapes(bool enable);

	// set mask all rigid bodies belong to this object tree
	static void SetCollisionMaskForGameObject(GameObject* obj, uint32_t mask);

	// set FamilyNoCollide flag for all rigid bodies belong to this object tree
	// enable -> all rigid bodies belong to this object tree will not collide each other
	static void SetFamilyNoCollideForGameObject(GameObject* obj, bool enable);

	inline PhysicsShape* GetShape(ID index) const
	{
		return m_shapes[index].get();
	}

	inline size_t GetShapesCount() const
	{
		return m_shapes.size();
	}

	inline const Handle<Joint>& GetJoint(ID index) const
	{
		return m_joints[index];
	}

	inline size_t GetJointsCount() const
	{
		return m_joints.size();
	}

};

NAMESPACE_END