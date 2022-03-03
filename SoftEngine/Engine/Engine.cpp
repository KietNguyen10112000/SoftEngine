#include "Engine.h"

#include "Global.h"

#include <iostream>

#include <Common.h>

#include "Core/Renderer.h"

#include "Math/Math.h"

#include "Core/Resource.h"
#include "Core/RenderPipeline.h"

#include "Core/ILightSystem.h"

#include "Components/FPPCamera.h"
#include "Components/SpaceCoordinate.h"
#include "Components/SkyBox.h"

#include "Components/AnimObject.h"

#include "Random.h"

#include "Logic/LogicWorker.h"
#include "Rendering/RenderingWorker.h"

#include "Scene/SimpleScene.h"

#include "Core/MultiThreading/ThreadBarrier.h"


Engine::Engine(const wchar_t* title, int width, int height) : Window(title, width, height)
{
	//SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	//::SetProcessDPIAware();
	//SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

	Random::Initialize();

	m_currentTime = GetTime();

	m_resourceManager = new Resource();

	m_rplManager = new class RenderPipelineManager();

	Global::engine = this;
	Global::input = Input();
	Global::resourceManager = ResourceManager();
	Global::rplManager = RenderPipelineManager();


	void* args[] = { GetNativeHandle() };
	m_renderer = new DeferredRenderer(2, width, height, 1, args);

	Global::renderer = Renderer();


	//for raw view
	m_cam = new ICamera();
	m_renderer->SetTargetCamera(m_cam);

	m_spaceCoord = new SpaceCoordinate();
	m_spaceCoord->DisplayGrid() = false;

	m_skyBox = new SkyCube(L"D:/KEngine/ResourceFile/temp_img/skybox-blue-night-sky.png");
	m_renderer->AttachSkyBox(m_skyBox);


	m_currentScene = new SimpleScene(this);


	m_threadBarrier = new ThreadBarrier(2);
	m_logicWorker = new LogicWorker(this);
	m_renderingWorker = new RenderingWorker(this);

	Timing();

	//spawn thread
	m_renderingThread = new std::thread([=]() 
		{
			while (m_renderingWorker->IsRunning())
			{
				((ThreadBarrier*)m_threadBarrier)->Synch(0, 0);
				m_renderingWorker->Update();
			}
		}
	);
	m_renderingThread->detach();
}

Engine::~Engine()
{
	m_renderingWorker->IsRunning() = false;

	m_logicWorker->IsRunning() = false;

	if (m_renderingThread->joinable()) m_renderingThread->join();

	delete m_renderingWorker;
	delete m_renderingThread;
	delete m_logicWorker;

	delete m_currentScene;

	delete m_cam;
	delete m_spaceCoord;
	delete m_skyBox;

	TBNAnimModel::FreeStaticResource();

	delete m_renderer;
	delete m_rplManager;
	delete m_resourceManager;

	Random::UnInitialize();
}

void Engine::Update()
{
	((ThreadBarrier*)m_threadBarrier)->Synch(
		[](void* arg)
		{
			((Engine*)(arg))->Timing();
		},
		this
	);

	if (m_logicWorker->IsRunning())
	{
		m_logicWorker->Update();
	}
}

void Engine::Timing()
{
	auto cur = GetTime();
	m_deltaTime = cur - m_currentTime;
	m_currentTime = cur;

	m_fCurrentTime = m_currentTime / _TIME_FACTOR;
	m_fDeltaTime = m_deltaTime / _TIME_FACTOR;

	m_time += m_fDeltaTime;

	//std::cout << Global::engine->FDeltaTime() << "\n";
}