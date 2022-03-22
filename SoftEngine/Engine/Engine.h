#pragma once
#include "Window.h"

#include <mutex>

#define _TIME_FACTOR 1000.0f

class IRenderer;
class Resource;
class RenderPipelineManager;

class LogicWorker;
class RenderingWorker;

class ICamera;
class SpaceCoordinate;
class IRenderableObject;

class Scene;


class Engine : public Window
{
private:
	uint64_t m_currentTime = 0;
	uint64_t m_deltaTime = 0;

	float m_fCurrentTime = 0;
	float m_time = 0;
	float m_fDeltaTime = 0;

	IRenderer* m_renderer = 0;
	Resource* m_resourceManager = 0;
	RenderPipelineManager* m_rplManager = 0;


	void* m_threadBarrier = 0;

	//multi-threading
	/*size_t m_synchFlag1 = 0;
	size_t m_synchFlag2 = 0;
	std::mutex m_sharedMutex1;
	std::mutex m_sharedMutex2;*/
	LogicWorker* m_logicWorker = 0;
	RenderingWorker* m_renderingWorker = 0;
	//main thread used for m_logicWorker
	//m_renderingWorker run on 2nd thread
	std::thread* m_renderingThread = 0;

	Scene* m_currentScene = 0;

public:
	//for raw view of scene if you don't create any camera
	ICamera* m_cam = 0;
	//for raw view
	SpaceCoordinate* m_spaceCoord = 0;
	//for raw view
	IRenderableObject* m_skyBox = 0;

public:
	Engine(const wchar_t* title, int width, int height);
	~Engine();

public:
	void Update() override;
	void Render();

	void Timing();

public:
	inline auto Input() { return m_input; };
	inline auto Renderer() { return m_renderer; };
	inline auto ResourceManager() { return m_resourceManager; };
	inline auto RenderPipelineManager() { return m_rplManager; };

	inline auto GetRenderingWorker() { return m_renderingWorker; };
	inline auto GetLogicWorker() { return m_logicWorker; };

	//in milisec
	inline auto& DeltaTime() { return m_deltaTime; };

	//in sec
	inline auto& FDeltaTime() { return m_fDeltaTime; };

	//in milisec, from 1/1/1970
	inline auto& CurrentTime() { return m_currentTime; };

	//in sec, from 1/1/1970
	inline auto& FCurrentTime() { return m_fCurrentTime; };

	//in sec, from start engine
	inline auto& FTime() { return m_time; };


	inline auto& CurrentScene() { return m_currentScene; };

};