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
#include "Resources/Resource.h"

#include "Plugins/Plugin.h"
#include "Plugins/PluginLoader.h"

#include "StartupConfig.h"
#include "ENGINE_EVENT.h"

#include "Components2D/Script/Script2D.h"
#include "Components2D/Rendering/Sprite.h"
#include "Components2D/Rendering/Camera2D.h"
#include "Components2D/Rendering/Sprites.h"
#include "Components2D/Physics/Physics2D.h"

#include "Objects2D/Physics/Bodies/RigidBody2D.h"
#include "Objects2D/Physics/Colliders/AARectCollider.h"

#include "Objects2D/Scene2D/UniqueGridScene2D.h"
#include "Objects2D/GameObject2D.h"

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
	resource::internal::Initialize();
}

Engine::~Engine()
{
	resource::internal::Finalize();
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
		if (Graphics2D::Initilize(m_window, StartupConfig::Get().windowWidth, StartupConfig::Get().windowHeight) != 0)
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
	g_timer.Update();
	g_timer.Update();

	DeferredBufferTracker::Get()->Reset();

	auto scene = mheap::New<UniqueGridScene2D>(this, 128, 128, 60, 60);
	scene->LockDt(StartupConfig::Get().fixedDt);
	scene->m_id = 0;
	m_scenes.Push(scene);

	Dispatch(ENGINE_EVENT::SCENE_ON_INITIALIZE, this, scene.Get());

	auto mainScene = m_scenes[0].Get();
	mainScene->BeginSetup();
	scene->Setup();
	Dispatch(ENGINE_EVENT::SCENE_ON_SETUP, this, scene.Get());

	std::array<std::array<size_t, 32>, 32> mapValues = {};

	{
		auto cellCollider = MakeShared<AARectCollider>(AARect({ 0,0 }, { 60,60 }));
		for (size_t y = 0; y < mapValues.size(); y++)
		{
			auto& row = mapValues[y];
			for (size_t x = 0; x < row.size(); x++)
			{
				row[x] = 1;// Random::RangeInt64(1, 2);

				auto v = Random::RangeInt64(1, 10);

				if (y == 0 || y == mapValues.size() - 1 || x == 0 || x == row.size() - 1 || v == 10)
				{
					row[x] = 0;
					auto object = mheap::New<GameObject2D>(GameObject2D::STATIC);
					object->NewComponent<Physics2D>(cellCollider, nullptr);
					object->Position() = { x * 60, y * 60 };
					mainScene->AddObject(object);
				}

				if (v == 10 && !(y == 0 || y == mapValues.size() - 1 || x == 0 || x == row.size() - 1))
				{
					row[x] = 2;
				}
			}
		}
	}

	mainScene->EndSetup();

	Dispatch(ENGINE_EVENT::SCENE_ON_START, this, scene.Get());

	{
		class PlayerScript : Traceable<PlayerScript>, public Script2D
		{
		protected:
			using Base = Script2D;
			TRACEABLE_FRIEND();
			void Trace(Tracer* tracer)
			{
				Base::Trace(tracer);
				tracer->Trace(m_renderer);
			}

			Handle<Sprites> m_renderer;

		public:
			float m_speed = 200;

			virtual void OnStart() override
			{
				m_renderer = GetObject()->GetComponent<Sprites>();
			}

			virtual void OnUpdate(float dt) override
			{
				m_renderer->SetSprite(0);

				if (Input()->IsKeyDown('W'))
				{
					Position().y -= m_speed * dt;
					m_renderer->SetSprite(1);
				}

				if (Input()->IsKeyDown('S'))
				{
					Position().y += m_speed * dt;
					m_renderer->SetSprite(2);
				}

				if (Input()->IsKeyDown('A'))
				{
					Position().x -= m_speed * dt;
					m_renderer->SetSprite(3);
				}

				if (Input()->IsKeyDown('D'))
				{
					Position().x += m_speed * dt;
					m_renderer->SetSprite(4);
				}
			}

		};

		auto player = mheap::New<GameObject2D>(GameObject2D::DYNAMIC);
		player->Position() = { 800 / 2, mapValues.size() * 60 - 100 };
		auto sprites = player->NewComponent<Sprites>();
		sprites->SetSprite(sprites->Load("Player.png", AARect(), Vec2(50, 50)));
		sprites->Load("PlayerUP.png", AARect(), Vec2(50, 50));
		sprites->Load("PlayerDOWN.png", AARect(), Vec2(50, 50));
		sprites->Load("PlayerLEFT.png", AARect(), Vec2(50, 50));
		sprites->Load("PlayerRIGHT.png", AARect(), Vec2(50, 50));
		player->NewComponent<PlayerScript>();

		auto cellCollider = MakeShared<AARectCollider>(AARect({ 0,0 }, { 50,50 }), Vec2(5, 5));
		auto rigidBody = MakeShared<RigidBody2D>(RigidBody2D::KINEMATIC);
		player->NewComponent<Physics2D>(cellCollider, rigidBody);

		Vec2 camViewSize = { 960 * 1.2f, 720 * 1.2f };
		auto camObj = mheap::New<GameObject2D>(GameObject2D::DYNAMIC);
		auto cam = camObj->NewComponent<Camera2D>(AARect({ 0,0 }, camViewSize));
		cam->SetClamp(camViewSize / 2.0f, Vec2(mapValues[0].size() * 60) - camViewSize / 2.0f);
		player->AddChild(camObj);

		mainScene->AddObject(player);
	}

	{
		class MapRenderer : public Renderer2D
		{
		private:
			struct LoadedSprite
			{
				Resource<Texture2D> rc;
				sf::Sprite sprite;
			};

			Vec2 m_cellSize = {};
			std::vector<LoadedSprite> m_sprites;
			std::vector<size_t> m_map;
			size_t m_width;
			size_t m_height;

		public:
			MapRenderer(size_t width, size_t height, const Vec2& cellSize, size_t cellValueMax)
				: m_width(width), m_height(height), m_cellSize(cellSize)
			{
				m_sprites.resize(cellValueMax + 1);
				m_map.resize(m_width * m_height);

				m_zOrder = -99999;
			}

			void SetCellValue(size_t x, size_t y, size_t value)
			{
				assert(value < m_sprites.size());
				m_map[y * m_width + x] = value;
			}

			void LoadCell(size_t value, String path, const AARect& textureRect)
			{
				auto& s = m_sprites[value];
				s.rc = resource::Load<Texture2D>(path);
				s.sprite.setTexture(s.rc->GetSFTexture());
				SetSpriteTextureRect(s.sprite, textureRect);
				s.sprite.setScale(reinterpret_cast<sf::Vector2f&>(GetScaleFitTo(s.sprite, m_cellSize)));
			}

			virtual void Render(RenderingSystem2D* rdr) override
			{
				Vec2 temp[4];
				auto cam = rdr->GetCurrentCamera();
				AARect view = cam->GetView();
				view.GetPoints(temp);

				auto beginX		= (intmax_t)std::floor(temp[0].x / m_cellSize.x);
				auto endX		= (intmax_t)std::ceil(temp[1].x / m_cellSize.x);

				auto beginY		= (intmax_t)std::floor(temp[0].y / m_cellSize.y);
				auto endY		= (intmax_t)std::ceil(temp[2].y / m_cellSize.y);

				beginX			= clamp(beginX, (intmax_t)0, (intmax_t)m_width);
				endX			= clamp(endX, (intmax_t)0, (intmax_t)m_width);

				beginY			= clamp(beginY, (intmax_t)0, (intmax_t)m_height);
				endY			= clamp(endY, (intmax_t)0, (intmax_t)m_height);

				for (size_t y = beginY; y < endY; y++)
				{
					for (size_t x = beginX; x < endX; x++)
					{
						auto v = m_map[y * m_width + x];
						RenderSprite(rdr, m_sprites[v].sprite, 0, { x * m_cellSize.x, y * m_cellSize.y });
					}
				}
			}

		};
		
		auto map = mheap::New<GameObject2D>(GameObject2D::GHOST);
		auto mapRenderer = map->NewComponent<MapRenderer>(mapValues[0].size(), mapValues.size(), Vec2(60, 60), 10);

		for (size_t y = 0; y < mapValues.size(); y++)
		{
			auto& row = mapValues[y];
			for (size_t x = 0; x < row.size(); x++)
			{
				mapRenderer->LoadCell(row[x], String::Format("{}.png", row[x]), {});
				mapRenderer->SetCellValue(x, y, row[x]);
			}
		}

		mainScene->AddObject(map);
	}
}

void Engine::Run()
{
	//Setup();

	auto mainScene = m_scenes[0].Get();

	while (m_isRunning)
	{
		Iteration();
		Thread::Sleep(1);
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

	g_timer.Update();

	g_sumDt += g_timer.dt;

	auto fixedDt = StartupConfig::Get().fixedDt;

	while (g_sumDt > fixedDt)
	{
		mainScene->PrevIteration();
		mainScene->Iteration();
		g_sumDt -= fixedDt;
	}
	
	mainScene->PostIteration();

	// SynchronizeAllSubSystems
	//SynchronizeAllSubSystems();
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