#pragma once

#include "SubSystems/SubSystem.h"

NAMESPACE_BEGIN

class Scene;
class Script;

class ScriptSystem : public SubSystem
{
public:
	ScriptSystem(Scene* scene);

private:
	static void ForEachScript(ID, SubSystem*, GameObject*, void*);

public:
	virtual void PrevIteration(float dt) override;

	virtual void Iteration(float dt) override;

	virtual void PostIteration(float dt) override;

};

NAMESPACE_END