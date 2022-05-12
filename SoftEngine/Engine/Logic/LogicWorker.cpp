#include "LogicWorker.h"

#include "Core/MultiThreading/FrameRateControl.h"

#include "Engine/Engine.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/SceneObject.h"

#include "Engine/Controllers/Controller.h"
#include "Engine/Scripting/ScriptEngine.h"
#include "Engine/Physics/PhysicsEngine.h"
#include "Engine/Physics/PhysicsObject.h"

#include "Engine/UI/ImGuiCommon.h"
#include "Engine/WorkerConfig.h"

#include "Components/FPPCamera.h"


#if defined(IMGUI)
ImGuiCommon::Console g_imGuiConsole;
#endif


struct LogicWorker::Data
{
	std::vector<SceneObject*> objects;
	std::vector<SharedPtr<SceneSharedObject>> sharedObjects;
	std::vector<IRenderableObject*> renderableObjects;
	std::vector<LightID> lights;
};


LogicWorker::LogicWorker(Engine* engine) :
	m_engine(engine)
{
	m_input = engine->Input();
	m_queryContext = engine->CurrentScene()->NewQueryContext(LOGIC_WORKER_ID);

	m_data = new LogicWorker::Data();
}

LogicWorker::~LogicWorker()
{
	delete m_data;
}

void LogicWorker::Update()
{
	auto& objects = m_data->objects;
	auto& sharedObjects = m_data->sharedObjects;
	auto& renderableObjects = m_data->renderableObjects;
	auto& lightObjects = m_data->lights;

#if defined(IMGUI)
	g_imGuiConsole.RedirectStdOutput();
#endif

	auto scene = m_engine->CurrentScene();

#if defined(IMGUI) && defined(DX11_RENDERER)
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//ImGui::EndFrame();
#endif
	//int64_t currentTime = GetTime();

	//==========================Query from scene ===============================================
	objects.clear();
	sharedObjects.clear();
	renderableObjects.clear();
	lightObjects.clear();

	m_queryContext->BeginQuery();

	scene->Query3DObjects(m_queryContext, (Frustum*)0, objects);

	// assume read data, process physics take 10ms
	// Sleep(2);

	if (scene->GetPhysicsEngine())
		scene->GetPhysicsEngine()->StepSimulation(m_engine->FDeltaTime(), 0, 0);

	m_queryContext->EndQuery();

	scene->CheckNoQueries();

	for (auto& obj : objects)
	{
		if (!obj->PhysicsObject() || obj->Controller()) continue;

		if (obj->PhysicsObject()->IsDynamic())
		{
			obj->Transform() = obj->PhysicsObject()->GetTransform();
			obj->DataChanged() = true;
		}
	}

	for (auto& obj : objects)
	{
		obj->TransformTraverse(
			[&](SceneObject* curObj, const Mat4x4& globalTransform)
			{
				auto controller = curObj->Controller();
				if (controller)
				{
					controller->Update(m_engine);
					controller->WriteTo(curObj);
				}
				return true;
			},
			nullptr
		);
	}

	LastUpdate();

#if defined(IMGUI)
	g_imGuiConsole.RestoreStdOutput();
#endif

	FrameRateControl::Control(60, m_engine->CurrentTime() / 1'000'000);
}

void LogicWorker::LastUpdate()
{
	auto scene = m_engine->CurrentScene();
	//allow ScriptEngine direct update to scene
	scene->InterlockedAcquire(m_queryContext);

	g_imGuiConsole.Update(scene);

	if (scene->GetScriptEngine())
		scene->GetScriptEngine()->Update(m_engine, scene);

	scene->InterlockedRelease(m_queryContext);
}

void LogicWorker::Idling()
{
#if defined(IMGUI)
	g_imGuiConsole.RedirectStdOutput();
#endif

#if defined(IMGUI) && defined(DX11_RENDERER)
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#endif

	auto scene = m_engine->CurrentScene();

	m_queryContext->BeginQuery();
	auto cam = scene->GetDebugCamera();
	Sleep(2);
	m_queryContext->EndQuery();
	
	cam->Controller()->Update(m_engine);
	cam->Controller()->WriteTo(cam);

	LastUpdate();

#if defined(IMGUI)
	g_imGuiConsole.RestoreStdOutput();
#endif

	FrameRateControl::Control(60, m_engine->CurrentTime() / 1'000'000);
}

