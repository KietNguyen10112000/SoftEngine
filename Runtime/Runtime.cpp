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
#include "Scene/Scene.h"

#include "FileSystem/FileSystem.h"

#include "MainSystem/Rendering/Components/Camera.h"
#include "MainSystem/Rendering/Components/Model3DBasicRenderer.h"
#include "MainSystem/Rendering/BuiltinConstantBuffers.h"
#include "MainSystem/Rendering/DisplayService.h"

#include "MainSystem/MainComponentDB.h"
#include "MainSystem/MainComponentList.h"

#include "MainSystem/Scripting/ScriptMeta.h"
#include "MainSystem/Scripting/Components/Script.h"

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

	return ret;
}

void Runtime::Finalize()
{
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

	InitNetwork();
	InitGraphics();
	InitPlugins();

	BuiltinConstantBuffers::SingletonInitialize();
	DisplayService::SingletonInitialize();
	MainComponentDB::SingletonInitialize();
	ScriptMeta::SingletonInitialize();

	MainComponentList::Initialize();
}

Runtime::~Runtime()
{
	ScriptMeta::SingletonFinalize();
	MainComponentDB::SingletonFinalize();
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

class FPPCameraScript : public Script
{
private:
	SCRIPT_DEFAULT_METHOD(FPPCameraScript);

	float m_rotateX = 0;
	float m_rotateY = 0;
	float m_rotateZ = 0;
	Vec3 m_position = {};

	float m_speed = 10;
	float m_rotationSensi = 0.12f;

protected:
	virtual void OnStart() override
	{
		auto transform = GetGameObject()->GetLocalTransform().ToTransformMatrix();

		m_position = transform.Position();
		Vec3 direction = transform.Forward().Normal();
		m_rotateX = asin(direction.y);
		m_rotateY = atan2(direction.x, direction.z);
	}

	virtual void OnUpdate(float dt) override
	{
		auto trans = Mat4::Identity();
		trans *= Mat4::Rotation(Vec3::Y_AXIS, m_rotateY);

		auto right = trans.Right().Normal();
		trans *= Mat4::Rotation(right, -m_rotateX);

		auto forward = trans.Forward().Normal();

		if (m_rotateZ != 0)
		{
			trans *= Mat4::Rotation(forward, m_rotateZ);
		}

		auto d = m_speed * dt;
		if (Input()->IsKeyDown('W'))
		{
			m_position += forward * d;
		}

		if (Input()->IsKeyDown('S'))
		{
			m_position -= forward * d;
		}

		if (Input()->IsKeyDown('A'))
		{
			m_position -= right * d;
		}

		if (Input()->IsKeyDown('D'))
		{
			m_position += right * d;
		}

		if (Input()->IsKeyUp(KEYBOARD::ESC))
		{
			std::cout << "ESC pressed\n";
			Input()->SetCursorLock(!Input()->GetCursorLock());
		}

		if (/*Input()->GetCursorLock() &&*/ Input()->IsCursorMoved())
		{
			auto& delta = Input()->GetDeltaCursorPosition();
			m_rotateY += delta.x * dt * m_rotationSensi;
			m_rotateX += delta.y * dt * m_rotationSensi;

			m_rotateX = std::max(std::min(m_rotateX, PI / 2.0f), -PI / 2.0f);
		}

		/*const auto MOUSE_SPEED = 20;
		if (Input()->IsKeyDown('U'))
		{
			m_rotateY += MOUSE_SPEED * dt * m_rotationSensi;
		}

		if (Input()->IsKeyDown('I'))
		{
			m_rotateY += -MOUSE_SPEED * dt * m_rotationSensi;
		}

		if (Input()->IsKeyDown('O'))
		{
			m_rotateX += MOUSE_SPEED * dt * m_rotationSensi;
			m_rotateX = std::max(std::min(m_rotateX, PI / 2.0f), -PI / 2.0f);
		}

		if (Input()->IsKeyDown('P'))
		{
			m_rotateX += -MOUSE_SPEED * dt * m_rotationSensi;
			m_rotateX = std::max(std::min(m_rotateX, PI / 2.0f), -PI / 2.0f);
		}*/

		auto transform = Transform();
		transform.Position() = m_position;
		transform.Rotation().SetFromMat4(trans);
		SetLocalTransform(transform);
	}

};

// why need this function -> this function is allowed to use fiber-based task system (fiber context switching), 
// meanwhile, Runtime::Initialize(), Runtime constructor is not allowed to do fiber context switching
void Runtime::Setup()
{
	g_timer.Update();
	g_timer.Update();

	DeferredBufferTracker::Get()->Reset();

	auto scene = mheap::New<Scene>(this);
	m_scenes.Push(scene);

	if (scene->BeginSetupLongLifeObject())
	{
		scene->EndSetupLongLifeObject();
	}

	Transform transform = {};

	auto cameraObj = mheap::New<GameObject>();
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
	transform.Position() = { 0,0,5 };
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

byte Runtime::GetNextStableValue()
{
	for (size_t i = 0; i < MAX_RUNNING_SCENES; i++)
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

NAMESPACE_END