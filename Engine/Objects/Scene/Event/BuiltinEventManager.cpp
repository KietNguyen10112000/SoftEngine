#include "BuiltinEventManager.h"

#include "TaskSystem/TaskSystem.h"

#include "BuitinEventImpl.h"

NAMESPACE_BEGIN

template<size_t ID, auto Func, typename T>
inline void InvokeEvent(T objEvents, Scene* scene)
{
	auto& evts = objEvents[ID];
	evts.ForEach([=](GameObject* obj)
		{
			Func(scene, obj);
		}
	);
	evts.Clear();
}

void BuiltinEventManager::FlushAllObjectEvents()
{
	Task tasks[BUILTIN_EVENT_SUIT::COUNT] = {};
	
	for (size_t i = 0; i < BUILTIN_EVENT_SUIT::COUNT; i++)
	{
		auto& task = tasks[i];
		auto& param = m_taskParams[i];

		param.mgr = this;
		param.index = i;

		task.Entry() = [](void* arg)
		{
			TaskParam* param = (TaskParam*)arg;
			auto& objEvents = param->mgr->m_objects[param->index];
			auto scene = param->mgr->m_scene;
			CALL_ALL_EVENTS(objEvents, scene);
		};

		task.Params() = (void*)&param;
	}

	if constexpr (BUILTIN_EVENT_SUIT::COUNT > 1)
	{
		TaskSystem::SubmitAndWait(tasks, BUILTIN_EVENT_SUIT::COUNT);
	}
	else
	{
		tasks->Entry()(tasks->Params());
	}
}

NAMESPACE_END