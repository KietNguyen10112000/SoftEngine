#pragma once

#include "Core/TypeDef.h"

#include "Core/Structures/Managed/Array.h"

NAMESPACE_BEGIN

class Scene;
class Input;

class Engine : Traceable<Engine>
{
public:
	constexpr static byte STABLE_VALUE = 127;

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

public:
	static Handle<Engine> Initialize();
	static void Finalize();

	Engine();
	~Engine();

	void Setup();

	void Run();

	void Iteration();

	void ProcessInput();

	void SynchronizeAllSubSystems();

};


NAMESPACE_END