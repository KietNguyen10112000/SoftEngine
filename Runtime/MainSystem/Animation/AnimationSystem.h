#pragma once

#include "../MainSystem.h"

#include "Core/Structures/STD/STDContainers.h"
#include "TaskSystem/TaskSystem.h"

NAMESPACE_BEGIN

class AnimationComponent;
class AnimationGameObject;

class API AnimationSystem : public MainSystem
{
private:
	std::vector<AnimationGameObject*> m_animationGameObjectRoots;

public:
	AnimationSystem(Scene* scene);
	~AnimationSystem();

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