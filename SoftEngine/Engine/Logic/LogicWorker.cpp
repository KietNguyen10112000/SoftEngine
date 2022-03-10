#include "LogicWorker.h"

#include "Engine/Engine.h"
#include "Components/FPPCamera.h"

#include "Engine/Scene/Scene.h"

#include "Engine/Controllers/Controller.h"

LogicWorker::LogicWorker(Engine* engine) :
	m_engine(engine)
{
	m_input = engine->Input();
	m_queryContext = engine->CurrentScene()->NewQueryContext();
}

LogicWorker::~LogicWorker()
{
}

void LogicWorker::Update()
{
	//int64_t currentTime = GetTime();

	auto scene = m_engine->CurrentScene();


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
}
