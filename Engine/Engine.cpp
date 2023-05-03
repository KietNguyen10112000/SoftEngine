#include "Engine.h"

#include <iostream>

#include "Core/Memory/Memory.h"
#include "Core/Memory/DeferredBuffer.h"
#include "Core/Structures/Managed/Function.h"
#include "Core/Random/Random.h"

#include "Platform/Platform.h"

#include "TaskSystem/TaskSystem.h"
#include "TaskSystem/TaskWorker.h"
#include "TaskSystem/TaskUtils.h"

#include "Input/Input.h"
#include "Graphics2D/Graphics2D.h"
#include "Network/Network.h"

#include "Plugins/Plugin.h"
#include "Plugins/PluginLoader.h"

#include "StartupConfig.h"
#include "ENGINE_EVENT.h"

#include "Components2D/Script/Script2D.h"
#include "Components2D/Rendering/Sprite.h"
#include "Components2D/Rendering/Camera2D.h"

#include "Objects2D/Scene2D/UniqueGridScene2D.h"
#include "Objects2D/GameObject2D.h"

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
	gc::Run(-1);
}

Engine::Engine() : EventDispatcher(ENGINE_EVENT::COUNT)
{
	m_eventArgv[0] = this;
	InitNetwork();
	InitGraphics();
	InitPlugins();
}

Engine::~Engine()
{
	FinalPlugins();
	FinalGraphics();
	FinalNetwork();
}

void Engine::InitGraphics()
{
	if (StartupConfig::Get().isEnableRendering)
	{
		m_input = rheap::New<Input>();
		//m_window = (void*)platform::CreateWindow(m_input, 0, 0, -1, -1, "SoftEngine");
		if (Graphics2D::Initilize(m_window, 960, 720) != 0)
		{
			m_isRunning = false;
		}
		platform::BindInput(m_input, m_window->getSystemHandle());
	}
}

void Engine::FinalGraphics()
{
	if (StartupConfig::Get().isEnableRendering)
	{
		Graphics2D::Finalize();
		platform::DeleteWindow(m_window);
		rheap::Delete(m_input);
	}
}

void Engine::InitNetwork()
{
	if (StartupConfig::Get().isEnableNetwork)
	{
		Network::Initialize();
	}
}

void Engine::FinalNetwork()
{
	if (StartupConfig::Get().isEnableNetwork)
	{
		Network::Finalize();
	}
}

void Engine::InitPlugins()
{
	if (StartupConfig::Get().pluginsPath)
	{
		if (PluginLoader::LoadAll(this, StartupConfig::Get().pluginsPath, m_plugins) == false)
		{
			m_isRunning = false;
		}
		else
		{
			PLUGIN_DESC desc;
			for (auto& plugin : m_plugins)
			{
				plugin->GetDesc(&desc);
				switch (desc.type)
				{
				case PLUGIN_TYPE::INTERVAL:
					m_intevalPlugins.push_back(plugin);
					break;
				default:
					break;
				}
			}
		}

	}
}

void Engine::FinalPlugins()
{
	PluginLoader::UnloadAll(this, m_plugins);
}


void Engine::Setup()
{
	DeferredBufferTracker::Get()->Reset();

	auto scene = mheap::New<UniqueGridScene2D>(this, 128, 128, 60, 60);
	scene->Setup();
	scene->m_id = 0;
	m_scenes.Push(scene);

	Dispatch(ENGINE_EVENT::SCENE_ON_INITIALIZE, this, scene.Get());

	auto mainScene = m_scenes[0].Get();
	mainScene->BeginSetup();
	Dispatch(ENGINE_EVENT::SCENE_ON_SETUP, this, scene.Get());
	mainScene->EndSetup();

	Dispatch(ENGINE_EVENT::SCENE_ON_START, this, scene.Get());

	class CameraScript : public Script2D
	{
	public:
		float m_speed = 100;

		virtual void OnUpdate(float dt) override
		{
			if (Input()->IsKeyDown('W'))
			{
				Position().y += m_speed * dt;
			}

			if (Input()->IsKeyDown('S'))
			{
				Position().y -= m_speed * dt;
			}

			if (Input()->IsKeyDown('A'))
			{
				Position().x += m_speed * dt;
			}

			if (Input()->IsKeyDown('D'))
			{
				Position().x -= m_speed * dt;
			}
		}

	};

	{
		auto object = mheap::New<GameObject2D>();
		object->NewComponent<Camera2D>(AARect({ 0,0 }, { 960, 720 }));
		object->NewComponent<CameraScript>();
		object->SetType(GameObject2D::DYNAMIC);

		object->Position() = { 800 / 2, 600 / 2 };

		mainScene->AddObject(object);
	}

	{
		constexpr static std::array<std::array<size_t, 6>, 6> mapValue = {{
			{ 1, 1, 1, 1, 1, 1 },
			{ 1, 0, 0, 0, 0, 1 },
			{ 1, 0, 0, 0, 0, 1 },
			{ 1, 0, 0, 0, 0, 1 },
			{ 1, 0, 0, 0, 0, 1 },
			{ 1, 1, 1, 1, 1, 1 }
		}};

		for (size_t y = 0; y < mapValue.size(); y++)
		{
			auto& row = mapValue[y];
			for (size_t x = 0; x < row.size(); x++)
			{
				auto object = mheap::New<GameObject2D>();
				object->NewComponent<Sprite>(String::Format("Resources/{}.png", row[x]).c_str(), AARect({ 0, 0 }, { 50, 50 }));
				object->SetType(GameObject2D::STATIC);

				object->Position() = { x * 50, y * 50 };

				mainScene->AddObject(object);
			}
		}

	}

}

void Engine::Run()
{
	//Setup();

	auto mainScene = m_scenes[0].Get();

	while (m_isRunning)
	{
		Iteration();
	}

	TaskWorker::Get()->IsRunning() = false;
}

void Engine::Iteration()
{
	static TaskWaitingHandle taskHandle = { 0, 0 };

	if (m_gcIsRunning.load(std::memory_order_relaxed) == false 
		&& m_gcIsRunning.exchange(true, std::memory_order_acquire) == false)
	{
		Task gcTask;
		gcTask.Entry() = [](void* e)
		{
			Engine* engine = (Engine*)e;
			auto heap = mheap::internal::Get();
			if (heap->IsNeedGC())
			{
				std::cout << "GC started...\n";
				gc::Run(-1);
				std::cout << "GC end...\n";
				heap->EndGC();
			}
			engine->m_gcIsRunning.exchange(false, std::memory_order_release);
		};
		gcTask.Params() = this;

		TaskSystem::Submit(gcTask, Task::HIGH);
	}

	auto mainScene = m_scenes[0].Get();

	ProcessInput();
	mainScene->PrevIteration();
	mainScene->Iteration();

	// SynchronizeAllSubSystems
	SynchronizeAllSubSystems();
}

void Engine::ProcessInput()
{
	m_input->RollEvent();

	auto& window = *m_window;
	sf::Event event;
	while (window.pollEvent(event))
	{
		// "close requested" event: we close the window
		if (event.type == sf::Event::Closed)
			window.close();
	}
	
	m_isRunning = (!platform::ProcessPlatformMsg(m_window)) && (window.isOpen());
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
	//DeferredBufferTracker::Get()->UpdateAllThenClear();

	//Graphics::Get()->Present(1, 0);

	auto tracker = DeferredBufferTracker::Get();
	tracker->UpdateCustomBegin();
	TaskUtils::ForEachConcurrentList(
		tracker->m_buffers, 
		[](DeferredBufferState* state, ID) 
		{
			state->Update();
		}, 
		TaskSystem::GetWorkerCount()
	);
	tracker->UpdateCustomEnd();
}

NAMESPACE_END