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
#include "UUID/UUID.h"
#include "JSON/JSON.h"

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
#include "MainSystem/Rendering/Components/CameraTPP.h"
#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"
#include "MainSystem/Rendering/BuiltinConstantBuffers.h"
#include "MainSystem/Rendering/DisplayService.h"

#include "MainSystem/Physics/Components/RigidBodyStatic.h"
#include "MainSystem/Physics/Components/RigidBodyDynamic.h"
#include "MainSystem/Physics/Components/CharacterControllerCapsule.h"
#include "MainSystem/Physics/Shapes/PhysicsShapePlane.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeBox.h"
#include "MainSystem/Physics/Materials/PhysicsMaterial.h"
#include "MainSystem/Physics/Joints/RevoluteJoint.h"
#include "MainSystem/Physics/PhysicsSystem.h"

#include "SerializableList.h"

#include "MainSystem/Scripting/ScriptMeta.h"
#include "MainSystem/Scripting/Components/FPPCameraScript.h"
#include "MainSystem/Scripting/Components/TPPCameraScript.h"
#include "MainSystem/Scripting/ScriptingSystem.h"

#include "Common/Base/Serializer.h"

#include "Scene/GameObjectCache.h"
#include "Scene/ModifiedRecorder.h"

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
	UUIDGenerator::SingletonInitialize();
	FileSystem::Initialize();
	MetadataParser::Initialize();
	resource::internal::Initialize();

	auto ret = mheap::New<Runtime>();

	Runtime::s_instance.reset(ret.Get());

	Runtime::s_instance->InitializeModules();

	return ret;
}

void Runtime::Finalize()
{
	Runtime::s_instance->FinalizeModules();
	Runtime::s_instance.release();

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
	UUIDGenerator::SingletonFinalize();
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

	for (auto& v : m_modifiedRecorder)
	{
		v = mheap::New<ModifiedRecorder>();
	}
}

void Runtime::FinalizeModules()
{
	FinalPlugins();

	m_genericStorage.Clear();
	for (auto& m : m_modifiedRecorder)
	{
		m = nullptr;
	}

	for (auto& scene : m_scenes)
	{
		scene->CleanUp();
	}

	m_scenes.clear();

	DisplayService::SingletonFinalize();
	BuiltinConstantBuffers::SingletonFinalize();

	FinalGraphics();
	FinalNetwork();

	ScriptMeta::SingletonFinalize();
	SerializableDB::SingletonFinalize();

	byte resetValues[2] = { MARK_COLOR::WHITE, MARK_COLOR::BLACK };
	gc::PerformFullSystemGC(255, resetValues);
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
	PluginLoader::SingletonInitialize();

	if (StartupConfig::Get().pluginsPath)
	{
		if (PluginLoader::Get()->LoadAll(this, StartupConfig::Get().pluginsPath, m_plugins) == false)
		{
			std::cerr << "[PLUGIN]: Initialization failed!\n";
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
	PluginLoader::Get()->UnloadAll(this, m_plugins);
	PluginLoader::SingletonFinalize();
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

	if (m_nextRunningScene != nullptr)
	{
		return;
	}
		
	SetRunningScene(scene.Get());

	Transform transform = {};

	auto cameraObj = mheap::New<GameObject>();
	cameraObj->Name() = "#camera";
	auto fppCamScript = cameraObj->NewComponent<FPPCameraScript>();
	auto camera = cameraObj->NewComponent<CameraTPP>();
	camera->Projection().SetPerspectiveFovLH(
		PI / 3.0f,
		Graphics::Get()->GetWindowWidth() / (float)Graphics::Get()->GetWindowHeight(),
		0.5f,
		1000.0f
	);
	camera->SetTPPEnabled(false);
	fppCamScript->SetFPPScriptEnable(true);
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
	//auto object = Serializer::CloneObject(resource::Load<AnimModel>("model/globin/globin.fbx")->MakeGameObject()); //ResourceUtils::LoadAnimModelArray("model/Mixamo/FastRun.fbx");
	////auto object = ResourceUtils::LoadAnimModel("model/simple/Character Running.fbx", "model/simple/Character Texture 256x256.png");
	////auto object = ResourceUtils::LoadAnimModelArray("model/robot/white_robot.glb", "model/robot/white_robot_albedo.png");
	////auto object = ResourceUtils::LoadModel3DBasic("Default/cube1.obj");
	////auto object = ResourceUtils::LoadAnimModelArray("model/vampires/dancing_vampire.dae", "model/vampires/Vampire_diffuse.png");
	//object->SetLocalTransform(transform);
	//scene->AddObject(object);

	//{
	//	Serializer serializer = {};
	//	serializer.Serialize(object);
	//	serializer.SetRootUUID(object->GetUUID());
	//	serializer.WriteToFile("Data/FastRun.AnimatorSkeletalArray.json");
	//}

	/*{
		Handle<GameObject> o;
		Serializer serializer = {};
		serializer.ReadFromFile("Data/FastRun.AnimatorSkeletalArray.json");
		serializer.Deserialize(serializer.GetRootUUID(), o);

		o->Name() = "Object2";
		scene->AddObject(o);
	}*/

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
		obj->Name() = "Ground";
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
		obj->Name() = "Kinematic";
		obj->NewComponent<MeshBasicRenderer>("Default/cube1.obj", "Default/green.png");

		auto shape = std::make_shared<PhysicsShapeBox>(Vec3(5.0f, 5.0f, 16.0f), material);
		obj->NewComponent<RigidBodyDynamic>(shape)->SetKinematic(true);

		transform = {};
		transform.Scale() = Vec3(2.5f, 2.5f, 8.0f);
		transform.Position() = { 10, 0.0f, 10 };
		transform.Rotation() = Mat4::Rotation(Vec3::X_AXIS, -PI / 6.0f);
		//transform.Rotation() = Mat4::Rotation(Vec3::Z_AXIS, PI / 2);
		obj->SetLocalTransform(transform);

		scene->AddObject(obj);
	}

	Handle<RigidBodyDynamic> testBody;
	//auto material2 = std::make_shared<PhysicsMaterial>(0.1f, 0.1f, 0.1f);
	for (size_t y = 1; y < 2; y++)
	{
		for (size_t x = 1; x < 2; x++)
		{
			auto obj = mheap::New<GameObject>();
			obj->Name() = "Center Cube";
			obj->NewComponent<MeshBasicRenderer>("Default/cube1.obj", "Default/green.png");

			auto shape = std::make_shared<PhysicsShapeBox>(Vec3(5.0f, 5.0f, 5.0f), material);
			auto dyn = obj->NewComponent<RigidBodyDynamic>(shape);//->SetPhysicsFlag(PHYSICS_FLAG_ENABLE_COLLISION, true);
			dyn->SetPhysicsFlag(PHYSICS_FLAG_COLLISION_RESULT, true);
			dyn->SetDensity(2.5f);

			//obj->NewComponent<TestScript2>();

			transform = {};
			transform.Scale() = Vec3(2.5f);
			transform.Position() = { x * 15.0f, 4.0f, y * 15.0f };
			//transform.Rotation() = Mat4::Rotation(Vec3::Z_AXIS, PI / 2);
			obj->SetLocalTransform(transform);

			scene->AddObject(obj);
		}
	}

	{
		auto obj1 = mheap::New<GameObject>();
		obj1->Name() = "Wall1";
		obj1->NewComponent<MeshBasicRenderer>("Default/cube1.obj", "Default/green.png");

		auto shape0 = std::make_shared<PhysicsShapeBox>(Vec3(10.0f, 5.0f, 0.2f), material);
		auto body0 = obj1->NewComponent<RigidBodyStatic>(shape0);

		Transform transform1 = {};
		transform1.Scale() = Vec3(10.0f, 5.0f, 0.2f) / 2.0f;
		transform1.Position() = { -15, 2.5f, 0 };
		//transform1.Rotation() = Mat4::Rotation(Vec3::X_AXIS, -PI / 6.0f);
		obj1->SetLocalTransform(transform1);


		auto obj2 = mheap::New<GameObject>();
		obj2->Name() = "Wall2";
		obj2->NewComponent<MeshBasicRenderer>("Default/cube1.obj", "Default/green.png");

		auto shape1 = std::make_shared<PhysicsShapeBox>(Vec3(3.0f, 4.0f, 0.2f), material);
		auto body1 = obj2->NewComponent<RigidBodyDynamic>(shape1);
		body1->SetDensity(1.f);

		Transform transform2 = {};
		transform2.Scale() = Vec3(3.0f, 4.0f, 0.2f) / 2.0f;
		transform2.Position() = { -15.0f - 5.0f - 2.0f, 2.5f, 0 };
		//transform2.Rotation() = Mat4::Rotation(Vec3::X_AXIS, -PI / 6.0f);
		obj2->SetLocalTransform(transform2);

		auto joint = mheap::New<RevoluteJoint>(body0, transform1, body1, transform2);

		scene->AddObject(obj1);
		scene->AddObject(obj2);

		testBody = body1;
	}

	{
		auto obj = mheap::New<GameObject>();
		obj->Name() = "CCT";
		//obj->NewComponent<MeshBasicRenderer>("Default/capsule.obj", "Default/green.png");

		{
			CharacterControllerCapsuleDesc desc = {};
			desc.capsule = Capsule(Vec3::ZERO + Vec3::UP, 1.0f, 0.5f);
			desc.material = material;
			auto cct = obj->NewComponent<CharacterControllerCapsule>(desc);
			cct->SetPhysicsFlag(PHYSICS_FLAG_COLLISION_RESULT, true);

			auto script = obj->NewComponent<TPPCameraScript>();
			script->m_camera = camera;
			script->m_fppCamScript = fppCamScript;
			script->m_testBody = testBody;

			transform = {};
			transform.Position() = Vec3::ZERO + Vec3::UP;
			transform.Position().y = 100;
			//transform.Position().z = 10;
			obj->SetLocalTransform(transform);

			scene->AddObject(obj);

			//cct->SetGravity(scene->GetPhysicsSystem()->GetGravity());
		}

		camera->SetTarget(obj);
	}

}

void Runtime::Run()
{
	while (m_isRunning)
	{
		/*if (m_runningSceneIdx != m_nextRunningSceneIdx)
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
		}*/

#ifdef PLUGIN_ALLOW_HOT_RELOAD
		if (m_reloadScripts)
		{
			HotReloadScriptsImpl();
			m_reloadScripts = false;
			continue;
		}
#endif

		Iteration();
		//Thread::Sleep(1);
	}

	TaskWorker::Get()->IsRunning() = false;

	while (m_gcIsRunning.load(std::memory_order_relaxed))
	{
		Thread::Sleep(100);
	}
}

void Runtime::SwapModifiedRecorder()
{
	GetModifiedRecorder()->Commit();
	m_curModifiedRecorderId = (m_curModifiedRecorderId + 1) % (sizeof(m_modifiedRecorder) / sizeof(*m_modifiedRecorder));
	GetModifiedRecorder()->Clear();

	size_t i = 0;
	for (auto& scene : m_scenes)
	{
		if (!m_nextRunningScene || (i != m_nextRunningScene->m_runtimeID && scene))
		{
			for (size_t j = 0; j < MainSystemInfo::COUNT; j++)
			{
				scene->PerformModificationForMainSystem(j);
				scene->FlushAsyncTasksForMainSystem(j);
			}
		}
		i++;
	}
}

void Runtime::ProcessSwapRunningScene()
{
	m_createSceneLock.lock();

	if (m_runningScene != m_nextRunningScene)
	{
		if (m_runningScene != nullptr)
		{
			m_runningScene->EndRunning();
		}

		m_runningScene = m_nextRunningScene;

		if (m_runningScene != nullptr)
		{
			m_runningScene->BeginRunning();
		}
	}

	if (!m_destroyingScenes.empty())
	{
		ProcessDestroyScenes();
	}

	m_createSceneLock.unlock();
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
			auto sheap = mheap::internal::GetHeap(mheap::internal::HEAP_ID::STABLE_HEAP);
			auto heap = mheap::internal::GetHeap(mheap::internal::HEAP_ID::GC_HEAP);
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

	ProcessSwapRunningScene();
	if (m_runningScene == nullptr)
	{
		return;
	}

	auto mainScene = GetCurrentRunningScene();

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
			SwapModifiedRecorder();
			ProcessSwapRunningScene();

			mainScene->Iteration(fixedDt);

			g_sumDt -= fixedDt;
		}

		return;
	}

	SwapModifiedRecorder();
	ProcessSwapRunningScene();
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
	for (int64_t i = 0; i < m_destroyingScenes.size(); i++)
	{
		auto& info = m_destroyingScenes[i];

		if (info.count != 0)
		{
			info.count--;
			continue;
		}

		Task task;
		task.Entry() = [](void* p)
		{
			auto scene = (Scene*)p;
			Runtime::Get()->DestroySceneImpl(scene);
		};
		task.Params() = info.scene;

		TaskSystem::Submit(task, Task::HIGH);
		//DestroySceneImpl(m_scenes[m_destroyingScenes[i]].Get());
		STD_VECTOR_ROLL_TO_FILL_BLANK_2(m_destroyingScenes, i);
		i--;
	}

	//m_destroyingScenes.clear();
}

void Runtime::DestroySceneImpl(Scene* scene)
{
	//Thread::Sleep(30);

	ID id = scene->m_runtimeID;

	scene->CleanUp();

	EventDispatcher()->Dispatch(EVENT::EVENT_SCENE_DESTROYED, scene);

	m_createSceneLock.lock();
	MANAGED_ARRAY_ROLL_TO_FILL_BLANK(m_scenes, scene, m_runtimeID);
	m_createSceneLock.unlock();

	byte resetValues[2] = { MARK_COLOR::WHITE, MARK_COLOR::BLACK };
	gc::PerformFullSystemGC(255, resetValues);

	//scene->m_runtimeID = INVALID_ID;
}

Handle<Scene> Runtime::CreateScene(Scene* _scene)
{
	Handle<Scene> scene = _scene == nullptr ? mheap::New<Scene>() : _scene;

	m_createSceneLock.lock();
	scene->m_runtimeID = m_scenes.size();
	m_scenes.Push(scene);
	m_createSceneLock.unlock();

	EventDispatcher()->Dispatch(EVENT::EVENT_SCENE_CREATED, scene.Get());
	return scene;
}

void Runtime::DestroyScene(Scene* scene)
{
	if (scene->m_destroyed || scene->m_runtimeID == INVALID_ID)
	{
		return;
	}

	if (scene == m_runningScene)
	{
		assert(m_nextRunningScene != m_runningScene && "Need to set another running scene before destroy the current scene!!!");
	}

	scene->m_destroyed = true;

	m_createSceneLock.lock();
	m_destroyingScenes.push_back({ scene });
	m_createSceneLock.unlock();

	if (scene == m_runningScene && m_nextRunningScene == m_runningScene)
	{
		m_nextRunningScene = nullptr;
	}
}

void Runtime::SetRunningScene(Scene* scene)
{
	m_nextRunningScene = scene;
}

void* Runtime::GetNativeHWND()
{
	return platform::GetWindowNativeHandle(m_window);
}

#ifdef PLUGIN_ALLOW_HOT_RELOAD
void Runtime::HotReloadScriptsImpl()
{
	struct ComponentInfo
	{
		Plugin* plugin;
		ID COMPONENT_ID;
	};

	struct ReloadingComponent
	{
		GameObject* obj;
		ComponentInfo* info;
		UUID componentUUID;
		String className;

		std::map<String, Variant> variables;
	};

	EventDispatcher()->Dispatch(EVENT::EVENT_HOT_RELOAD_SCRIPTS_BEGIN);

	auto plugins = PluginLoader::Get()->GetHotReloadablePlugins();

	std::map<String, ComponentInfo> classNameByPlugin;

	for (auto& plugin : plugins)
	{
		size_t i = 0;
		for (auto& arr : plugin->m_customComps)
		{
			for (auto& compClassName : arr)
			{
				classNameByPlugin[compClassName] = { plugin, i };
			}
			i++;
		}
	}

	std::vector<ReloadingComponent> reloadingComponents;

	auto RecordComponent = [&](Scene* scene, GameObject* obj, bool removeComp)
		{
			ID compId = 0;
			for (auto& comp : obj->m_mainComponents)
			{
				if (comp && compId == Script::COMPONENT_ID)
				{
					auto className = comp->GetClassName();
					auto it = classNameByPlugin.find(className);
					if (it != classNameByPlugin.end())
					{
						reloadingComponents.push_back({ obj, &it->second, comp->GetUUID(),className });
						auto& back = reloadingComponents.back();
						
						auto metaData = comp->GetMetadata(0);
						metaData->ForEachProperties(
							[&](ClassMetadata* metadata, const char* propertyName, Accessor& accessor, size_t depth)
							{
								if (depth != 0)
								{
									return;
								}

								back.variables.insert({ propertyName,accessor.Get() });
							}, 
							nullptr
						);

						if (removeComp)
						{
							//assert(compId == Script::COMPONENT_ID);
							//scene->GetScriptingSystem()->RemoveComponent(comp);
							obj->RemoveComponentRaw((Script*)comp.Get());
						}
					}
				}
				compId++;
			}
		};

	for (auto& scene : m_scenes)
	{
		for (auto& obj : scene->m_longLifeObjects)
		{
			RecordComponent(scene, obj, true);
		}

		for (auto& obj : scene->m_shortLifeObjects)
		{
			RecordComponent(scene, obj, true);
		}

		for (auto& trash : scene->m_trashObjects)
		{
			for (auto& obj : trash)
			{
				RecordComponent(scene, (GameObject*)obj.Get(), false);
			}
		}
	}

	auto oldRunningScene = m_nextRunningScene;
	m_nextRunningScene = nullptr;
	for (size_t i = 0; i < 5; i++)
	{
		SwapModifiedRecorder();
	}
	m_nextRunningScene = oldRunningScene;

	for (auto& plugin : plugins)
	{
		for (auto& arr : plugin->m_customComps)
		{
			for (auto& compClassName : arr)
			{
				SerializableDB::Get()->RemoveRecord(compClassName.c_str());
			}
		}
	}

	for (size_t i = 0; i < 1; i++)
	{
		byte resetValues[2] = { MARK_COLOR::WHITE, MARK_COLOR::BLACK };
		gc::PerformFullSystemGC(255, resetValues);
	}

	PluginLoader::Get()->ReloadAll(this);

	EventDispatcher()->Dispatch(EVENT::EVENT_HOT_RELOAD_SCRIPTS_END);

	for (auto& elm : reloadingComponents)
	{
		auto obj = elm.obj;
		const auto COMPONENT_ID = elm.info->COMPONENT_ID;

		assert(COMPONENT_ID == Script::COMPONENT_ID);

		/*if (obj->m_isLongLife)
		{
			mheap::internal::SetHeapId(mheap::internal::HEAP_ID::STABLE_HEAP);
		}
		else
		{
			mheap::internal::SetHeapId(mheap::internal::HEAP_ID::GC_HEAP);
		}*/

		auto comp = DynamicCast<Script>(SerializableDB::Get()->GetSerializableRecord(elm.className.c_str()).ctor());

		auto metaData = comp->GetMetadata(0);
		metaData->ForEachProperties(
			[&](ClassMetadata* metadata, const char* propertyName, Accessor& accessor, size_t depth)
			{
				if (depth != 0)
				{
					return;
				}

				auto it = elm.variables.find(propertyName);
				if (it == elm.variables.end())
				{
					return;
				}

				if (accessor.Get().Type() == it->second.Type() && it->second.Type() != VARIANT_TYPE::UNKNOWN)
				{
					accessor.Set(it->second);
				}
			},
			nullptr
		);

		assert(comp != nullptr);
		obj->AddComponent(comp);
	}

	//mheap::internal::SetHeapId(mheap::internal::HEAP_ID::GC_HEAP);

	std::cout << "Done reload\n";

}
void Runtime::HotReloadScripts()
{
	m_reloadScripts = true;
}
#endif

NAMESPACE_END