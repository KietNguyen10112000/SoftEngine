#pragma once

#include "SubSystems/SubSystem.h"

NAMESPACE_BEGIN

class Scene;

class RenderingSystem : public SubSystem
{
public:
	RenderingSystem(Scene* scene);

public:
	virtual void PrevIteration(float dt) override;

	virtual void Iteration(float dt) override;

	virtual void PostIteration(float dt) override;

};

NAMESPACE_END