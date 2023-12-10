#pragma once

#include "../MainSystem.h"

#include "Core/Structures/STD/STDContainers.h"
#include "TaskSystem/TaskSystem.h"

#include "Common/Base/AsyncTaskRunnerRaw.h"
#include "Runtime/Config.h"

#include "Scene/Scene.h"

NAMESPACE_BEGIN

class AnimationComponent;
class AnimationGameObject;
class AnimSkeletalGameObject;
class AnimatorSkeletalGameObject;
class AnimatorSkeletalArray;

class API AnimationSystem : public MainSystem
{
private:
	constexpr static size_t NUM_DEFER_BUFFER = Config::NUM_DEFER_BUFFER;

	friend class AnimSkeletalGameObject;

	struct AnimMeshRenderingBufferCounter
	{
		size_t count;
		void* p;
		AnimatorSkeletalGameObject* animator;
	};

	std::vector<AnimSkeletalGameObject*> m_animSkeletalGameObjects;
	std::vector<AnimatorSkeletalArray*> m_animSkeletalArrays;

	std::vector<AnimMeshRenderingBufferCounter> m_animMeshRenderingBufferCount;

	raw::AsyncTaskRunner<AnimationSystem> m_asyncTaskRunnerST[NUM_DEFER_BUFFER] = {};
	raw::AsyncTaskRunner<AnimationSystem> m_asyncTaskRunnerMT[NUM_DEFER_BUFFER] = {};

public:
	AnimationSystem(Scene* scene);
	~AnimationSystem();

private:
	void AddAnimMeshRenderingBuffer(void*, AnimatorSkeletalGameObject* animator);
	void RemoveMeshRenderingBuffer(void*, AnimatorSkeletalGameObject* animator);
	void CalculateAABBForMeshRenderingBuffer(AnimMeshRenderingBufferCounter* counter);

	inline auto* GetCurrentAsyncTaskRunnerST()
	{
		return &m_asyncTaskRunnerST[m_scene->GetCurrentDeferBufferIdx()];
	}

	inline auto* GetCurrentAsyncTaskRunnerMT()
	{
		return &m_asyncTaskRunnerMT[m_scene->GetCurrentDeferBufferIdx()];
	}

	inline auto* GetPrevAsyncTaskRunnerST()
	{
		return &m_asyncTaskRunnerST[m_scene->GetPrevDeferBufferIdx()];
	}

	inline auto* GetPrevAsyncTaskRunnerMT()
	{
		return &m_asyncTaskRunnerMT[m_scene->GetPrevDeferBufferIdx()];
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

};

NAMESPACE_END