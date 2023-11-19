#include "AnimationSystem.h"

NAMESPACE_BEGIN

AnimationSystem::AnimationSystem(Scene* scene) : MainSystem(scene)
{
}

AnimationSystem::~AnimationSystem()
{
}

void AnimationSystem::BeginModification()
{
}

void AnimationSystem::AddComponent(MainComponent* comp)
{
}

void AnimationSystem::RemoveComponent(MainComponent* comp)
{
}

void AnimationSystem::OnObjectTransformChanged(MainComponent* comp)
{
}

void AnimationSystem::EndModification()
{
}

void AnimationSystem::PrevIteration()
{
}

void AnimationSystem::Iteration(float dt)
{
}

void AnimationSystem::PostIteration()
{
}

NAMESPACE_END