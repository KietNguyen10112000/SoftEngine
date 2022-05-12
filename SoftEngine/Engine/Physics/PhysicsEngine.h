#pragma once

#include "./Shape.h"
#include "./PhysicsMaterial.h"

class PhysicsObject;

class PhysicsEngine
{
public:
	inline virtual ~PhysicsEngine() {};

public:
	virtual void SetGravity(const Vec3& g) = 0;
	virtual Vec3 GetGravity() = 0;

public:
	virtual PhysicsObject* MakeObject(
		const Transform& transform,
		const Shape& shape, 
		const PhysicsMaterial& material
	) = 0;

	virtual PhysicsObject* MakeObject(
		const Transform& transform,
		const CompoundShape& shape,
		const CompoundPhysicsMaterial& material
	) = 0;

	virtual void AddObjects(std::vector<PhysicsObject*>& objects) = 0;
	virtual void RemoveObjects(std::vector<PhysicsObject*>& objects) = 0;

public:
	virtual void StepSimulation(
		float deltaTime, 
		std::vector<PhysicsObject*>* dynamicObjectsAdd, 
		std::vector<PhysicsObject*>* dynamicObjectsRemove
	) = 0;

};

