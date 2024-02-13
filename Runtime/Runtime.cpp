#include "Runtime.h"

#include <iostream>

#include "Core/Memory/Memory.h"
#include "Core/Structures/Managed/Function.h"
#include "Core/Random/Random.h"

#include "Platform/Platform.h"

#include "TaskSystem/TaskSystem.h"
#include "TaskSystem/TaskWorker.h"
#include "TaskSystem/TaskUtils.h"

#include "Input/Input.h"
#include "Graphics/Graphics.h"
#include "Graphics/DebugGraphics.h"

#include "PhysX/PhysX.h"

#include "Network/Network.h"
#include "Resources/Resource.h"

#include "Plugins/Plugin.h"
#include "Plugins/PluginLoader.h"

#include "StartupConfig.h"
#include "RUNTIME_EVENT.h"

#include "Common/Base/MetadataUtils.h"
#include "Common/Base/Metadata.h"
#include "Common/Base/SerializableDB.h"

#include "Resources/Utils/Utils.h"

//#include "Network/TCPAcceptor.h"
//#include "Network/TCPConnector.h"

#include "imgui.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "FileSystem/FileSystem.h"

#include "MainSystem/Rendering/Components/Camera.h"
#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"
#include "MainSystem/Rendering/BuiltinConstantBuffers.h"
#include "MainSystem/Rendering/DisplayService.h"

#include "MainSystem/Physics/Components/RigidBodyStatic.h"
#include "MainSystem/Physics/Components/RigidBodyDynamic.h"
#include "MainSystem/Physics/Components/CharacterControllerCapsule.h"
#include "MainSystem/Physics/Shapes/PhysicsShapePlane.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeBox.h"
#include "MainSystem/Physics/Materials/PhysicsMaterial.h"

#include "SerializableList.h"

#include "MainSystem/Scripting/ScriptMeta.h"
#include "MainSystem/Scripting/Components/FPPCameraScript.h"
#include "MainSystem/Scripting/Components/TestScript.h"
#include "MainSystem/Physics/PhysicsSystem.h"

#include "Common/Base/Serializer.h"

#include "Scene/GameObjectCache.h"

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

	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (debugGraphics)
	{
		rheap::Delete(debugGraphics);
		Graphics::Get()->m_debugGraphics = nullptr;
	}

	resource::internal::Finalize();

	Graphics::Finalize();
	PhysX::SingletonFinalize();
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
	SerializableDB::SingletonInitialize();
	ScriptMeta::SingletonInitialize();

	InitNetwork();
	InitGraphics();
	PhysX::SingletonInitialize();
	InitPlugins();

	BuiltinConstantBuffers::SingletonInitialize();
	DisplayService::SingletonInitialize();

	SerializableList::Initialize();

	m_gameObjectCache = mheap::New<class GameObjectCache>();
}

void Runtime::FinalizeModules()
{
	DisplayService::SingletonFinalize();
	BuiltinConstantBuffers::SingletonFinalize();

	FinalPlugins();
	FinalGraphics();
	FinalNetwork();

	ScriptMeta::SingletonFinalize();
	SerializableDB::SingletonFinalize();
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

	/*transform = {};
	transform.Position() = { 0,5,5 };
	auto object = mheap::New<GameObject>();
	object->SetLocalTransform(transform);
	object->NewComponent<MeshBasicRenderer>();
	scene->AddObject(object);

	transform = {};
	transform.Position() = { -5,0,5 };
	object = mheap::New<GameObject>();
	object->SetLocalTransform(transform);
	object->NewComponent<MeshBasicRenderer>();
	scene->AddObject(object);

	transform = {};
	transform.Position() = { 5,0,5 };
	object = mheap::New<GameObject>();
	object->SetLocalTransform(transform);
	object->NewComponent<MeshBasicRenderer>();
	scene->AddObject(object);*/

	//transform = {};
	//transform.Scale() = { 0.03f,0.03f,0.03f };
	//auto object = ResourceUtils::LoadModel3DBasic("model/robot/white_robot.glb", "model/robot/white_robot_albedo.png");
	//auto object = ResourceUtils::LoadModel3DBasic("model/globin/globin.fbx", "model/globin/textures/lowRes/character diffuse.png");
	//auto object = ResourceUtils::LoadModel3DBasic("Default/cube1.obj");
	//object->SetLocalTransform(transform);
	//scene->AddObject(object);

	//transform = {};
	//transform.Scale() = { 0.01f,0.01f,0.01f };
	//auto object = ResourceUtils::LoadAnimModelArray("model/globin/globin2.fbx", "model/globin/textures/lowRes/character diffuse.png");
	////auto object = ResourceUtils::LoadAnimModel("model/simple/Character Running.fbx", "model/simple/Character Texture 256x256.png");
	////auto object = ResourceUtils::LoadAnimModelArray("model/robot/white_robot.glb", "model/robot/white_robot_albedo.png");
	////auto object = ResourceUtils::LoadModel3DBasic("Default/cube1.obj");
	////auto object = ResourceUtils::LoadAnimModelArray("model/vampires/dancing_vampire.dae", "model/vampires/Vampire_diffuse.png");
	//object->SetLocalTransform(transform);
	//scene->AddObject(object);

	/*object = ResourceUtils::LoadAnimModel("model/robot/white_robot.glb", "model/robot/white_robot_albedo.png");
	scene->AddObject(object);

	object = ResourceUtils::LoadAnimModelArray("model/robot/white_robot.glb", "model/robot/white_robot_albedo.png");
	scene->AddObject(object);*/

	/*constexpr int64_t NUM = 2;

	for (int64_t y = -NUM / 2; y < NUM / 2; y++)
	{
		for (int64_t x = -NUM / 2; x < NUM / 2; x++)
		{
			Serializer serializer;
			auto cloned = StaticCast<GameObject>(serializer.Clone(object.Get()));

			transform = {};
			transform.Scale() = Vec3(0.01f);
			transform.Position() = { x * 3,0,y * 3 };

			cloned->SetLocalTransform(transform);

			scene->AddObject(cloned);
		}
	}*/

	//transform = {};
	//transform.Position() = { 0,0,0 };
	////transform.Scale() = { 10,1,10 };
	//auto object = mheap::New<GameObject>();
	//object->SetLocalTransform(transform);
	//object->NewComponent<MeshBasicRenderer>();

	//{
	//	transform = {};
	//	transform.Position() = { 0,-5,0 };
	//	auto child1 = mheap::New<GameObject>();
	//	child1->SetLocalTransform(transform);
	//	child1->NewComponent<MeshBasicRenderer>();
	//	object->AddChild(child1);

	//	transform = {};
	//	transform.Position() = { 0,-5,0 };
	//	auto child2 = mheap::New<GameObject>();
	//	child2->SetLocalTransform(transform);
	//	child2->NewComponent<MeshBasicRenderer>();
	//	child1->AddChild(child2);
	//}
	//
	//scene->AddObject(object);

	/*{
		auto obj1 = mheap::New<GameObject>();
		obj1->NewComponent<MeshBasicRenderer>();

		auto obj2 = mheap::New<GameObject>();
		obj2->NewComponent<MeshBasicRenderer>();

		transform = {};
		transform.Position() = { 0,5,0 };
		obj2->SetLocalTransform(transform);

		obj1->AddChild(obj2);

		scene->AddObject(obj1);
	}*/


	auto material = std::make_shared<PhysicsMaterial>(0.9f, 0.9f, 0.6f);

	{
		auto obj = mheap::New<GameObject>();
		obj->NewComponent<MeshBasicRenderer>("Default/cube1.obj", "Default/white.png");

		auto shape = std::make_shared<PhysicsShapePlane>(material);
		obj->NewComponent<RigidBodyStatic>(shape);

		transform = {};
		transform.Scale() = { 0.01f, 100.f, 100.f };
		transform.Rotation() = Mat4::Rotation(Vec3::Z_AXIS, PI / 2);
		obj->SetLocalTransform(transform);

		scene->AddObject(obj);
	}

	{
		auto obj = mheap::New<GameObject>();
		obj->NewComponent<MeshBasicRenderer>("Default/cube1.obj", "Default/green.png");

		auto shape = std::make_shared<PhysicsShapeBox>(Vec3(5.0f, 5.0f, 16.0f), material);
		obj->NewComponent<RigidBodyStatic>(shape);

		transform = {};
		transform.Scale() = Vec3(2.5f, 2.5f, 8.0f);
		transform.Position() = { 10, 0.0f, 10 };
		transform.Rotation() = Mat4::Rotation(Vec3::X_AXIS, -PI / 6.0f);
		//transform.Rotation() = Mat4::Rotation(Vec3::Z_AXIS, PI / 2);
		obj->SetLocalTransform(transform);

		scene->AddObject(obj);
	}

	for (size_t y = 0; y < 1; y++)
	{
		for (size_t x = 0; x < 1; x++)
		{
			auto obj = mheap::New<GameObject>();
			obj->NewComponent<MeshBasicRenderer>("Default/cube1.obj", "Default/green.png");

			auto shape = std::make_shared<PhysicsShapeBox>(Vec3(5.0f, 5.0f, 5.0f), material);
			auto dyn = obj->NewComponent<RigidBodyDynamic>(shape);//->SetPhysicsFlag(PHYSICS_FLAG_ENABLE_COLLISION, true);
			dyn->SetPhysicsFlag(PHYSICS_FLAG_ENABLE_COLLISION, true);
			dyn->SetMass(100);

			//obj->NewComponent<TestScript2>();

			transform = {};
			transform.Scale() = Vec3(2.5f);
			transform.Position() = { x * 5.0f, 2.5f, y * 5.0f };
			//transform.Rotation() = Mat4::Rotation(Vec3::Z_AXIS, PI / 2);
			obj->SetLocalTransform(transform);

			scene->AddObject(obj);
		}
	}

	{
		auto obj = mheap::New<GameObject>();
		//obj->NewComponent<MeshBasicRenderer>("Default/capsule.obj", "Default/green.png");

		{
			CharacterControllerCapsuleDesc desc = {};
			desc.capsule = Capsule(Vec3::ZERO + Vec3::UP, 1.0f, 0.5f);
			desc.material = material;
			auto cct = obj->NewComponent<CharacterControllerCapsule>(scene, desc);
			cct->SetPhysicsFlag(PHYSICS_FLAG_ENABLE_COLLISION, true);

			obj->NewComponent<TestScript>();

			transform = {};
			transform.Position() = Vec3::ZERO + Vec3::UP;
			transform.Position().y = 100;
			//transform.Position().z = 10;
			obj->SetLocalTransform(transform);

			scene->AddObject(obj);

			cct->SetGravity(scene->GetPhysicsSystem()->GetGravity());
		}
	}

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

void* Runtime::GetNativeHWND()
{
	return platform::GetWindowNativeHandle(m_window);
}

NAMESPACE_END