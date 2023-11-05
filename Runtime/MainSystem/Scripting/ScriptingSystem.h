#pragma once

#include "../MainSystem.h"

#include "Core/Memory/Memory.h"

#include "Core/Structures/Raw/UnorderedList.h"

NAMESPACE_BEGIN

class ScriptScheduler;
class Script;

class ScriptingSystem : public MainSystem
{
public:
	friend class Script;

	constexpr static size_t NUM_DEFER_BUFFER = 2;
	std::vector<ScriptScheduler*> m_schedulers;
	std::vector<ScriptScheduler*> m_callAsyncSchedulers[NUM_DEFER_BUFFER];

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

private:
	void OnScriptRecordAsyncTask(Script* script);

};

NAMESPACE_END