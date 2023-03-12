#include "ScriptSystem.h"

#include <iostream>

#include "Components/Script/Script.h"

NAMESPACE_BEGIN

ScriptSystem::ScriptSystem(Scene* scene) : SubSystem(scene, Script::COMPONENT_ID)
{
}

void ScriptSystem::PrevIteration(float dt)
{
}

void ScriptSystem::Iteration(float dt)
{
	std::cout << "ScriptSystem::Iteration()\n";
}

void ScriptSystem::PostIteration(float dt)
{
}

void ScriptSystem::AddScript(Script* script)
{

}
NAMESPACE_END
