#pragma once
#include <Window.h>

#define _TIME_FACTOR 1000.0f

class IRenderer;
class Resource;
class RenderPipelineManager;

class Engine : public Window
{
private:
	uint64_t m_currentTime = 0;
	uint64_t m_deltaTime = 0;
	float m_time = 0;

	IRenderer* m_renderer = nullptr;
	Resource* m_resourceManager = nullptr;
	RenderPipelineManager* m_rplManager = nullptr;

public:
	Engine(const wchar_t* title, int width, int height);
	~Engine();

public:
	void Update() override;
	void Render();

public:
	inline auto Input() { return m_input; };
	inline auto Renderer() { return m_renderer; };
	inline auto ResourceManager() { return m_resourceManager; };
	inline auto RenderPipelineManager() { return m_rplManager; };

	//in milisec
	inline auto DeltaTime() { return m_deltaTime; };

	//in sec
	inline auto FDeltaTime() { return m_deltaTime / _TIME_FACTOR; };

	//in milisec, from 1/1/1970
	inline auto CurrentTime() { return m_currentTime; };

	//in sec, from 1/1/1970
	inline auto FCurrentTime() { return m_currentTime / _TIME_FACTOR; };

	//in sec, from start engine
	inline auto FTime() { return m_time; };

};