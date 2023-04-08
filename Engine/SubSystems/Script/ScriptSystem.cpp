#include "ScriptSystem.h"

#include <iostream>

#include "Components/Script/Script.h"

NAMESPACE_BEGIN

void ScriptSystem::ForEachScript(ID, SubSystem* subSystem, GameObject* obj, void*)
{
	auto scriptSystem = (ScriptSystem*)subSystem;
	auto dt = scriptSystem->m_scene->Dt();
	
	GameObject::PostTraversal(obj,
		[=](GameObject* object) 
		{
			auto script = object->GetComponentRaw<Script>();
			if (script)
			{
				script->Update(dt);
			}
		}
	);
}

ScriptSystem::ScriptSystem(Scene* scene) : SubSystem(scene, Script::COMPONENT_ID)
{
	InitForEachRootObjects<ScriptSystem::ForEachScript>();
}

void ScriptSystem::PrevIteration(float dt)
{
}

void ScriptSystem::Iteration(float dt)
{
	ForEachRootObjects(0);
}

void ScriptSystem::PostIteration(float dt)
{
}

NAMESPACE_END
