#include "LogicWorker.h"

#include "Engine/Engine.h"
#include "Components/FPPCamera.h"

#include "Engine/Scene/Scene.h"
#include "Engine/Controllers/Controller.h"
#include "Engine/Scripting/ScriptEngine.h"

#include "Engine/UI/ImGuiCommon.h"

#if defined(IMGUI)
ImGuiCommon::Console g_imGuiConsole;
#endif

LogicWorker::LogicWorker(Engine* engine) :
	m_engine(engine)
{
	m_input = engine->Input();
	m_queryContext = engine->CurrentScene()->NewQueryContext();
}

LogicWorker::~LogicWorker()
{
	//auto scene = m_engine->CurrentScene();
	//scene->UpdateRefCounted();
}

void LogicWorker::Update()
{
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
	m_dataNodes.clear();
	m_lightObjects.clear();

	m_queryContext->BeginFrame();

	scene->Query3D(m_queryContext, (Frustum*)0, m_dataNodes);

	for (auto& nodeid : m_dataNodes)
	{
		auto node = &m_queryContext->Node(nodeid);
		node->TransformTraverse(
			[&](SceneQueriedNode* curNode, const Mat4x4& globalTransform)
			{
				auto controller = (Controller*)curNode->GetSceneNode()->Controller();
				if (controller)
				{
					return controller->Update(m_queryContext, curNode, globalTransform, m_engine);
				}
				return true;
			},
			nullptr
		);
	}

	m_queryContext->EndFrame();

	LastUpdate();

#if defined(IMGUI)
	g_imGuiConsole.RestoreStdOutput();
#endif
}

void LogicWorker::LastUpdate()
{
	auto scene = m_engine->CurrentScene();
	//allow ScriptEngine direct update to scene
	scene->BeginUpdate(m_queryContext);

	g_imGuiConsole.Update(scene);

	scene->GetScriptEngine()->Update(m_engine, scene);

	scene->UpdateRefCounted();

	scene->EndUpdate(m_queryContext);
}

