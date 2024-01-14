#pragma once

#include "../MainSystem.h"

#include "Core/Structures/STD/STDContainers.h"
#include "TaskSystem/TaskSystem.h"

#include "Common/Base/AsyncTaskRunnerRaw.h"
#include "Runtime/Config.h"

#include "Scene/Scene.h"

#include "PhysicsClasses.h"

namespace physx
{
class PxScene;
class PxControllerManager;
}

NAMESPACE_BEGIN

class API PhysicsSystem : public MainSystem
{
private:
	PHYSICS_FRIEND_CLASSES();

	constexpr static size_t NUM_DEFER_BUFFER = Config::NUM_DEFER_BUFFER;

	raw::AsyncTaskRunner<PhysicsSystem> m_asyncTaskRunnerST[NUM_DEFER_BUFFER] = {};
	raw::AsyncTaskRunner<PhysicsSystem> m_asyncTaskRunnerMT[NUM_DEFER_BUFFER] = {};

	raw::AsyncTaskRunnerForMainComponent<PhysicsSystem> m_asyncTaskRunner[NUM_DEFER_BUFFER] = {};

	physx::PxScene* m_pxScene = nullptr;
	physx::PxControllerManager* m_pxControllerManager = nullptr;

public:
	PhysicsSystem(Scene* scene);
	~PhysicsSystem();

private:
	inline auto* GetCurrentAsyncTaskRunnerST()
	{
		return &m_asyncTaskRunnerST[m_scene->GetCurrentDeferBufferIdx()];
	}

	inline auto* GetCurrentAsyncTaskRunnerMT()
	{
		return &m_asyncTaskRunnerMT[m_scene->GetCurrentDeferBufferIdx()];
	}

	inline auto* GetCurrentAsyncTaskRunner()
	{
		return &m_asyncTaskRunner[m_scene->GetCurrentDeferBufferIdx()];
	}

	inline auto* GetPrevAsyncTaskRunnerST()
	{
		return &m_asyncTaskRunnerST[m_scene->GetPrevDeferBufferIdx()];
	}

	inline auto* GetPrevAsyncTaskRunnerMT()
	{
		return &m_asyncTaskRunnerMT[m_scene->GetPrevDeferBufferIdx()];
	}

	inline auto* GetPrevAsyncTaskRunner()
	{
		return &m_asyncTaskRunner[m_scene->GetPrevDeferBufferIdx()];
	}

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

	inline auto* AsyncTaskRunnerST()
	{
		return GetCurrentAsyncTaskRunnerST();
	}

	inline auto* AsyncTaskRunnerMT()
	{
		return GetCurrentAsyncTaskRunnerMT();
	}

	inline auto* AsyncTaskRunner()
	{
		return GetCurrentAsyncTaskRunner();
	}

};

NAMESPACE_END