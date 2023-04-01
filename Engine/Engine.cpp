#include "Engine.h"

#include <iostream>

#include "Core/Memory/Memory.h"
#include "Core/Memory/DeferredBuffer.h"
#include "Core/Structures/Managed/Function.h"
#include "Core/Random/Random.h"
#include "Core/Input/Input.h"

#include "Platform/Platform.h"

#include "TaskSystem/TaskSystem.h"
#include "TaskSystem/TaskWorker.h"

#include "Objects/Scene/Scene.h"
#include "Objects/Scene/MultipleDynamicLayersScene.h"
#include "Objects/Scene/Event/BuiltinEventManager.h"
#include "Objects/Scene/Event/EventManager.h"


#include "Components/Script/Script.h"


NAMESPACE_BEGIN

Handle<Engine> Engine::Initialize()
{
	auto old = mheap::internal::GetStableValue();
	mheap::internal::SetStableValue(Engine::STABLE_VALUE);
	auto ret = mheap::New<Engine>();
	mheap::internal::SetStableValue(old);
	ret->Setup();
	return ret;
}

void Engine::Finalize()
{
	mheap::internal::FreeStableObjects(Engine::STABLE_VALUE, 0, 0);
	EventManager::Finalize();
	gc::Run(-1);
}

Engine::Engine()
{
	m_input = rheap::New<Input>();
	m_window = (void*)platform::CreateWindow(m_input, 0, 0, -1, -1, "SoftEngine");
}

Engine::~Engine()
{
	platform::DeleteWindow(m_window);
	rheap::Delete(m_input);
}

void Engine::Setup()
{
	EventManager::Initialize();
	DeferredBufferTracker::Get()->Reset();

	m_scenes.Push(mheap::New<MultipleDynamicLayersScene>(this));

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

	for (size_t i = 0; i < 200; i++)
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

	/*auto dynamicObj = mheap::New<GameObject>();
	auto aabb = (AABox*)&dynamicObj->GetAABB();
	*aabb = {};*/

	class MyScript : public Script
	{
	public:
		~MyScript()
		{
			std::cout << "MyScript::~MyScript()\n";
		}

		virtual void OnStart() override
		{
			std::cout << "MyScript::OnStart()\n";
		}

		virtual void OnUpdate(float dt) override
		{
			//std::cout << "MyScript::OnUpdate()\n";
			if (Input()->IsKeyPressed('A'))
			{
				std::cout << "Pressed\n";
			}
		}

	};

	auto object = mheap::New<GameObject>();
	auto aabb = (AABox*)&object->GetAABB();
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
	auto script = object->NewComponent<MyScript>();
	mainScene->AddObject(object);
}

void Engine::Run()
{
	//Setup();

	auto mainScene = m_scenes[0].Get();

	while (m_isRunning)
	//for (size_t i = 0; i < 100; i++)
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

		//std::cout << "Refresh: " << count << " objects\n";
	}

	TaskWorker::Get()->IsRunning() = false;
}

void Engine::Iteration()
{
	if (m_gcIsRunning.exchange(true, std::memory_order_acquire) == false)
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

	mainScene->Iteration();

	// SynchronizeAllSubSystems
	SynchronizeAllSubSystems();
}

void Engine::ProcessInput()
{
	//std::cout << "ProcessInput()\n";
	m_input->RollEvent();
	m_isRunning = !platform::ProcessPlatformMsg(m_window);

	Thread::Sleep(8);
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

	mainScene->PostIteration();

	//std::cout << "SynchronizeAllSubSystems()\n";
	DeferredBufferTracker::Get()->UpdateAllThenClear();
}

NAMESPACE_END