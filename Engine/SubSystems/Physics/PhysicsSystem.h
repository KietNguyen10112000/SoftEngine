#pragma once

#include "SubSystems/SubSystem.h"

NAMESPACE_BEGIN

class Scene;

class PhysicsSystem : public SubSystem
{
public:
	PhysicsSystem(Scene* scene);

public:
	virtual void PrevIteration(float dt) override;

	virtual void Iteration(float dt) override;

	virtual void PostIteration(float dt) override;

};

NAMESPACE_END