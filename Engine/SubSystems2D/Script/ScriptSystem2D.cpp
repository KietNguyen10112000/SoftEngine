#include "ScriptSystem2D.h"

#include "Core/TemplateUtils/TemplateUtils.h"

#include "Components2D/Script/Script2D.h"
#include "Components2D/Physics/Physics2D.h"

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
	for (auto& script : m_onCollide)
	{
		auto physics = script->GetObject()->GetComponentRaw<Physics2D>();
		auto& pairs = physics->CollisionPairs();
		for (auto& pair : pairs)
		{
			auto another = pair->GetAnotherOf(physics);
			if (another && pair->result.HasCollision())
			{
				script->OnCollide(another->GetObject(), *pair);
			}
		}
	}

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

	if (IsOverridden<&Script2D::OnCollide>(&g_ScriptSystem2DDummy, script))
	{
		script->m_onCollideId = m_onCollide.size();
		m_onCollide.push_back(script);
	}

	if (IsOverridden<&Script2D::OnUpdate>(&g_ScriptSystem2DDummy, script))
	{
		if (root->m_subSystemID[COMPONENT_ID] != INVALID_ID)
		{
			assert(0);
		}
		root->m_subSystemID[COMPONENT_ID] = m_onUpdate.size();
		m_onUpdate.push_back(root);
	}
}

void ScriptSystem2D::RemoveSubSystemComponent(SubSystemComponent2D* comp)
{
	auto root = comp->GetObject()->GetRoot();
	auto script = (Script2D*)comp;

	if (script->m_onGUIId != INVALID_ID)
	{
		STD_VECTOR_ROLL_TO_FILL_BLANK(m_onGUI, script->m_onGUIId, back->m_onGUIId);
		script->m_onGUIId = INVALID_ID;
	}

	if (script->m_onCollideId != INVALID_ID)
	{
		STD_VECTOR_ROLL_TO_FILL_BLANK(m_onCollide, script->m_onCollideId, back->m_onCollideId);
		script->m_onCollideId = INVALID_ID;
	}

	if (root->m_subSystemID[COMPONENT_ID] != INVALID_ID)
	{
		STD_VECTOR_ROLL_TO_FILL_BLANK(m_onUpdate, root->m_subSystemID[COMPONENT_ID], back->m_subSystemID[COMPONENT_ID]);
		root->m_subSystemID[COMPONENT_ID] = INVALID_ID;
	}
}

NAMESPACE_END
