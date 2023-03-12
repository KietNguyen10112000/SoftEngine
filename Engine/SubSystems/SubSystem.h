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

#define MERGE_OBJECTS(objs, num)				\
for (size_t i = 0; i < num; i++)				\
{												\
	objs[i]->MergeSubSystemComponentsData();	\
}

class SubSystem
{
protected:
	constexpr static size_t MERGE_BATCHSIZE = 256;

	struct MergeTaskParam
	{
		GameObject** objs;
		std::atomic<bool> processing;
		bool processed;
	};

	Pool<sizeof(GameObject*) * MERGE_BATCHSIZE, 16, 1, rheap::malloc, rheap::free> m_allocator = { 1 };
	Pool<sizeof(MergeTaskParam), 16, 1, rheap::malloc, rheap::free> m_paramAllocator = { 1 };

	const union
	{
		ID COMPONENT_ID;
		ID SUBSYSTEM_ID;
	};
	
	Scene* m_scene = nullptr;

	GameObject** m_currentMergingObjs = nullptr;
	size_t m_currentMergingObjsSize = 0;

	GameObject** m_prevMergingObjs = nullptr;

	std::Vector<MergeTaskParam*> m_mergingTasks;

	void(*TASK_FN)(void*) = [](void* p)
	{
		auto param = (MergeTaskParam*)p;

		if (param->processing.exchange(true))
		{
			// the batch was processed by SubSystem thread
			return;
		}

		auto objs = param->objs;
		MERGE_OBJECTS(objs, MERGE_BATCHSIZE);

		param->processed = true;
	};

public:
	inline SubSystem(Scene* scene, ID subSystemID) : m_scene(scene), COMPONENT_ID(subSystemID) 
	{
		m_currentMergingObjs = (GameObject**)m_allocator.Allocate();
	};

public:
	// call before scene reconstruction, so all scene query methods can not be use
	virtual void PrevIteration(float dt) = 0;

	// call after scene reconstruction
	virtual void Iteration(float dt) = 0;

	// call after Iteration =)))
	virtual void PostIteration(float dt) = 0;

public:
	inline void MergeSynch()
	{
		if (m_currentMergingObjs)
		{
			MERGE_OBJECTS(m_prevMergingObjs, m_currentMergingObjsSize);
		}

		if (m_prevMergingObjs)
		{
			MERGE_OBJECTS(m_prevMergingObjs, MERGE_BATCHSIZE);
		}

		for (auto& param : m_mergingTasks)
		{
			TASK_FN(param);
		}

		for (auto& param : m_mergingTasks)
		{
			if (param->processed == false)
			{
				Thread::Yield();
			}

			m_allocator.Deallocate(param->objs);
			m_paramAllocator.Deallocate(param);
		}

		m_mergingTasks.clear();
		m_currentMergingObjs = nullptr;
		m_currentMergingObjsSize = 0;
		m_prevMergingObjs = nullptr;
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
				// dispatch batched objects to process merging
				if (m_prevMergingObjs)
				{
					auto param = (MergeTaskParam*)m_paramAllocator.Allocate();
					param->objs = m_prevMergingObjs;
					param->processed = false;
					m_mergingTasks.push_back(param);

					Task task;
					task.Params() = param;
					task.Entry() = TASK_FN;
					TaskSystem::Submit(task, Task::HIGH);
				}
				
				// defer dispatch so that 2 last batches will be excute on this thread 
				// and all dispatched tasks will be compeleted before SubSystem thread
				m_prevMergingObjs = m_currentMergingObjs;
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

#undef MERGE_OBJECTS

NAMESPACE_END