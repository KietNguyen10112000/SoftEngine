#include "Engine.h"

#include <iostream>

#include "Core/Memory/Memory.h"
#include "Core/Memory/DeferredBuffer.h"

#include "TaskSystem/TaskSystem.h"
#include "TaskSystem/TaskWorker.h"
#include "SubSystems/Physics/PhysicsSystem.h"
#include "SubSystems/Rendering/RenderingSystem.h"

#include "Objects/Scene/Scene.h"
#include "Objects/Scene/MultipleDynamicLayersScene.h"

#include "Objects/Scene/Event/BuiltinEventManager.h"

#include "Objects/Scene/Event/EventManager.h"

#include "Core/Structures/Managed/Function.h"

#include "Core/Random/Random.h"

NAMESPACE_BEGIN

Handle<Engine> Engine::Initialize()
{
	auto old = mheap::internal::GetStableValue();
	mheap::internal::SetStableValue(Engine::STABLE_VALUE);
	auto ret = mheap::New<Engine>();
	mheap::internal::SetStableValue(old);
	return ret;
}

void Engine::Finalize()
{
	mheap::internal::FreeStableObjects(Engine::STABLE_VALUE, 0, 0);
	EventManager::Finalize();
	gc::Run(-1);
}

void Engine::Setup()
{
	EventManager::Initialize();
	DeferredBufferTracker::Get()->Reset();
	
	m_scenes.Push(mheap::New<MultipleDynamicLayersScene>());

	/*void (*fn)(Handle<Scene>, size_t) = [](Handle<Scene> scene, size_t number) {
		std::cout << "Func called --- " << number << "\n";
	};

	auto func = MakeAsyncFunction(fn, m_scenes[0], 100ull);
	func->Invoke();

	
	auto func1 = MakeAsyncFunction(fn, m_scenes[0], 1000ull)->Then(
		[]()
		{
			std::cout << "Post func called\n";
		}
	);
	func1->Invoke();

	Handle<FunctionBase> fn1 = func1;
	fn1->Invoke();*/

	constexpr float rangeX = 1000;
	constexpr float rangeY = 1000;
	constexpr float rangeZ = 1000;

	constexpr float rangeDimX = 100;
	constexpr float rangeDimY = 100;
	constexpr float rangeDimZ = 100;

	auto mainScene = m_scenes[0].Get();
	mainScene->BeginSetup();
	mainScene->EndSetup();

	for (size_t i = 0; i < 3000; i++)
	{
		auto dynamicObj = mheap::New<GameObject>();
		auto aabb = (AABox*)&dynamicObj->GetAABB();
		*aabb = {
				Vec3(
					Random::RangeFloat(-rangeX, rangeX),
					Random::RangeFloat(-rangeY, rangeY),
					Random::RangeFloat(-rangeZ, rangeZ)
				),

				Vec3(
					Random::RangeFloat(1, rangeDimX),
					Random::RangeFloat(1, rangeDimY),
					Random::RangeFloat(1, rangeDimZ)
				),
		};
		mainScene->AddObject(dynamicObj);
	}
}

void Engine::Run()
{
	Setup();

	auto mainScene = m_scenes[0].Get();

	//while (m_isRunning)
	for (size_t i = 0; i < 100; i++)
	{
		Iteration();

		auto& dynamicObjs = mainScene->m_tempObjects;
		size_t count = 0;
		for (size_t j = 0; j < dynamicObjs.size(); j++)
		{
			//if (Random::RangeInt64(0, 1) == 0)
			{
				mainScene->RefreshDynamicObject(dynamicObjs[j].Get());
				count++;
			}
		}

		std::cout << "Refresh: " << count << " objects\n";
	}

	TaskWorker::Get()->IsRunning() = false;
}

void Engine::Iteration()
{
	if (m_gcIsRunning.exchange(true, std::memory_order_release) == false)
	{
		Task gcTask;
		gcTask.Entry() = [](void* e)
		{
			Engine* engine = (Engine*)e;
			auto heap = mheap::internal::Get();
			if (heap->IsNeedGC())
			{
				gc::Run(-1);
				heap->EndGC();
			}
			engine->m_gcIsRunning.exchange(false, std::memory_order_release);
		};
		gcTask.Params() = this;

		TaskSystem::Submit(gcTask, Task::HIGH);
	}
	

	auto mainScene = m_scenes[0].Get();

	Task tasks[16];


	// process input & reconstruct scene => 2 tasks
	auto& processInput = tasks[0];
	processInput.Params() = this;
	processInput.Entry() = [](void* e)
	{
		auto engine = (Engine*)e;
		engine->ProcessInput();
	};

	auto& reconstructScene = tasks[1];
	reconstructScene.Params() = mainScene;
	reconstructScene.Entry() = [](void* s)
	{
		auto scene = (Scene*)s;
		scene->PrevIteration();
	};

	TaskSystem::SubmitAndWait(tasks, 2, Task::CRITICAL);


	// process rendering, physics, ... => 2 tasks, currently
	auto& rendering = tasks[0];
	rendering.Params() = mainScene;
	rendering.Entry() = [](void* s)
	{
		auto scene = (Scene*)s;
		RenderingSystem::Get()->Process(scene);
	};

	auto& physics = tasks[1];
	physics.Params() = mainScene;
	physics.Entry() = [](void* s)
	{
		auto scene = (Scene*)s;
		PhysicsSystem::Get()->Process(scene);
	};

	TaskSystem::SubmitAndWait(tasks, 2, Task::CRITICAL);


	// SynchronizeAllSubSystems
	SynchronizeAllSubSystems();

}

void Engine::ProcessInput()
{
	std::cout << "ProcessInput()\n";
}

void Engine::SynchronizeAllSubSystems()
{
	auto mainScene = m_scenes[0].Get();

	/*mainScene->m_objectEventMgr->DispatchObjectEvent(
		BUILTIN_EVENT::SCENE_ADD_OBJECT,
		BUILTIN_EVENT_SUIT::SCENE_EVENT,
		nullptr
	);
	mainScene->m_objectEventMgr->DispatchObjectEvent(
		BUILTIN_EVENT::SCENE_REMOVE_OBJECT,
		BUILTIN_EVENT_SUIT::SCENE_EVENT,
		nullptr
	);
	mainScene->m_objectEventMgr->FlushAllObjectEvents();*/

	std::cout << "SynchronizeAllSubSystems()\n";
	DeferredBufferTracker::Get()->UpdateAllThenClear();
}

NAMESPACE_END