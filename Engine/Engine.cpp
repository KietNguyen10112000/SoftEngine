#include "Engine.h"

#include <iostream>

#include "Core/Memory/Memory.h"
#include "Core/Memory/DeferredBuffer.h"
#include "Core/Structures/Managed/Function.h"
#include "Core/Random/Random.h"

#include "Platform/Platform.h"

#include "TaskSystem/TaskSystem.h"
#include "TaskSystem/TaskWorker.h"

#include "Objects/Scene/Scene.h"
#include "Objects/Scene/MultipleDynamicLayersScene.h"
#include "Objects/Scene/Event/BuiltinEventManager.h"
#include "Objects/Scene/Event/EventManager.h"


#include "Components/Script/Script.h"
#include "Components/Rendering/Camera.h"

#include "Input/Input.h"
#include "Graphics/Graphics.h"
#include "Graphics/DebugGraphics.h"
#include "Network/Network.h"

#include "Plugins/Plugin.h"
#include "Plugins/PluginLoader.h"

#include "StartupConfig.h"
#include "ENGINE_EVENT.h"

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
		m_window = (void*)platform::CreateWindow(m_input, 0, 0, -1, -1, "SoftEngine");
		if (Graphics::Initilize(platform::GetWindowNativeHandle(m_window), GRAPHICS_BACKEND_API::DX12) != 0)
		{
			m_isRunning = false;
		}
	}
}

void Engine::FinalGraphics()
{
	if (StartupConfig::Get().isEnableRendering)
	{
		Graphics::Finalize();
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
	EventManager::Initialize();
	DeferredBufferTracker::Get()->Reset();

	auto scene = mheap::New<MultipleDynamicLayersScene>(this);
	scene->Setup();
	scene->m_id = 0;
	m_scenes.Push(scene);

	Dispatch(ENGINE_EVENT::SCENE_ON_INITIALIZE, this, scene.Get());

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

	/*constexpr float rangeX = 1000;
	constexpr float rangeY = 1000;
	constexpr float rangeZ = 1000;

	constexpr float rangeDimX = 100;
	constexpr float rangeDimY = 100;
	constexpr float rangeDimZ = 100;*/

	constexpr float rangeX = 100;
	constexpr float rangeY = 100;
	constexpr float rangeZ = 100;

	constexpr float rangeDimX = 10;
	constexpr float rangeDimY = 10;
	constexpr float rangeDimZ = 10;

	auto mainScene = m_scenes[0].Get();
	mainScene->BeginSetup();
	Dispatch(ENGINE_EVENT::SCENE_ON_SETUP, this, scene.Get());
	mainScene->EndSetup();

	Dispatch(ENGINE_EVENT::SCENE_ON_START, this, scene.Get());

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

	//{
	//	class MyScript : public Script
	//	{
	//	public:
	//		~MyScript()
	//		{
	//			std::cout << "MyScript::~MyScript()\n";
	//		}

	//		virtual void OnStart() override
	//		{
	//			std::cout << "MyScript::OnStart()\n";
	//		}

	//		virtual void OnUpdate(float dt) override
	//		{
	//			//std::cout << dt << '\n';
	//			if (Input()->IsKeyPressed('U'))
	//			{
	//				std::cout << "Pressed\n";
	//			}
	//		}

	//	};

	//	auto object = mheap::New<GameObject>();
	//	auto aabb = (AABox*)&object->GetAABB();
	//	*aabb = {
	//			Vec3(5,0,0),
	//			Vec3(1.0f, 1.0f, 1.0f),
	//	};
	//	auto script = object->NewComponent<MyScript>();
	//	mainScene->AddObject(object);
	//}

	/*float X = 0;
	float Y = 0;
	for (size_t y = 0; y < 10; y++)
	{
		X = 0;

		for (size_t x = 0; x < 10; x++)
		{
			auto dynamicObj = mheap::New<GameObject>();
			auto aabb = (AABox*)&dynamicObj->GetAABB();
			*aabb = {
					Vec3(X, Y, 0),
					Vec3(1, 1, 1),
			};
			mainScene->AddObject(dynamicObj);

			X += 2.0f;
		}

		Y += 2.0f;
	}*/
	
	{
		class CameraScript : public Script
		{
		public:
			float m_rotateX = 0;
			float m_rotateY = 0;
			float m_rotateZ = 0;
			Vec3 m_position = {};

			float m_speed = 10;
			float m_rotationSensi = 0.12f;

			~CameraScript()
			{
				std::cout << "CameraScript::~CameraScript()\n";
			}

			virtual void OnStart() override
			{
				auto transform = GetObject()->GetTransformMat4();

				m_position = transform.Position();
				Vec3 direction = transform.Forward().Normal();
				m_rotateX = asin(direction.y);
				m_rotateY = atan2(direction.x, direction.z);
			}

			virtual void OnUpdate(float dt) override
			{
				/*if (dt > 0.025)
				{
					std::cout << dt << "\n";
				}*/

				auto& transform = TransformMat4();
				auto trans = Mat4::Identity();
				trans *= Mat4::Rotation(Vec3::Y_AXIS, m_rotateY);

				auto right = trans.Right().Normal();
				trans *= Mat4::Rotation(right, -m_rotateX);

				auto forward = trans.Forward().Normal();

				if (m_rotateZ != 0)
				{
					trans *= Mat4::Rotation(forward, m_rotateZ);
				}

				trans *= Mat4::Translation(m_position);
				transform = trans;

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

				if (Input()->IsKeyPressed(KEYBOARD::ESC))
				{
					Input()->SetCursorLock(!Input()->GetCursorLock());
				}

				if (Input()->IsKeyPressed('U'))
				{
					m_rotationSensi = 0;
				}

				if (Input()->IsKeyPressed('I'))
				{
					m_rotationSensi = 0.12f;
				}

				if (Input()->IsCursorMoved())
				{
					auto& delta = Input()->GetDeltaCursorPosition();
					m_rotateY += delta.x * dt * m_rotationSensi;
					m_rotateX -= delta.y * dt * m_rotationSensi;

					m_rotateX = std::max(std::min(m_rotateX, PI / 2.0f), -PI / 2.0f);
				}
			}

			virtual void OnGUI() override
			{
				auto dbg = Graphics::Get()->GetDebugGraphics();
				dbg->DrawCube(Mat4::Scaling(2, 2, 2) * Mat4::Translation(0, 10, 0), { 0, 1, 1, 1 });
			}

		};

		auto object = mheap::New<GameObject>();
		auto aabb = (AABox*)&object->GetAABB();
		*aabb = {
				Vec3(0,0,0),
				Vec3(0.01f, 0.01f, 0.01f),
		};

		auto camera = object->NewComponent<Camera>(ToRadians(60), 16 / 9.0f, 0.5f, 1000.0f);
		object->InitializeTransform(
			Mat4::Identity().SetLookAtLH({ 10, 10, 10 }, { 0,0,0 }, Vec3::UP).Inverse()
		);

		object->NewComponent<CameraScript>();

		mainScene->AddObject(object);
	}

}

void Engine::Run()
{
	//Setup();

	auto mainScene = m_scenes[0].Get();

	while (m_isRunning)
	//for (size_t i = 0; i < 100; i++)
	{
		Iteration();

		//auto& dynamicObjs = mainScene->m_tempObjects;
		//size_t count = 0;
		//for (size_t j = 0; j < dynamicObjs.size(); j++)
		//{
		//	//if (Random::RangeInt64(0, 1) == 0)
		//	{
		//		mainScene->RefreshDynamicObject(dynamicObjs[j].Get());
		//		count++;
		//	}
		//}

		//std::cout << "Refresh: " << count << " objects\n";
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
				gc::Run(-1);
				heap->EndGC();
			}
			engine->m_gcIsRunning.exchange(false, std::memory_order_release);
		};
		gcTask.Params() = this;

		TaskSystem::Submit(gcTask, Task::HIGH);
	}
	

	auto mainScene = m_scenes[0].Get();


	// dynamic submit wait
	TaskSystem::PrepareHandle(&taskHandle);

	// process input task must execute on main thread 
	// the thread create the Window - this is required for win32 messeges queue
	// win32 messeges queue is attach with thread that create HWND
	Task processInput = {};
	processInput.Params() = this;
	processInput.Entry() = [](void* e)
	{
		auto engine = (Engine*)e;
		engine->ProcessInput();
	};

	// 0 is main thread id
	TaskSystem::SubmitForThread(&taskHandle, 0, processInput);

	Task reconstructScene = {};
	reconstructScene.Params() = mainScene;
	reconstructScene.Entry() = [](void* s)
	{
		auto scene = (Scene*)s;
		scene->PrevIteration();
	};

	// submit 1 task, so that if this is main thread, it can switch to process input task
	TaskSystem::Submit(&taskHandle, reconstructScene, Task::CRITICAL);

	// wait
	TaskSystem::WaitForHandle(&taskHandle);

	mainScene->Iteration();

	// SynchronizeAllSubSystems
	SynchronizeAllSubSystems();
}

void Engine::ProcessInput()
{
	//std::cout << "ProcessInput()\n";
	m_input->RollEvent();
	m_isRunning = !platform::ProcessPlatformMsg(m_window);
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

	//Graphics::Get()->Present(1, 0);
}

NAMESPACE_END