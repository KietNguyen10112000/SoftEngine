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
//
//// call refresh only on root obj
//#define MERGE_OBJECTS(objs, num)				\
//for (size_t i = 0; i < num; i++)				\
//{												\
//	auto* o = objs[i];							\
//	o->MergeSubSystemComponentsData();			\
//	if (o->m_isNeedRefresh && o->IsRootObject())\
//	{											\
//		o->m_scene->RefreshObject(o);			\
//		o->m_isNeedRefresh = false;				\
//	}											\
//}
//
//class SubSystemMergingUnit
//{
//public:
//	constexpr static size_t MERGE_BATCHSIZE = 512;
//
//protected:
//	constexpr static void(*TASK_FN)(void*) = [](void* p)
//	{
//		auto objs = (GameObject**)p;
//		MERGE_OBJECTS(objs, MERGE_BATCHSIZE);
//	};
//
//	Pool<sizeof(GameObject*)* MERGE_BATCHSIZE, 16, 1, rheap::malloc, rheap::free> m_allocator = { 1 };
//
//	Scene* m_scene = nullptr;
//
//	GameObject** m_currentMergingObjs = nullptr;
//	size_t m_currentMergingObjsSize = 0;
//
//	std::Vector<GameObject**> m_mergingTasks;
//
//	TaskWaitingHandle m_taskWaitingHandle = { 0,0 };
//
//public:
//	SubSystemMergingUnit(Scene* scene) : m_scene(scene)
//	{
//		m_currentMergingObjs = (GameObject**)m_allocator.Allocate();
//		m_mergingTasks.reserve(1024);
//	}
//
//public:
//	inline void MergeBegin()
//	{
//		TaskSystem::PrepareHandle(&m_taskWaitingHandle);
//	}
//
//	inline void MergeEnd()
//	{
//		if (m_currentMergingObjs)
//		{
//			MERGE_OBJECTS(m_currentMergingObjs, m_currentMergingObjsSize);
//		}
//
//		TaskSystem::WaitForHandle(&m_taskWaitingHandle);
//
//		for (auto& param : m_mergingTasks)
//		{
//			m_allocator.Deallocate(param);
//		}
//
//		m_mergingTasks.clear();
//		m_currentMergingObjs = nullptr;
//		m_currentMergingObjsSize = 0;
//	}
//
//	inline void Merge(GameObject* obj)
//	{
//		assert(obj->IsRootObject());
//
//		if (obj->m_numBranch == 1)
//		{
//			return;
//		}
//
//		if (GameObjectDirectAccessor::BranchMerge(obj))
//		{
//			auto index = m_currentMergingObjsSize++;
//
//			if (index == MERGE_BATCHSIZE)
//			{
//				// dispatch merging task for other threads
//				Task task;
//				task.Params() = m_currentMergingObjs;
//				task.Entry() = TASK_FN;
//				TaskSystem::Submit(&m_taskWaitingHandle, task, Task::HIGH);
//
//				m_mergingTasks.push_back(m_currentMergingObjs);
//				m_currentMergingObjs = (GameObject**)m_allocator.Allocate();
//				m_currentMergingObjsSize = 0;
//			}
//			else
//			{
//				// record obj
//				m_currentMergingObjs[index] = obj;
//			}
//			return;
//		}
//
//		if (obj->m_isBranched.load() == false)
//		{
//			if (obj->m_isBranched.exchange(true) == false)
//			{
//				m_scene->RecordBranchedObject(obj);
//			}
//		}
//	}
//
//};

class SubSystem
{
protected:
	constexpr static size_t MERGE_BATCHSIZE = 512;

	const union
	{
		ID COMPONENT_ID;
		ID SUBSYSTEM_ID;
	};
	
	Scene* m_scene = nullptr;

	//byte m_buffer[sizeof(SubSystemMergingUnit) * ThreadLimit::MAX_THREADS];

	//SubSystemMergingUnit*		m_mergingUnits[ThreadLimit::MAX_THREADS];
	size_t						m_numMergingUnits = 0;

	// root objects to be processed by this SubSystem
	std::Vector<GameObject*>	m_rootObjects;


	// for multi-thread foreach m_rootObjects
	using ForEachCallback = void(*)(ID, SubSystem*, GameObject*, void*);
	struct TaskParam
	{
		SubSystem*		subSystem;
		size_t			startIdx;
		size_t			dispatchId;
		void*			userPtr;
		//ForEachCallback callback;
	};

	Task					m_processTasks[ThreadLimit::MAX_THREADS] = {};
	TaskParam				m_processParams[ThreadLimit::MAX_THREADS] = {};
	size_t					m_rootObjectCount = 0;
	std::atomic<size_t>		m_processedObjectCount = { 0 };

	/*constexpr static void(*TASK_FN)(void*) = [](void* p)
	{
		TASK_SYSTEM_UNPACK_PARAM_4(TaskParam, p, subSystem, startIdx, dispatchId, callback);

		auto objects = subSystem->m_rootObjects.data();

		auto size = subSystem->m_rootObjectCount;
		auto& processedCount = subSystem->m_processedObjectCount;

		auto endId = startIdx == 0 ? size : startIdx - 1;
		auto id = startIdx;
		while (processedCount.load(std::memory_order_relaxed) == size)
		{
			auto obj = objects[id];

			callback(dispatchId, subSystem, obj, 0);

			if (id == endId)
			{
				break;
			}

			id = (id + 1) % size;
		}
	};*/

public:
	inline SubSystem(Scene* scene, ID subSystemID) : m_scene(scene), COMPONENT_ID(subSystemID) 
	{
		// allocator of SubSystemMergingUnit allocate large block memory, so dynamic initialize it
		m_numMergingUnits = std::max(1ull, TaskSystem::GetWorkerCount() / SubSystemInfo::GetNumAvailabelIndexedSubSystemCount());
		/*for (size_t i = 0; i < m_numMergingUnits; i++)
		{
			m_mergingUnits[i] = (SubSystemMergingUnit*)&m_buffer[i * sizeof(SubSystemMergingUnit)];
			new (m_mergingUnits[i]) SubSystemMergingUnit(m_scene);
		}*/
	};

	inline virtual ~SubSystem()
	{
		/*for (size_t i = 0; i < m_numMergingUnits; i++)
		{
			m_mergingUnits[i]->~SubSystemMergingUnit();
		}*/
	}

public:
	// call before scene reconstruction, so all scene query methods can not be use
	virtual void PrevIteration(float dt) = 0;

	// call after scene reconstruction
	virtual void Iteration(float dt) = 0;

	// call after Iteration =)))
	virtual void PostIteration(float dt) = 0;

protected:
	template <auto FN>
	void InitForEachRootObjects()
	{
		constexpr void(*TASK_FN)(void*) = [](void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_4(TaskParam, p, subSystem, startIdx, dispatchId, userPtr);

			//auto mergingUnit = subSystem->m_mergingUnits[dispatchId];
			auto objects = subSystem->m_rootObjects.data();

			auto size = subSystem->m_rootObjectCount;
			auto& processedCount = subSystem->m_processedObjectCount;

			auto endId = startIdx == 0 ? size : startIdx - 1;
			auto id = startIdx;

			auto scene = subSystem->m_scene;
			auto iterationCount = scene->GetIterationCount();

			const auto COMPONENT_ID = subSystem->COMPONENT_ID;

			bool refresh = false;

			//mergingUnit->MergeBegin();
			while (processedCount.load(std::memory_order_relaxed) != size)
			{
				processedCount++;

				auto obj = objects[id];
				auto& lock = obj->m_subSystemProcessCount[COMPONENT_ID];
				if (lock.load(std::memory_order_relaxed) == iterationCount
					|| lock.exchange(iterationCount) == iterationCount)
				{
					goto Next;
				}

				assert(scene == obj->m_scene);
				assert(obj->IsRootObject());

				FN(dispatchId, subSystem, obj, userPtr);

				refresh = false;
				if (obj->m_numBranch != 1)
				{
					if (GameObjectDirectAccessor::BranchMerge(obj))
					{
						obj->MergeSubSystemComponentsData();
						refresh = true;
					}
					else 
					{
						// record
						if (obj->m_isBranched.load(std::memory_order_relaxed) == false 
							&& obj->m_isBranched.exchange(true) == false)
						{
							scene->RecordBranchedObject(obj);
						}
					}
				}
				else
				{
					refresh = true;
				}

				if (refresh && obj->m_isNeedRefresh)
				{
					scene->RefreshObject(obj);
					obj->m_isNeedRefresh = false;
				}
				
			Next:
				if (id == endId)
				{
					break;
				}

				id = (id + 1) % size;
			}
		};

		for (size_t i = 0; i < ThreadLimit::MAX_THREADS; i++)
		{
			auto& task = m_processTasks[i];
			auto& param = m_processParams[i];
			task.Entry() = TASK_FN;
			task.Params() = &param;

			param.subSystem = this;
			param.dispatchId = i;
		}
	}

	// multi-threaded
	void ForEachRootObjects(void* userPtr)
	{
		m_rootObjectCount = m_rootObjects.size();
		m_processedObjectCount = 0;

		if (m_rootObjectCount <= MERGE_BATCHSIZE)
		{
			auto& param = m_processParams[0];
			param.startIdx = 0;
			param.userPtr = userPtr;
			m_processTasks[0].Entry()(&param);
			return;
		}

		auto numTasks = m_numMergingUnits;
		auto numPerTask = m_rootObjectCount / numTasks;
		auto start = 0;
		for (size_t i = 0; i < m_numMergingUnits; i++)
		{
			auto& param = m_processParams[i];
			param.startIdx = start;
			param.userPtr = userPtr;
			start += numPerTask;
		}

		TaskSystem::SubmitAndWait(m_processTasks, numTasks, Task::HIGH);
	}

public:
	inline void AddSubSystemComponent(SubSystemComponent* comp, const ID COMPONENT_ID)
	{
		auto obj = comp->GetObject();
		auto root = obj->GetRoot();

		assert(root->m_subSystemCompCounts[COMPONENT_ID] != 0);

		if (root->m_subSystemCompCounts[COMPONENT_ID] == 1)
		{
			root->m_subSystemId[COMPONENT_ID] = m_rootObjects.size();
			m_rootObjects.push_back(root);
		}
	}

	inline void RemoveSubSystemComponent(SubSystemComponent* comp, const ID COMPONENT_ID)
	{
		auto obj = comp->GetObject();
		auto root = obj->GetRoot();

		assert(root->m_subSystemCompCounts[COMPONENT_ID] != 0);

		if (root->m_subSystemCompCounts[COMPONENT_ID] == 1)
		{
			if (m_rootObjects.size() > 1)
			{
				auto back = m_rootObjects.back();
				back->m_subSystemId[COMPONENT_ID] = root->m_subSystemId[COMPONENT_ID];
				m_rootObjects[root->m_subSystemId[COMPONENT_ID]] = back;
				return;
			}
			m_rootObjects.clear();
		}
	}

public:
	inline auto GetScene() const
	{
		return m_scene;
	}

};

#undef MERGE_OBJECTS

NAMESPACE_END