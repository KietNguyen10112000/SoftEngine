#pragma once

#include "SubSystems2D/SubSystem2D.h"

//#include "Objects/Physics/Collision/Manifold.h"

NAMESPACE_BEGIN

class Scene2D;

class PhysicsSystem2D : public SubSystem2D
{
private:
	size_t m_iterationCount = 0;

	Scene2DQuerySession* m_querySession;

public:
	PhysicsSystem2D(Scene2D* scene);
	~PhysicsSystem2D();

public:
	virtual void PrevIteration(float dt) override;
	virtual void Iteration(float dt) override;
	virtual void PostIteration(float dt) override;
	virtual void AddSubSystemComponent(SubSystemComponent2D* comp) override;
	virtual void RemoveSubSystemComponent(SubSystemComponent2D* comp) override;

};

NAMESPACE_END