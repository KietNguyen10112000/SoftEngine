#include "ScriptingSystem.h"

#include "Scene/GameObject.h"

#include "Components/Script.h"

#include "ScriptMeta.h"
#include "ScriptScheduler.h"

NAMESPACE_BEGIN

ScriptingSystem::ScriptingSystem(Scene* scene) : MainSystem(scene)
{
	m_schedulers.reserve(8 * KB);
}

ScriptingSystem::~ScriptingSystem()
{
	for (auto& s : m_schedulers)
	{
		delete s;
	}
	m_schedulers.clear();
}

void ScriptingSystem::BeginModification()
{
}

void ScriptingSystem::AddComponent(MainComponent* comp)
{
	if (comp->GetGameObject()->Parent().Get() != nullptr)
	{
		return;
	}

	auto script = (Script*)comp;
	auto metaData = script->GetScriptMetaData();
	
	assert(metaData->schedulerId != INVALID_ID);

	if (m_schedulers.size() <= metaData->schedulerId)
	{
		m_schedulers.resize(metaData->schedulerId + 1);
		m_schedulers[metaData->schedulerId] = new ScriptScheduler();
	}

	auto scheduler = m_schedulers[metaData->schedulerId];
	if (metaData->overriddenVtbIdx.test(ScriptMeta::Get()->onGUIVtbIdx))
	{
		assert(script->m_onGUIId == INVALID_ID && "script added twices");
		script->m_onGUIId = scheduler->m_onGUIs.size();
		scheduler->m_onGUIs.push_back(script);
	}

	if (metaData->overriddenVtbIdx.test(ScriptMeta::Get()->onUpdateVtbIdx))
	{
		assert(script->m_onUpdateId == INVALID_ID && "script added twices");
		script->m_onUpdateId = scheduler->m_onUpdates.size();
		scheduler->m_onUpdates.push_back(script);
	}
}

void ScriptingSystem::RemoveComponent(MainComponent* comp)
{
	if (comp->GetGameObject()->Parent().Get() != nullptr)
	{
		return;
	}

	auto script = (Script*)comp;
	auto metaData = script->GetScriptMetaData();

	assert(metaData->schedulerId != INVALID_ID);
	assert(m_schedulers.size() > metaData->schedulerId);

	auto scheduler = m_schedulers[metaData->schedulerId];
	if (script->m_onGUIId != INVALID_ID)
	{
		auto& list = scheduler->m_onGUIs;
		STD_VECTOR_ROLL_TO_FILL_BLANK(list, script, m_onGUIId);
		script->m_onGUIId = INVALID_ID;
	}

	if (script->m_onUpdateId != INVALID_ID)
	{
		auto& list = scheduler->m_onUpdates;
		STD_VECTOR_ROLL_TO_FILL_BLANK(list, script, m_onUpdateId);
		script->m_onUpdateId = INVALID_ID;
	}
}

void ScriptingSystem::OnObjectTransformChanged(MainComponent* comp)
{
}

void ScriptingSystem::EndModification()
{
}

void ScriptingSystem::PrevIteration()
{
}

void ScriptingSystem::Iteration(float dt)
{
	for (auto& scheduler : m_schedulers)
	{
		if (scheduler)
		{
			scheduler->CallOnUpdate(dt);
		}
	}
}

void ScriptingSystem::PostIteration()
{
}

NAMESPACE_END