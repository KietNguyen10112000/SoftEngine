#include "ScriptSystem2D.h"

#include "Core/TemplateUtils/TemplateUtils.h"

#include "Components2D/Script/Script2D.h"

#include "Objects2D/GameObject2D.h"

#include <iostream>

NAMESPACE_BEGIN

Script2D g_ScriptSystem2DDummy;

ScriptSystem2D::ScriptSystem2D(Scene2D* scene) : SubSystem2D(scene, Script2D::COMPONENT_ID)
{
	
}

void ScriptSystem2D::PrevIteration(float dt)
{
}

void ScriptSystem2D::Iteration(float dt)
{
	for (auto& obj : m_onUpdate)
	{
		GameObject2D::PostTraversal(
			obj,
			[=](GameObject2D* obj) 
			{
				auto script = obj->GetComponentRaw<Script2D>();
				if (script)
				{
					script->OnUpdate(dt);
				}
			}
		);
	}
}

void ScriptSystem2D::PostIteration(float dt)
{
}

void ScriptSystem2D::AddSubSystemComponent(SubSystemComponent2D* comp)
{
	auto root = comp->GetObject()->GetRoot();

	auto script = (Script2D*)comp;
	if (IsOverridden<&Script2D::OnGUI>(&g_ScriptSystem2DDummy, script))
	{
		script->m_onGUIId = m_onGUI.size();
		m_onGUI.push_back(script);
	}

	if (IsOverridden<&Script2D::OnUpdate>(&g_ScriptSystem2DDummy, script))
	{
		if (root->m_subSystemID[COMPONENT_ID] != INVALID_ID)
		{
			root->m_subSystemID[COMPONENT_ID] = m_onUpdate.size();
		}
		m_onUpdate.push_back(root);
	}
}

void ScriptSystem2D::RemoveSubSystemComponent(SubSystemComponent2D* comp)
{
	auto script = (Script2D*)comp;
}

NAMESPACE_END
