#include "ScriptScheduler.h"

#include "Components/Script.h"

NAMESPACE_BEGIN

ScriptScheduler::ScriptScheduler()
{
	m_onGUIs.reserve(8 * KB);
	m_onUpdates.reserve(8 * KB);

	for (auto& list : m_asyncCalls)
	{
		list.ReserveNoSafe(8 * KB);
	}
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

bool ScriptScheduler::RecordAsyncCall(ScriptingSystem* system, Script* script)
{
	auto scene = script->GetScene();
	auto iterationCount = scene->GetIterationCount();
	if (m_flushAsyncCallsCount.exchange(iterationCount) == iterationCount)
	{
		return false;
	}

	m_asyncCalls[scene->GetCurrentDeferBufferIdx()].Add(script);

	return true;
}

void ScriptScheduler::FlushAsyncCalls(Scene* scene)
{
	auto& list = m_asyncCalls[scene->GetPrevDeferBufferIdx()];
	for (auto& v : list)
	{
		v->FlushAsync();
	}
	list.Clear();
}

NAMESPACE_END