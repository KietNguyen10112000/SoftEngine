#include "RenderingWorker.h"

#include "Core/Renderer.h"
#include "Engine/Engine.h"
#include "Components/SpaceCoordinate.h"

#include "Engine/Scene/Scene.h"

#include "Components/AABBRenderer.h"

#ifdef DX11_RENDERER
#include "RenderAPI/DX11/DX11Global.h"
#endif

RenderingWorker::RenderingWorker(Engine* engine) :
	m_engine(engine)
{
	m_renderer = engine->Renderer();

	m_queryContext = engine->CurrentScene()->NewQueryContext();

	m_aabbRenderer = new AABBRenderer();
}

RenderingWorker::~RenderingWorker()
{
	delete m_aabbRenderer;
}

void RenderingWorker::Update()
{
	RunSynch(RENDERING_TASK_HINT::RUN_AT_BEGIN_FRAME);

	if (!m_needReRender)
	{
		m_renderer->Present();
		Sleep(15);
		return;
	}

	auto* scene = m_engine->CurrentScene();

	//==========================Query from scene ===============================================

	m_dataNodes.clear();
	m_renderableObjects.clear();
	m_lightObjects.clear();

	m_queryContext->BeginFrame(); //call 1 time per frame

	scene->Query3D(m_queryContext, (Frustum*)0, m_dataNodes);

	for (auto& nodeid : m_dataNodes)
	{
		auto node = &m_queryContext->Node(nodeid);
		node->TransformTraverse(
			[&](SceneQueriedNode* curNode, const Mat4x4& globalTransform)
			{
				auto& sceneNode = curNode->GetSceneNode();

				//auto a = curNode->GetSceneNode();

				//if (!sceneNode->IsStateChange()) return false;

				switch (sceneNode->Type())
				{
				case SceneNode::RENDERABLE_NODE:
				{
					auto obj = sceneNode->RenderingObject().renderableObject;

					if (sceneNode->IsStateChange())
					{
						obj->Transform() = globalTransform;
						m_queryContext->DecrementState(curNode);
						sceneNode->m_aabb = obj->GetAABB();
						//sceneNode->StateChange()--;
					}

					m_renderableObjects.push_back(obj);
					m_aabbRenderer->Add(sceneNode->m_aabb);
				}
				break;
				case SceneNode::CAMERA_NODE:
				{
					auto camera = sceneNode->RenderingObject().camera;
					auto& blob = curNode->Blob();

					if (sceneNode->IsStateChange())
					{
						camera->ProjectionMatrix() = blob.AsCamera().proj;
						camera->Transform() = globalTransform;
						camera->MVP() = GetInverse(camera->Transform()) * camera->ProjectionMatrix();
						m_queryContext->DecrementState(curNode);
						//sceneNode->StateChange()--;

						//std::cout << sceneNode->StateChange() << "\n";
					}
					
				}
				break;
				case SceneNode::LIGHT_NODE:
				{
					auto lightId = sceneNode->RenderingObject().lightID;
					auto& blob = curNode->Blob();

					auto& light = m_renderer->LightSystem()->GetLight(lightId);

					if (sceneNode->IsStateChange() || light.type == LIGHT_TYPE::CSM_DIRECTIONAL_LIGHT)
					{
						light = blob.AsLight();
						m_renderer->LightSystem()->UpdateLight(lightId);
						m_renderer->LightSystem()->UpdateShadow(lightId);
						
						if (sceneNode->IsStateChange()) m_queryContext->DecrementState(curNode);
						//sceneNode->StateChange()--;
					}

					m_lightObjects.push_back(lightId);
				}
				break;
				default:
					break;
				}

				return true;
			},
			nullptr
		);
	}
	m_queryContext->EndFrame(); //call 1 time per frame
	//==========================End query======================================================

	m_renderer->LightSystem()->Update();

	static float clsCol[] = { 0,0,0,0 };
	m_renderer->ClearFrame(clsCol);

	m_engine->m_spaceCoord->Render(m_renderer);
	m_aabbRenderer->Present(m_renderer);

	for (auto& obj : m_renderableObjects)
	{
		obj->Update(m_engine);
		obj->Render(m_renderer);
	}

	for (auto& light : m_lightObjects)
	{
		if (m_renderer->LightSystem()->BeginShadow(light))
		{
			for (auto& obj : m_renderableObjects)
			{
				obj->Render(m_renderer);
			}
			m_renderer->LightSystem()->EndShadow(light);
		}
	}

	RunSynch(RENDERING_TASK_HINT::RUN_BEFORE_PRESENT_TO_SCREEN);
	RunSynch(RENDERING_TASK_HINT::RUN_AUDIO);
	m_renderer->Present();

	switch (m_renderingMode)
	{
	case RENDERING_MODE::REALTIME:
		m_needReRender = true;
		break;
	case RENDERING_MODE::MANUALLY_REFRESH:
		m_needReRender = false;
		break;
	default:
		break;
	}
	//m_renderer->VisualizeBackgroundRenderPipeline(1);

	RunSynch(RENDERING_TASK_HINT::RUN_AT_END_FRAME);
#ifndef IMGUI
	static int visualizeArg = 3;

	if (m_engine->Input()->GetPressKey(UP_ARROW))
	{
		visualizeArg = (visualizeArg + 1) % 3;
	}

	if (visualizeArg < 2)
	{
		m_renderer->VisualizeBackgroundRenderPipeline(visualizeArg);
	}
	else
	{
		m_renderer->Present();
	}

#ifdef DX11_RENDERER
	DX11Global::renderer->m_dxgiSwapChain->Present(1, 0);
#endif
#endif // !IMGUI
	
}

void RenderingWorker::Refresh()
{
	static bool once = false;

	if (once) return;
	once = true;

	RunAsync([&]() { once = false;  m_needReRender = true; }, RENDERING_TASK_HINT::RUN_AT_BEGIN_FRAME);
}
