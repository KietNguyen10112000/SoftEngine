#pragma once

#include "BulletLinkLibraries.h"

#include "Engine/Physics/PhysicsEngine.h"

#include "btBulletDynamicsCommon.h"

class BulletEngine : public PhysicsEngine
{
public:
	btDefaultCollisionConfiguration* m_collisionConfiguration = 0;
	btCollisionDispatcher* m_dispatcher = 0;

	btBroadphaseInterface* m_broadphase = 0;
	btSequentialImpulseConstraintSolver* m_solver = 0;

	btDiscreteDynamicsWorld* m_dynamicsWorld = 0;

public:
	BulletEngine();
	~BulletEngine();

public:
	// Inherited via PhysicsEngine
	virtual PhysicsObject* MakeObject(const Transform& transform, const Shape& shape, const PhysicsMaterial& material) override;

	virtual PhysicsObject* MakeObject(const Transform& transform, const CompoundShape& shape, const CompoundPhysicsMaterial& material) override;

	virtual void StepSimulation(
		float deltaTime, 
		std::vector<PhysicsObject*>* dynamicObjectsAdd,
		std::vector<PhysicsObject*>* dynamicObjectsRemove) override;


	// Inherited via PhysicsEngine
	virtual void SetGravity(const Vec3& g) override;

	virtual Vec3 GetGravity() override;

	virtual void AddObjects(std::vector<PhysicsObject*>& objects) override;

	virtual void RemoveObjects(std::vector<PhysicsObject*>& objects) override;

};

