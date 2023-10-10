#include "ScriptingSystem.h"

NAMESPACE_BEGIN

ScriptingSystem::ScriptingSystem(Scene* scene) : MainSystem(scene)
{
}

ScriptingSystem::~ScriptingSystem()
{
}

void ScriptingSystem::AddComponent(MainComponent* comp)
{
}

void ScriptingSystem::RemoveComponent(MainComponent* comp)
{
}

void ScriptingSystem::OnObjectTransformChanged(MainComponent* comp)
{
}

void ScriptingSystem::PrevIteration()
{
}

void ScriptingSystem::Iteration(float dt)
{
}

void ScriptingSystem::PostIteration()
{
}

NAMESPACE_END