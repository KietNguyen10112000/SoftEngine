#pragma once

#include "../MainSystem.h"

#include "Core/Memory/Memory.h"

#include "Core/Structures/Raw/UnorderedList.h"
#include "Common/Base/AsyncTaskRunner.h"

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

	AsyncTaskRunner m_mAsyncTaskRunnerST[NUM_DEFER_BUFFER] = {};

	ScriptingSystem(Scene* scene);
	~ScriptingSystem();
	virtual void Finalize() override;

private:
	inline auto* GetCurrentMAsyncTaskRunnerST()
	{
		return &m_mAsyncTaskRunnerST[m_scene->GetCurrentDeferBufferIdx()];
	}

	inline auto* GetPrevMAsyncTaskRunnerST()
	{
		return &m_mAsyncTaskRunnerST[m_scene->GetPrevDeferBufferIdx()];
	}

public:
	// Inherited via MainSystem
	virtual void FlushAsyncTasks() override;

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

public:
#ifdef PLUGIN_ALLOW_HOT_RELOAD
	
#endif

	inline auto* MAsyncTaskRunnerST()
	{
		return GetCurrentMAsyncTaskRunnerST();
	}

};

NAMESPACE_END