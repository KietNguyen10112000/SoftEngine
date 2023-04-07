#include "ScriptSystem.h"

#include <iostream>

#include "Components/Script/Script.h"

NAMESPACE_BEGIN

ScriptSystem::ScriptSystem(Scene* scene) : SubSystem(scene, Script::COMPONENT_ID)
{
	m_scripts.reserve(1 * MB);

	void (*fn)(void*) = [](void* p)
	{
		auto param = (ScriptTaskParam*)p;
		auto it = param->begin;
		auto end = param->end;
		auto dt = param->dt;
		auto mergingUnit = param->mergingUnit;

		mergingUnit->MergeBegin();

		while (it != end)
		{
			auto script = *it;
			script->Update(dt);
			mergingUnit->Merge(script->m_object);
			it++;
		}

		mergingUnit->MergeEnd();
	};

	auto numTasks = m_numMergingUnits;
	for (size_t i = 0; i < numTasks; i++)
	{
		m_tasks[i].Entry() = fn;
		m_tasks[i].Params() = &m_tasksParam[i];

		m_tasksParam[i].mergingUnit = m_mergingUnits[i];
	}
}

void ScriptSystem::PrevIteration(float dt)
{
}

void ScriptSystem::Iteration(float dt)
{
	auto numScript = m_scripts.size();
	if (numScript == 0)
	{
		return;
	}

	if (numScript <= 2 * SubSystemMergingUnit::MERGE_BATCHSIZE)
	{
		auto& param = m_tasksParam[0];
		param.begin = &m_scripts[0];
		param.end = param.begin + numScript;
		param.dt = dt;
		m_tasks[0].Entry()(&param);
		return;
	}

	auto numTasks = m_numMergingUnits;
	auto numScripPerTask = numScript / numTasks;

	auto it = &m_scripts[0];
	auto end = it + numScript;
	size_t i = 0;
	while (it + numScripPerTask < end)
	{
		auto& param = m_tasksParam[i];
		param.begin = it;
		param.end = it + numScripPerTask;
		param.dt = dt;

		it += numScripPerTask;
		i++;
	}
	
	auto& param = m_tasksParam[i];
	param.begin = it;
	param.end = end;
	param.dt = dt;

	TaskSystem::SubmitAndWait(m_tasks, numTasks, Task::HIGH);
}

void ScriptSystem::PostIteration(float dt)
{
}

void ScriptSystem::AddScript(Script* script)
{
	script->m_scriptId = m_scripts.size();
	m_scripts.push_back(script);
}

void ScriptSystem::RemoveScript(Script* script)
{
	auto back = m_scripts.back();
	m_scripts[script->m_scriptId] = back;
	m_scripts.pop_back();
}

NAMESPACE_END
