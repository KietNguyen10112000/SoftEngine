#pragma once

#include "Core/TypeDef.h"

#include "Core/Structures/Managed/Array.h"
#include "Core/Structures/STD/STDContainers.h"

NAMESPACE_BEGIN

class Input;
class Plugin;
class Scene;

class IterationHandler
{
public:
	// sumDt is sum of delta time from previous iteration
	// return remain sumDt
	virtual float DoIteration(float sumDt, Scene* scene) = 0;

};

class API Runtime : Traceable<Runtime>
{
public:
	constexpr static byte STABLE_VALUE	= 127;
	constexpr static byte NUM_ARGS		= 128;

private:
	Array<Handle<Scene>> m_scenes;

	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_scenes);
	}

	Input* m_input = nullptr;
	void* m_window = nullptr;

	bool m_isRunning = true;

	std::atomic<bool> m_gcIsRunning = false;

	std::Vector<Plugin*> m_plugins;
	std::Vector<Plugin*> m_intevalPlugins;

	void* m_eventArgv[NUM_ARGS] = {};

	IterationHandler* m_iterationHandler = nullptr;

public:
	static Handle<Runtime> Initialize();
	static void Finalize();

	Runtime();
	~Runtime();

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