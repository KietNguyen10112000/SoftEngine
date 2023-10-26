#pragma once

#include "../MainSystem.h"

#include "Core/Memory/Memory.h"

#include "Core/Structures/Raw/UnorderedList.h"

NAMESPACE_BEGIN

class ScriptScheduler;

class ScriptingSystem : public MainSystem
{
public:
	std::vector<ScriptScheduler*> m_schedulers;

	ScriptingSystem(Scene* scene);
	~ScriptingSystem();

public:
	// Inherited via MainSystem
	virtual void BeginModification() override;

	virtual void AddComponent(MainComponent* comp) override;

	virtual void RemoveComponent(MainComponent* comp) override;

	virtual void OnObjectTransformChanged(MainComponent* comp) override;

	virtual void EndModification() override;

	virtual void PrevIteration() override;

	virtual void Iteration(float dt) override;

	virtual void PostIteration() override;

};

NAMESPACE_END