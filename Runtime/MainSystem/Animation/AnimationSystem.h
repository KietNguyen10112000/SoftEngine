#pragma once

#include "../MainSystem.h"

#include "Core/Structures/STD/STDContainers.h"
#include "TaskSystem/TaskSystem.h"

NAMESPACE_BEGIN

class AnimationComponent;
class AnimationGameObject;
class AnimSkeletalGameObject;
class Animator;

class API AnimationSystem : public MainSystem
{
private:
	friend class AnimSkeletalGameObject;

	struct AnimMeshRenderingBufferCounter
	{
		size_t count;
		void* p;
		Animator* animator;
	};

	std::vector<AnimSkeletalGameObject*> m_animSkeletalGameObjects;

	std::vector<AnimMeshRenderingBufferCounter> m_animMeshRenderingBufferCount;

public:
	AnimationSystem(Scene* scene);
	~AnimationSystem();

private:
	void AddAnimMeshRenderingBuffer(void*, Animator* animator);
	void RemoveMeshRenderingBuffer(void*, Animator* animator);
	void CalculateAABBForMeshRenderingBuffer(AnimMeshRenderingBufferCounter* counter);

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