#pragma once

#include "../MainSystem.h"

#include "Core/Memory/Memory.h"

NAMESPACE_BEGIN

class ScriptingSystem : Traceable<ScriptingSystem>, public MainSystem
{
public:
	ScriptingSystem(Scene* scene);
	~ScriptingSystem();

protected:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{

	}

public:
	// Inherited via MainSystem
	virtual void AddComponent(MainComponent* comp) override;

	virtual void RemoveComponent(MainComponent* comp) override;

	virtual void OnObjectTransformChanged(MainComponent* comp) override;

	virtual void PrevIteration() override;

	virtual void Iteration(float dt) override;

	virtual void PostIteration() override;

};

NAMESPACE_END