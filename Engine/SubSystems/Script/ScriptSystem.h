#pragma once

#include "SubSystems/SubSystem.h"

NAMESPACE_BEGIN

class Scene;
class Script;

class ScriptSystem : public SubSystem
{
public:
	ScriptSystem(Scene* scene);

public:
	virtual void PrevIteration(float dt) override;

	virtual void Iteration(float dt) override;

	virtual void PostIteration(float dt) override;

public:
	virtual void AddScript(Script* script);
	virtual void RemoveScript(Script* script);

};

NAMESPACE_END