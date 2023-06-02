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
#include "Components2D/Rendering/SpriteRenderer.h"
#include "Components2D/Rendering/Camera2D.h"
#include "Components2D/Rendering/SpritesRenderer.h"
#include "Components2D/Rendering/AnimatedSpritesRenderer.h"
#include "Components2D/Physics/Physics2D.h"
#include "Components2D/Physics/RigidBody2D.h"

#include "Objects2D/Physics/Colliders/AARectCollider.h"
#include "Objects2D/Physics/Colliders/RectCollider.h"

#include "Objects2D/Scene2D/UniqueGridScene2D.h"
#include "Objects2D/GameObject2D.h"

#include "imgui.h"
#include "imgui-SFML.h"

//#include "Network/TCPAcceptor.h"
//#include "Network/TCPConnector.h"

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
	resource::internal::Initialize();
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
	resource::internal::Finalize();
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
		/*SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		::SetProcessDPIAware();*/

		m_input = rheap::New<Input>();
		//m_window = (void*)platform::CreateWindow(m_input, 0, 0, -1, -1, "SoftEngine");
		if (Graphics2D::Initilize(m_window, StartupConfig::Get().windowWidth, StartupConfig::Get().windowHeight) != 0)
		{
			m_isRunning = false;
		}
		platform::BindInput(m_input, m_window->getSystemHandle());

		ImGui::SFML::Init(reinterpret_cast<sf::RenderWindow&>(*m_window), false);

		ImGuiStyle& style = ImGui::GetStyle();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImFontConfig config{};
		//config.GlyphExtraSpacing.x = 1.0f;
		config.OversampleH = config.OversampleV = 1;
		config.PixelSnapH = true;
		config.SizePixels = 13.0f * 1.0f;
		//config.EllipsisChar = (ImWchar)0x0085;
		config.GlyphOffset.y = 1.0f * ((float)(int)(((config.SizePixels / 13.0f)) + 0.5f));
		config.GlyphRanges = io.Fonts->GetGlyphRangesVietnamese();
		//config.GlyphExtraSpacing.x = 1.0f;
		
		//io.Fonts->AddFontDefault(&config);
		io.Fonts->AddFontFromFileTTF("segoeui.ttf", (int)(24.0f), &config);
		ImGui::SFML::UpdateFontTexture();
		
		//io.FontGlobalScale = 2.0f;
		//style.ScaleAllSizes(2.0f);
	}
}

void Engine::FinalGraphics()
{
	if (StartupConfig::Get().isEnableRendering)
	{
		ImGui::SFML::Shutdown();
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
	/*TCP_SOCKET_DESCRIPTION desc;
	desc.host = "127.0.0.1";
	desc.port = 9023;
	desc.useNonBlocking = true;
	TCPConnector tpc(desc);
	if (tpc.Connect() < 0)
	{
		std::cout << "Unable to connect to server. Aborted.\n";
		Throw();
	}
	tpc.SetBlockingMode(false);
	auto str = tpc.GetAddressString();
	auto str2 = tpc.GetPeerAddressString();*/

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

	//std::array<std::array<size_t, 32>, 32> mapValues = {};

	//{
	//	auto cellCollider = MakeShared<AARectCollider>(AARect({ 0,0 }, { 60,60 }));
	//	for (size_t y = 0; y < mapValues.size(); y++)
	//	{
	//		auto& row = mapValues[y];
	//		for (size_t x = 0; x < row.size(); x++)
	//		{
	//			row[x] = 1;// Random::RangeInt64(1, 2);

	//			auto v = Random::RangeInt64(1, 10);

	//			if (y == 0 || y == mapValues.size() - 1 || x == 0 || x == row.size() - 1 || v == 10)
	//			{
	//				row[x] = 0;
	//				auto object = mheap::New<GameObject2D>(GameObject2D::STATIC);
	//				object->NewComponent<Physics2D>(cellCollider)
	//					->CollisionMask() = 1ull | (1ull << 2) | (1ull << 3);
	//				object->Position() = { x * 60, y * 60 };
	//				mainScene->AddObject(object);
	//			}

	//			if (v == 10 && !(y == 0 || y == mapValues.size() - 1 || x == 0 || x == row.size() - 1))
	//			{
	//				row[x] = 2;
	//			}
	//		}
	//	}
	//}

	mainScene->EndSetup();

	Dispatch(ENGINE_EVENT::SCENE_ON_START, this, scene.Get());

	//{
	//	class BulletScript : Traceable<BulletScript>, public Script2D
	//	{
	//	protected:
	//		using Base = Script2D;
	//		TRACEABLE_FRIEND();
	//		void Trace(Tracer* tracer)
	//		{
	//			Base::Trace(tracer);
	//			tracer->Trace(m_from);
	//		}

	//		Handle<GameObject2D>	m_from;
	//		Vec2					m_dir;
	//		float					m_speed;

	//	public:
	//		virtual void OnUpdate(float dt) override
	//		{
	//			Position() += m_dir * m_speed * dt;
	//		}

	//		virtual void OnCollide(GameObject2D* obj, const Collision2DPair& pair) override
	//		{
	//			//m_scene->RemoveObject(GetObject());
	//			//std::cout << "Bullet removed\n";
	//			m_speed = 0;

	//			//if (obj->GetComponentRaw<Physics2D>()->CollisionMask() 
	//			//	== GetObject()->GetComponentRaw<Physics2D>()->CollisionMask())
	//			{
	//				m_scene->RemoveObject(GetObject());
	//			}
	//		}

	//		inline void Setup(const Handle<GameObject2D>& obj, const Vec2& dir, float speed)
	//		{
	//			m_from = obj;
	//			m_dir = dir;
	//			m_speed = speed;
	//		}
	//	};

	//	class PlayerScript : Traceable<PlayerScript>, public Script2D
	//	{
	//	protected:
	//		using Base = Script2D;
	//		TRACEABLE_FRIEND();
	//		void Trace(Tracer* tracer)
	//		{
	//			Base::Trace(tracer);
	//			tracer->Trace(m_renderer);
	//			tracer->Trace(m_cam);
	//			tracer->Trace(m_gun);
	//			tracer->Trace(m_redLine);
	//		}

	//		Handle<SpritesRenderer>			m_renderer;
	//		Handle<Camera2D>				m_cam;
	//		Handle<GameObject2D>			m_gun;
	//		Handle<GameObject2D>			m_redLine;
	//		Handle<GameObject2D>			m_crossHair;

	//		SharedPtr<RectCollider>			m_bulletCollider;

	//	public:
	//		float m_speed = 300;
	//		float m_rotationSpeed = 100;
	//		float m_recoil = 0;
	//		float m_recoilMax = 0.1f;
	//		Vec2 m_gunRecoilBegin;
	//		Vec2 m_gunRecoilEnd;
	//		float m_gunRecoilLen = 15;
	//		bool m_enableMouse = false;

	//		virtual void OnStart() override
	//		{
	//			m_renderer	= GetObject()->GetComponent<SpritesRenderer>();
	//			m_cam		= GetObject()->Child(0)->GetComponent<Camera2D>();
	//			m_gun		= GetObject()->Child(2);
	//			m_redLine	= GetObject()->Child(1);
	//			m_crossHair = GetObject()->Child(3);

	//			Input()->SetClampCursorInsideWindow(m_enableMouse);

	//			m_bulletCollider = MakeShared<RectCollider>(Rect(-30, -5, 60, 10));

	//			m_gunRecoilEnd = m_gun->Position();
	//		}

	//		virtual void OnUpdate(float dt) override
	//		{
	//			Vec2 motion = { 0,0 };
	//			m_renderer->SetSprite(0);

	//			if (Input()->IsKeyDown('W'))
	//			{
	//				motion.y -= 1;
	//				m_renderer->SetSprite(1);
	//			}

	//			if (Input()->IsKeyDown('S'))
	//			{
	//				motion.y += 1;
	//				m_renderer->SetSprite(2);
	//			}

	//			if (Input()->IsKeyDown('A'))
	//			{
	//				motion.x -= 1;
	//				m_renderer->SetSprite(3);
	//			}

	//			if (Input()->IsKeyDown('D'))
	//			{
	//				motion.x += 1;
	//				m_renderer->SetSprite(4);
	//			}

	//			if (motion != Vec2::ZERO)
	//			{
	//				Position() += motion.Normalize() * m_speed * dt;
	//			}

	//			if (Input()->IsKeyPressed(KEYBOARD::ESC))
	//			{
	//				m_enableMouse = !m_enableMouse;
	//				Input()->SetClampCursorInsideWindow(m_enableMouse);
	//			}

	//			{
	//				auto& cursorPos = Input()->GetCursor().position;
	//				auto center = m_cam->GetWorldPosition(Vec2(cursorPos.x, cursorPos.y), 
	//					Input()->GetWindowWidth(), Input()->GetWindowHeight());
	//				auto& position = Position();
	//				Vec2 d = { center.x - position.x - 25,  center.y - position.y - 40  };
	//				auto len = d.Length();
	//				d.Normalize();

	//				auto rotation = (d.y / std::abs(d.y)) * std::acos(d.Dot(Vec2::X_AXIS));

	//				m_recoil = std::max(m_recoil - dt, 0.0f);

	//				if (m_recoil > 0)
	//				{
	//					m_gun->Position() = Lerp(m_gunRecoilBegin, m_gunRecoilEnd, (m_recoilMax - m_recoil) / m_recoilMax);
	//				}
	//				else
	//				{
	//					m_gun->Position() = m_gunRecoilEnd;
	//				}

	//				if (Input()->IsKeyDown(KEYBOARD::MOUSE_LEFT))
	//				{
	//					Shoot(d, rotation);
	//				}

	//				m_gun->Rotation() = rotation;

	//				m_redLine->Rotation() = m_gun->Rotation();
	//				m_redLine->Scale().x = len;

	//				m_crossHair->Position() = center - position;
	//			}
	//		}

	//		/*virtual void OnCollide(GameObject2D* obj, const Collision2DPair& pair) override
	//		{
	//			std::cout << "Collide " << m_count++ <<"\n";
	//		}*/

	//		inline void Shoot(const Vec2& dir, float rotation)
	//		{
	//			//auto bullet = mheap::New<GameObject2D>(GameObject2D::DYNAMIC);
	//			//bullet->NewComponent<SpriteRenderer>("medium_bullet2.png")
	//			//	->Sprite().Transform().Scale() = { 0.5f,0.5f };
	//			//bullet->NewComponent<BulletScript>()->SetFrom(GetObject());
	//			//bullet->NewComponent<RigidBody2D>(RigidBody2D::KINEMATIC, m_bulletCollider);
	//			//bullet->Position() = Position();
	//			////bullet->Rotation() = PI / 2.0f;
	//			//m_scene->AddObject(bullet);

	//			if (m_recoil <= 0.0f)
	//			{
	//				auto bullet = mheap::New<GameObject2D>(GameObject2D::DYNAMIC);
	//				auto bulletRdr = bullet->NewComponent<SpriteRenderer>("red.png");
	//				bulletRdr->Sprite().FitTextureSize({ 60, 10 });
	//				bulletRdr->Sprite().SetAnchorPoint({ 0.5f, 0.5f });
	//				bullet->NewComponent<BulletScript>()->Setup(GetObject(), dir, 3000.0f);
	//				bullet->NewComponent<Physics2D>(Physics2D::SENSOR, m_bulletCollider)
	//					->CollisionMask() = (1ull << 3);
	//				bullet->Position() = Position();
	//				bullet->Position().x += 25;
	//				bullet->Position().y += 40;

	//				bullet->Position() += dir * 50;

	//				bullet->Rotation() = rotation;
	//				m_scene->AddObject(bullet);

	//				m_gunRecoilEnd = m_gun->Position();
	//				m_gunRecoilBegin = m_gun->Position() - dir * m_gunRecoilLen;
	//				m_gun->Position() = m_gunRecoilBegin;

	//				m_recoil = m_recoilMax;
	//			}
	//		}
	//	};

	//	Transform2D originTranform = {};

	//	auto player = mheap::New<GameObject2D>(GameObject2D::DYNAMIC);
	//	player->Position() = { 800 / 2, mapValues.size() * 60 - 100 };
	//	auto sprites = player->NewComponent<SpritesRenderer>();
	//	sprites->SetSprite(sprites->Load("Player.png", AARect(), Vec2(50, 50)));
	//	sprites->Load("PlayerUP.png", AARect(), Vec2(50, 50));
	//	sprites->Load("PlayerDOWN.png", AARect(), Vec2(50, 50));
	//	sprites->Load("PlayerLEFT.png", AARect(), Vec2(50, 50));
	//	sprites->Load("PlayerRIGHT.png", AARect(), Vec2(50, 50));
	//	player->NewComponent<PlayerScript>();

	//	auto cellCollider = MakeShared<AARectCollider>(AARect({ 0,0 }, { 50,50 }), Vec2(5, 5));
	//	player->NewComponent<RigidBody2D>(RigidBody2D::KINEMATIC, cellCollider)
	//		->CollisionMask() = (1ull << 2);

	//	Vec2 camViewSize = { 960 * 1.2f, 720 * 1.2f };
	//	auto camObj = mheap::New<GameObject2D>(GameObject2D::DYNAMIC);
	//	auto cam = camObj->NewComponent<Camera2D>(AARect({ 0,0 }, camViewSize));
	//	cam->SetClamp(camViewSize / 2.0f, Vec2(mapValues[0].size() * 60) - camViewSize / 2.0f);
	//	player->AddChild(camObj);

	//	// red line
	//	auto line = mheap::New<GameObject2D>(GameObject2D::DYNAMIC);
	//	auto lineRdr = line->NewComponent<SpriteRenderer>("red.png");
	//	lineRdr->Sprite().FitTextureSize(Vec2(1, 5));
	//	lineRdr->Sprite().SetOpacity(64);
	//	lineRdr->ClearAABB();
	//	line->Position() = { 50 / 2.0f, 40 };
	//	player->AddChild(line);

	//	// gun
	//	auto gunObj = mheap::New<GameObject2D>(GameObject2D::DYNAMIC);
	//	auto gunRdr = gunObj->NewComponent<SpriteRenderer>("smg2.png");
	//	gunRdr->Sprite().SetAnchorPoint(Vec2(100 / 331.0f, 40 / 120.0f));
	//	gunRdr->Sprite().Transform().Scale() = Vec2(0.3f);
	//	gunRdr->ClearAABB();
	//	gunObj->Position() = { 50 / 2.0f, 40 };
	//	player->AddChild(gunObj);

	//	// crosshair
	//	auto crossHair = mheap::New<GameObject2D>(GameObject2D::DYNAMIC);
	//	auto crossHairRdr = crossHair->NewComponent<SpriteRenderer>("CrosshairsRed.png");
	//	crossHairRdr->Sprite().FitTextureSize({ 80, 80 });
	//	crossHairRdr->Sprite().SetAnchorPoint({ 0.5f,0.5f });
	//	crossHairRdr->Sprite().SetOpacity(128);
	//	crossHairRdr->ClearAABB();
	//	player->AddChild(crossHair);

	//	mainScene->AddObject(player);
	//}

	//{
	//	class MapRenderer : public Renderer2D
	//	{
	//	private:
	//		struct LoadedSprite
	//		{
	//			Resource<Texture2D> rc;
	//			sf::Sprite sprite;
	//		};

	//		Vec2 m_cellSize = {};
	//		std::vector<LoadedSprite> m_sprites;
	//		std::vector<size_t> m_map;
	//		size_t m_width;
	//		size_t m_height;

	//	public:
	//		MapRenderer(size_t width, size_t height, const Vec2& cellSize, size_t cellValueMax)
	//			: m_width(width), m_height(height), m_cellSize(cellSize)
	//		{
	//			m_sprites.resize(cellValueMax + 1);
	//			m_map.resize(m_width * m_height);

	//			m_zOrder = -99999;
	//		}

	//		void SetCellValue(size_t x, size_t y, size_t value)
	//		{
	//			assert(value < m_sprites.size());
	//			m_map[y * m_width + x] = value;
	//		}

	//		void LoadCell(size_t value, String path, const AARect& textureRect)
	//		{
	//			auto& s = m_sprites[value];
	//			s.rc = resource::Load<Texture2D>(path);
	//			s.sprite.setTexture(s.rc->GetSFTexture());
	//			SetSpriteTextureRect(s.sprite, textureRect);
	//			s.sprite.setScale(reinterpret_cast<sf::Vector2f&>(GetScaleFitTo(s.sprite, m_cellSize)));
	//		}

	//		virtual void Render(RenderingSystem2D* rdr) override
	//		{
	//			Vec2 temp[4];
	//			auto cam = rdr->GetCurrentCamera();
	//			AARect view = cam->GetView();
	//			view.GetPoints(temp);

	//			auto beginX		= (intmax_t)std::floor(temp[0].x / m_cellSize.x);
	//			auto endX		= (intmax_t)std::ceil(temp[1].x / m_cellSize.x);

	//			auto beginY		= (intmax_t)std::floor(temp[0].y / m_cellSize.y);
	//			auto endY		= (intmax_t)std::ceil(temp[2].y / m_cellSize.y);

	//			beginX			= clamp(beginX, (intmax_t)0, (intmax_t)m_width);
	//			endX			= clamp(endX, (intmax_t)0, (intmax_t)m_width);

	//			beginY			= clamp(beginY, (intmax_t)0, (intmax_t)m_height);
	//			endY			= clamp(endY, (intmax_t)0, (intmax_t)m_height);

	//			for (size_t y = beginY; y < endY; y++)
	//			{
	//				for (size_t x = beginX; x < endX; x++)
	//				{
	//					auto v = m_map[y * m_width + x];
	//					RenderSprite(rdr, m_sprites[v].sprite, 0, { x * m_cellSize.x, y * m_cellSize.y });
	//				}
	//			}
	//		}

	//	};
	//	
	//	auto map = mheap::New<GameObject2D>(GameObject2D::GHOST);
	//	auto mapRenderer = map->NewComponent<MapRenderer>(mapValues[0].size(), mapValues.size(), Vec2(60, 60), 10);

	//	for (size_t y = 0; y < mapValues.size(); y++)
	//	{
	//		auto& row = mapValues[y];
	//		for (size_t x = 0; x < row.size(); x++)
	//		{
	//			mapRenderer->LoadCell(row[x], String::Format("{}.png", row[x]), {});
	//			mapRenderer->SetCellValue(x, y, row[x]);
	//		}
	//	}

	//	mainScene->AddObject(map);
	//}
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

	TaskSystem::InvokeAllWaitWorkers();
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

	if (m_iterationHandler)
	{
		g_sumDt = m_iterationHandler->DoIteration(g_sumDt, mainScene);
		return;
	}

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
	if (!m_input) return;

	m_input->RollEvent();

	auto& window = *m_window;
	sf::Event event;
	while (window.pollEvent(event))
	{
		ImGui::SFML::ProcessEvent(window, event);
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