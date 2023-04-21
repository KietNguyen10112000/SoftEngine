#include "ScriptSystem.h"

#include "Core/TemplateUtils/TemplateUtils.h"

#include "Components/Script/Script.h"

#include <iostream>

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

bool ScriptSystem::FilterAddSubSystemComponent(SubSystemComponent* comp)
{
	static Script dummy;

	auto script = (Script*)comp;
	if (IsOverridden<&Script::OnGUI>(&dummy, script))
	{
		script->m_onGUIId = m_onGUI.size();
		m_onGUI.push_back(script);
	}
	
	return true;
}

bool ScriptSystem::FilterRemoveSubSystemComponent(SubSystemComponent* comp)
{
	auto script = (Script*)comp;
	auto onGUIId = script->m_onGUIId;
	if (onGUIId != INVALID_ID)
	{
		m_onGUI.back()->m_onGUIId = onGUIId;
		m_onGUI[onGUIId] = m_onGUI.back();
		m_onGUI.pop_back();
	}

	return true;
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
