#include "ScriptScheduler.h"

#include "Components/Script.h"

NAMESPACE_BEGIN

ScriptScheduler::ScriptScheduler()
{
	m_onGUIs.reserve(8 * KB);
	m_onUpdates.reserve(8 * KB);
}

void ScriptScheduler::CallOnUpdate(float dt)
{
	for (auto& script : m_onUpdates)
	{
		script->OnUpdate(dt);
	}
}

void ScriptScheduler::CallOnGUI()
{
	for (auto& script : m_onUpdates)
	{
		script->OnGUI();
	}
}

NAMESPACE_END