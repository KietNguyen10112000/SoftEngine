#pragma once

#include "Core/Memory/SmartPointers.h"

#include "PhysicsComponent.h"
#include "RigidBody.h"

NAMESPACE_BEGIN

class PhysicsShape;

class RigidBodyDynamic : public RigidBody
{
public:
	byte m_isKinematic = 0;

	COMPONENT_CLASS(RigidBodyDynamic);

	RigidBodyDynamic(const SharedPtr<PhysicsShape>& shape);
	~RigidBodyDynamic();

private:
	static void TransformContributor(GameObject* object, Transform& local, Mat4& global, void* self);

protected:
	virtual void OnPhysicsTransformChanged() override;

public:
	inline virtual PHYSICS_TYPE GetPhysicsType() const 
	{
		return PHYSICS_TYPE_RIGID_BODY_DYNAMIC;
	};

public:
	// Inherited via RigidBody
	void Serialize(Serializer* serializer) override;

	void Deserialize(Serializer* serializer) override;

	void CleanUp() override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

	void OnComponentAdded() override;

	void OnComponentRemoved() override;

	void OnTransformChanged() override;

	AABox GetGlobalAABB() override;

public:
	void SetMass(float mass);

	void SetKinematic(bool enable);

};

NAMESPACE_END