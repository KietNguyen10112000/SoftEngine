#pragma once

#include "Core/TypeDef.h"

#include "Core/Structures/Managed/Array.h"
#include "Core/Structures/STD/STDContainers.h"

#include "Objects/Event/EventDispatcher.h"

#include "SFML/Graphics.hpp"

NAMESPACE_BEGIN

class Scene2D;
class Input;
class Plugin;

class IterationHandler
{
public:
	// sumDt is sum of delta time from previous iteration
	// return remain sumDt
	virtual float DoIteration(float sumDt, Scene2D* scene) = 0;

};

class API Engine : Traceable<Engine>, public EventDispatcher
{
public:
	constexpr static byte STABLE_VALUE	= 127;
	constexpr static byte NUM_ARGS		= 128;

private:
	Array<Handle<Scene2D>> m_scenes;

	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_scenes);
	}

	Input* m_input = nullptr;
	sf::Window* m_window;

	bool m_isRunning = true;

	std::atomic<bool> m_gcIsRunning = false;

	std::Vector<Plugin*> m_plugins;
	std::Vector<Plugin*> m_intevalPlugins;

	void* m_eventArgv[NUM_ARGS] = {};

	IterationHandler* m_iterationHandler = nullptr;

public:
	static Handle<Engine> Initialize();
	static void Finalize();

	Engine();
	~Engine();

private:
	void InitGraphics();
	void FinalGraphics();

	void InitNetwork();
	void FinalNetwork();

	void InitPlugins();
	void FinalPlugins();

public:
	void Setup();

	void Run();

	void Iteration();

	void ProcessInput();

	void SynchronizeAllSubSystems();

public:
	inline auto GetInput()
	{
		return m_input;
	}

	inline auto& IsRunning()
	{
		return m_isRunning;
	}

	inline auto SetIterationHandler(IterationHandler* handler)
	{
		m_iterationHandler = handler;
	}

};


NAMESPACE_END