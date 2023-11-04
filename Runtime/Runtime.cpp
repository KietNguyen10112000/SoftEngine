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
#include "Common/Base/SerializableDB.h"

//#include "Network/TCPAcceptor.h"
//#include "Network/TCPConnector.h"

#include "imgui.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "FileSystem/FileSystem.h"

#include "MainSystem/Rendering/Components/Camera.h"
#include "MainSystem/Rendering/Components/Model3DBasicRenderer.h"
#include "MainSystem/Rendering/BuiltinConstantBuffers.h"
#include "MainSystem/Rendering/DisplayService.h"

#include "SerializableList.h"

#include "MainSystem/Scripting/ScriptMeta.h"
#include "MainSystem/Scripting/Components/FPPCameraScript.h"

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
	FileSystem::Initialize();
	MetadataParser::Initialize();
	resource::internal::Initialize();

	auto old = mheap::internal::GetStableValue();
	mheap::internal::SetStableValue(Runtime::STABLE_VALUE);
	auto ret = mheap::New<Runtime>();
	mheap::internal::SetStableValue(old);

	Runtime::s_instance.reset(ret.Get());

	Runtime::s_instance->InitializeModules();

	return ret;
}

void Runtime::Finalize()
{
	Runtime::s_instance->FinalizeModules();
	Runtime::s_instance.release();
	mheap::internal::FreeStableObjects(Runtime::STABLE_VALUE, 0, 0);
	for (size_t i = 0; i < 5; i++)
	{
		gc::Run(-1);
	}
	resource::internal::Finalize();

	Graphics::Finalize();
	MetadataParser::Finalize();
	FileSystem::Finalize();
}

Runtime::Runtime() : m_eventDispatcher(this)
{
	m_eventArgv[0] = this;
}

Runtime::~Runtime()
{
}

void Runtime::InitializeModules()
{
	InitNetwork();
	InitGraphics();
	InitPlugins();

	BuiltinConstantBuffers::SingletonInitialize();
	DisplayService::SingletonInitialize();
	SerializableDB::SingletonInitialize();
	ScriptMeta::SingletonInitialize();

	SerializableList::Initialize();
}

void Runtime::FinalizeModules()
{
	ScriptMeta::SingletonFinalize();
	SerializableDB::SingletonFinalize();
	DisplayService::SingletonFinalize();
	BuiltinConstantBuffers::SingletonFinalize();

	FinalPlugins();
	FinalGraphics();
	FinalNetwork();
}

void Runtime::InitGraphics()
{
	if (StartupConfig::Get().isEnableRendering)
	{
		/*SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		::SetProcessDPIAware();*/

		m_input = rheap::New<Input>();

		m_window = (void*)platform::CreateWindow(m_input, 0, 0, StartupConfig::Get().windowWidth, StartupConfig::Get().windowHeight, "SoftEngine");
		if (Graphics::Initilize(platform::GetWindowNativeHandle(m_window), GRAPHICS_BACKEND_API::DX12) != 0)
		{
			m_isRunning = false;
		}

		platform::ProcessPlatformMsg(m_window);
		platform::ProcessPlatformMsg(m_window);
	}
}

void Runtime::FinalGraphics()
{
	if (StartupConfig::Get().isEnableRendering)
	{
		//Graphics::Finalize();
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

// why need this function -> this function is allowed to use fiber-based task system (fiber context switching), 
// meanwhile, Runtime::Initialize(), Runtime constructor is not allowed to do fiber context switching
void Runtime::Setup()
{
	g_timer.Update();
	g_timer.Update();

	DeferredBufferTracker::Get()->Reset();

	auto scene = CreateScene();
	if (scene->BeginSetupLongLifeObject())
	{
		scene->EndSetupLongLifeObject();
	}

	SetRunningScene(scene.Get());

	Transform transform = {};

	auto cameraObj = mheap::New<GameObject>();
	cameraObj->Name() = "#camera";
	cameraObj->NewComponent<FPPCameraScript>();
	auto camera = cameraObj->NewComponent<Camera>();
	camera->Projection().SetPerspectiveFovLH(
		PI / 3.0f,
		Graphics::Get()->GetWindowWidth() / (float)Graphics::Get()->GetWindowHeight(),
		0.5f,
		1000.0f
	);
	scene->AddObject(cameraObj);

	/*auto rotationMat = Mat4::Rotation(Vec3::UP, PI / 3.0f);
	transform.Position() = { -5,-5,-5 };
	transform.Rotation() = Quaternion(rotationMat);
	cameraObj = mheap::New<GameObject>();
	cameraObj->SetTransform(transform);
	camera = cameraObj->NewComponent<Camera>();
	camera->Projection().SetPerspectiveFovLH(
		PI / 3.0f,
		Graphics::Get()->GetWindowWidth() / (float)Graphics::Get()->GetWindowHeight(),
		0.5f,
		1000.0f
	);
	scene->AddObject(cameraObj);*/

	transform = {};
	transform.Position() = { 0,5,5 };
	auto object = mheap::New<GameObject>();
	object->SetLocalTransform(transform);
	object->NewComponent<Model3DBasicRenderer>("cube.obj", "2.png");
	scene->AddObject(object);

	transform = {};
	transform.Position() = { -5,0,5 };
	object = mheap::New<GameObject>();
	object->SetLocalTransform(transform);
	object->NewComponent<Model3DBasicRenderer>("cube.obj", "2.png");
	scene->AddObject(object);

	transform = {};
	transform.Position() = { 5,0,5 };
	object = mheap::New<GameObject>();
	object->SetLocalTransform(transform);
	object->NewComponent<Model3DBasicRenderer>("cube.obj", "2.png");
	scene->AddObject(object);
}

void Runtime::Run()
{
	while (m_isRunning)
	{
		if (m_runningSceneIdx != m_nextRunningSceneIdx)
		{
			if (m_runningSceneIdx != INVALID_ID)
			{
				m_scenes[m_runningSceneIdx]->EndRunning();
			}

			m_runningSceneIdx = m_nextRunningSceneIdx;

			if (m_runningSceneIdx != INVALID_ID)
			{
				m_scenes[m_runningSceneIdx]->BeginRunning();
			}
		}

		if (m_destroyingScenesCount != 0)
		{
			ProcessDestroyScenes();
		}

		Iteration();
		//Thread::Sleep(1);
	}

	TaskWorker::Get()->IsRunning() = false;

	while (m_gcIsRunning.load(std::memory_order_relaxed))
	{
		Thread::Sleep(100);
	}
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
			auto rheap = rheap::internal::Get();
			auto sheap = mheap::internal::GetStableHeap();
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

	//std::cout << "Iteration [thread id: " << Thread::GetID() << ", fiber id: " << Thread::GetCurrentFiberID() << "]\n";

	// dynamic submit wait
	TaskSystem::PrepareHandle(&taskHandle);

	// process input task must execute on main thread 
	// the thread create the Window - this is required for win32 messeges queue
	// win32 messeges queue is attach with thread that create HWND
	Task processInput = {};
	processInput.Params() = this;
	processInput.Entry() = [](void* e)
	{
		static bool isFirstIteration = true;

		auto engine = (Runtime*)e;

		if (isFirstIteration) 
		{
			isFirstIteration = false;
		}
		else 
		{
			Graphics::Get()->Present(true);
		}

		engine->ProcessInput();

		//std::cout << "ProcessInput [thread id: " << Thread::GetID() << ", fiber id: " << Thread::GetCurrentFiberID() << "]\n";
	};

	// 0 is main thread id
	TaskSystem::SubmitForThread(&taskHandle, 0, processInput);

	// wait
	TaskSystem::WaitForHandle(&taskHandle);

	g_timer.Update();

	if (m_runningSceneIdx == INVALID_ID)
	{
		return;
	}

	auto& mainScene = GetCurrentRunningScene();

	g_sumDt += g_timer.dt;

	if (m_iterationHandler)
	{
		g_sumDt = m_iterationHandler->DoIteration(g_sumDt, mainScene);
		//SynchronizeAllSubSystems();
		return;
	}

	auto fixedDt = StartupConfig::Get().fixedDt;
	if (fixedDt > 0)
	{
		while (g_sumDt > fixedDt)
		{
			mainScene->Iteration(fixedDt);
			g_sumDt -= fixedDt;
		}

		return;
	}

	mainScene->Iteration(g_sumDt);
	g_sumDt = 0;
	

	// SynchronizeAllSubSystems
	//SynchronizeAllSubSystems();
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

	/*auto tracker = DeferredBufferTracker::Get();
	tracker->UpdateCustomBegin();
	TaskUtils::ForEachConcurrentList(
		tracker->m_buffers, 
		[](DeferredBufferState* state, ID) 
		{
			state->Update();
		}, 
		TaskSystem::GetWorkerCount()
	);
	tracker->UpdateCustomEnd();*/
}

void Runtime::ProcessDestroyScenes()
{
	for (size_t i = 0; i < m_destroyingScenesCount; i++)
	{
		Task task;
		task.Entry() = [](void* p)
		{
			auto scene = (Scene*)p;
			Runtime::Get()->DestroySceneImpl(scene);
		};
		task.Params() = m_scenes[m_destroyingScenes[i]].Get();

		TaskSystem::Submit(task, Task::CRITICAL);
		//DestroySceneImpl(m_scenes[m_destroyingScenes[i]].Get());
	}

	m_destroyingScenesCount = 0;
}

byte Runtime::GetNextStableValue()
{
	for (size_t i = 1; i < MAX_RUNNING_SCENES + 1; i++)
	{
		if (!m_runningSceneStableValue.test(i))
		{
			m_runningSceneStableValue.set(i, true);
			return (byte)i;
		}
	}

	assert(0 && "Too much running scene");
	return 256;
}

void Runtime::DestroySceneImpl(Scene* scene)
{
	ID id = scene->m_runtimeID;

	scene->CleanUp();
	scene->m_runtimeID = INVALID_ID;
	scene->m_stableValue = 0;

	m_createSceneLock.lock();

	EventDispatcher()->Dispatch(EVENT::EVENT_SCENE_DESTROYED, scene);
	m_runningSceneStableValue.set(id, false);
	m_scenes[id] = nullptr;

	m_createSceneLock.unlock();
}

Handle<Scene> Runtime::CreateScene()
{
	m_createSceneLock.lock();

	auto scene = mheap::New<Scene>();
	auto id = GetNextStableValue();
	scene->m_runtimeID = id;
	scene->m_stableValue = id;

	m_scenes[id] = scene;

	EventDispatcher()->Dispatch(EVENT::EVENT_SCENE_CREATED, scene.Get());

	m_createSceneLock.unlock();
	return scene;
}

void Runtime::DestroyScene(Scene* scene)
{
	if (scene->m_destroyed || scene->m_runtimeID == INVALID_ID)
	{
		return;
	}

	m_createSceneLock.lock();

	scene->m_destroyed = true;
	m_destroyingScenes[m_destroyingScenesCount++] = scene->m_runtimeID;

	m_createSceneLock.unlock();

	if (scene->m_runtimeID == m_runningSceneIdx)
	{
		m_nextRunningSceneIdx = INVALID_ID;
	}
}

void Runtime::SetRunningScene(Scene* scene)
{
	m_nextRunningSceneIdx = scene->m_runtimeID;
}

NAMESPACE_END