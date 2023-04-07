#pragma once

#include "Core/TypeDef.h"

#include "Core/Memory/Memory.h"
#include "Core/Structures/STD/STDContainers.h"

#include "TaskSystem/TaskSystem.h"

#include "Objects/Scene/Scene.h"
#include "Objects/GameObjectDirectAccessor.h"

NAMESPACE_BEGIN

class Scene;
class GameObject;

// call refresh only on root obj
#define MERGE_OBJECTS(objs, num)				\
for (size_t i = 0; i < num; i++)				\
{												\
	auto* o = objs[i];							\
	o->MergeSubSystemComponentsData();			\
	if (o->m_isNeedRefresh && o->IsRootObject())\
	{											\
		o->m_scene->RefreshObject(o);			\
		o->m_isNeedRefresh = false;				\
	}											\
}

class SubSystemMergingUnit
{
public:
	constexpr static size_t MERGE_BATCHSIZE = 512;

protected:
	constexpr static void(*TASK_FN)(void*) = [](void* p)
	{
		auto objs = (GameObject**)p;
		MERGE_OBJECTS(objs, MERGE_BATCHSIZE);
	};

	Pool<sizeof(GameObject*)* MERGE_BATCHSIZE, 16, 1, rheap::malloc, rheap::free> m_allocator = { 1 };

	Scene* m_scene = nullptr;

	GameObject** m_currentMergingObjs = nullptr;
	size_t m_currentMergingObjsSize = 0;

	std::Vector<GameObject**> m_mergingTasks;

	TaskWaitingHandle m_taskWaitingHandle = { 0,0 };

public:
	SubSystemMergingUnit(Scene* scene) : m_scene(scene)
	{
		m_currentMergingObjs = (GameObject**)m_allocator.Allocate();
		m_mergingTasks.reserve(1024);
	}

public:
	inline void MergeBegin()
	{
		TaskSystem::PrepareHandle(&m_taskWaitingHandle);
	}

	inline void MergeEnd()
	{
		if (m_currentMergingObjs)
		{
			MERGE_OBJECTS(m_currentMergingObjs, m_currentMergingObjsSize);
		}

		TaskSystem::WaitForHandle(&m_taskWaitingHandle);

		for (auto& param : m_mergingTasks)
		{
			m_allocator.Deallocate(param);
		}

		m_mergingTasks.clear();
		m_currentMergingObjs = nullptr;
		m_currentMergingObjsSize = 0;
	}

	inline void Merge(GameObject* obj)
	{
		if (obj->m_numBranch == 1)
		{
			return;
		}

		if (GameObjectDirectAccessor::BranchMerge(obj))
		{
			auto index = m_currentMergingObjsSize++;

			if (index == MERGE_BATCHSIZE)
			{
				// dispatch merging task for other threads
				Task task;
				task.Params() = m_currentMergingObjs;
				task.Entry() = TASK_FN;
				TaskSystem::Submit(&m_taskWaitingHandle, task, Task::HIGH);

				m_mergingTasks.push_back(m_currentMergingObjs);
				m_currentMergingObjs = (GameObject**)m_allocator.Allocate();
				m_currentMergingObjsSize = 0;
			}
			else
			{
				// record obj
				m_currentMergingObjs[index] = obj;
			}
			return;
		}

		if (obj->m_isBranched.load() == false)
		{
			if (obj->m_isBranched.exchange(true) == false)
			{
				m_scene->RecordBranchedObject(obj);
			}
		}
	}

};

class SubSystem
{
protected:
	const union
	{
		ID COMPONENT_ID;
		ID SUBSYSTEM_ID;
	};
	
	Scene* m_scene = nullptr;

	byte m_buffer[sizeof(SubSystemMergingUnit) * ThreadLimit::MAX_THREADS];

	SubSystemMergingUnit* m_mergingUnits[ThreadLimit::MAX_THREADS];
	size_t m_numMergingUnits = 0;

public:
	inline SubSystem(Scene* scene, ID subSystemID) : m_scene(scene), COMPONENT_ID(subSystemID) 
	{
		// allocator of SubSystemMergingUnit allocate large block memory, so dynamic initialize it
		m_numMergingUnits = std::max(1ull, TaskSystem::GetWorkerCount() / SubSystemInfo::GetNumAvailabelIndexedSubSystemCount());
		for (size_t i = 0; i < m_numMergingUnits; i++)
		{
			m_mergingUnits[i] = (SubSystemMergingUnit*)&m_buffer[i * sizeof(SubSystemMergingUnit)];
			new (m_mergingUnits[i]) SubSystemMergingUnit(m_scene);
		}
	};

	inline virtual ~SubSystem()
	{
		for (size_t i = 0; i < m_numMergingUnits; i++)
		{
			m_mergingUnits[i]->~SubSystemMergingUnit();
		}
	}

public:
	// call before scene reconstruction, so all scene query methods can not be use
	virtual void PrevIteration(float dt) = 0;

	// call after scene reconstruction
	virtual void Iteration(float dt) = 0;

	// call after Iteration =)))
	virtual void PostIteration(float dt) = 0;

};

#undef MERGE_OBJECTS

NAMESPACE_END