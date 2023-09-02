#include "Runtime.h"

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
#include "Graphics/Graphics.h"
#include "Network/Network.h"
#include "Resources/Resource.h"

#include "Plugins/Plugin.h"
#include "Plugins/PluginLoader.h"

#include "StartupConfig.h"
#include "RUNTIME_EVENT.h"

#include "Common/Base/MetadataUtils.h"
#include "Common/Base/Metadata.h"

//#include "Network/TCPAcceptor.h"
//#include "Network/TCPConnector.h"

#include "imgui.h"

#include "Scene/GameObject.h"

NAMESPACE_BEGIN

struct Timer
{
	size_t prevTimeSinceEpoch;
	size_t curTimeSinceEpoch;
	float dt;

	inline auto Update()
	{
		prevTimeSinceEpoch = curTimeSinceEpoch;
		curTimeSinceEpoch = Clock::ms::now();
		dt = (curTimeSinceEpoch - prevTimeSinceEpoch) / 1'000.0f;
	}
};

Timer g_timer;
float g_sumDt = 0;

Handle<Runtime> Runtime::Initialize()
{
	resource::internal::Initialize();
	auto old = mheap::internal::GetStableValue();
	mheap::internal::SetStableValue(Runtime::STABLE_VALUE);
	auto ret = mheap::New<Runtime>();
	mheap::internal::SetStableValue(old);
	ret->Setup();
	return ret;
}

void Runtime::Finalize()
{
	mheap::internal::FreeStableObjects(Runtime::STABLE_VALUE, 0, 0);
	gc::Run(-1);
	resource::internal::Finalize();
}

Runtime::Runtime()
{
	m_eventArgv[0] = this;

	MetadataParser::Initialize();

	InitNetwork();
	InitGraphics();
	InitPlugins();
}

Runtime::~Runtime()
{
	FinalPlugins();
	FinalGraphics();
	FinalNetwork();

	MetadataParser::Finalize();
}

void Runtime::InitGraphics()
{
	if (StartupConfig::Get().isEnableRendering)
	{
		/*SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		::SetProcessDPIAware();*/

		m_input = rheap::New<Input>();

		m_window = (void*)platform::CreateWindow(m_input, 0, 0, -1, -1, "SoftEngine");
		if (Graphics::Initilize(platform::GetWindowNativeHandle(m_window), GRAPHICS_BACKEND_API::DX12) != 0)
		{
			m_isRunning = false;
		}
	}
}

void Runtime::FinalGraphics()
{
	if (StartupConfig::Get().isEnableRendering)
	{
		Graphics::Finalize();
		platform::DeleteWindow(m_window);
		rheap::Delete(m_input);
	}
}

void Runtime::InitNetwork()
{
	if (StartupConfig::Get().isEnableNetwork)
	{
		Network::Initialize();
	}
}

void Runtime::FinalNetwork()
{
	if (StartupConfig::Get().isEnableNetwork)
	{
		Network::Finalize();
	}
}

void Runtime::InitPlugins()
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

void Runtime::FinalPlugins()
{
	PluginLoader::UnloadAll(this, m_plugins);
}


void Runtime::Setup()
{
	g_timer.Update();
	g_timer.Update();

	DeferredBufferTracker::Get()->Reset();

	int v = 0;
	auto accessor = Accessor::For("Mat4", v, 0);
	accessor.Set(
		"1.2"
	);
	std::cout << accessor.Get() << "\n";
	//exit(0);

	m_isRunning = false;
}

void Runtime::Run()
{
	if (m_scenes.size() != 0)
	{
		auto mainScene = m_scenes[0].Get();

		while (m_isRunning)
		{
			Iteration();
			Thread::Sleep(1);
		}
	}

	TaskWorker::Get()->IsRunning() = false;
}

void Runtime::Iteration()
{
	static TaskWaitingHandle taskHandle = { 0, 0 };

	TaskSystem::InvokeAllWaitWorkers();
	if (m_gcIsRunning.load(std::memory_order_relaxed) == false 
		&& m_gcIsRunning.exchange(true, std::memory_order_acquire) == false)
	{
		Task gcTask;
		gcTask.Entry() = [](void* e)
		{
			Runtime* engine = (Runtime*)e;
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

	g_timer.Update();

	g_sumDt += g_timer.dt;

	if (m_iterationHandler)
	{
		g_sumDt = m_iterationHandler->DoIteration(g_sumDt, mainScene);
		SynchronizeAllSubSystems();
		return;
	}

	auto fixedDt = StartupConfig::Get().fixedDt;

	while (g_sumDt > fixedDt)
	{
		//mainScene->Iteration();
		g_sumDt -= fixedDt;
	}

	// SynchronizeAllSubSystems
	SynchronizeAllSubSystems();
}

void Runtime::ProcessInput()
{
	if (!m_input) return;

	m_input->RollEvent();

	m_isRunning = (!platform::ProcessPlatformMsg(m_window));
}

void Runtime::SynchronizeAllSubSystems()
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