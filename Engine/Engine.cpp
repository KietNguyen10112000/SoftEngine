#include "Engine.h"

#include <iostream>

#include "Core/Memory/Memory.h"
#include "Core/Memory/DeferredBuffer.h"

#include "TaskSystem/TaskSystem.h"
#include "TaskSystem/TaskWorker.h"
#include "SubSystems/Physics/PhysicsSystem.h"
#include "SubSystems/Rendering/RenderingSystem.h"

#include "Objects/Scene/Scene.h"
#include "Objects/Scene/DoubleDynamicLayersScene.h"

#include "Objects/Scene/Event/BuiltinEventManager.h"

#include "Objects/Scene/Event/EventManager.h"

#include "Core/Structures/Managed/Function.h"

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
	
	m_scenes.Push(mheap::New<DoubleDynamicLayersScene>());

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
}

void Engine::Run()
{
	Setup();

	//while (m_isRunning)
	{
		Iteration();
	}

	TaskWorker::Get()->IsRunning() = false;
}

void Engine::Iteration()
{
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