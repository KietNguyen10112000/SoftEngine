#include "RenderingWorker.h"

#include "Core/Renderer.h"
#include "Engine/Engine.h"
#include "Components/SpaceCoordinate.h"

#include "Engine/Scene/Scene.h"

#ifdef DX11_RENDERER
#include "RenderAPI/DX11/DX11Global.h"
#endif

RenderingWorker::RenderingWorker(Engine* engine) :
	m_engine(engine)
{
	m_renderer = engine->Renderer();

	m_queryContext = engine->CurrentScene()->NewQueryContext();
}

RenderingWorker::~RenderingWorker()
{
}

void RenderingWorker::Update()
{	
	auto* scene = m_engine->CurrentScene();

	//==========================Query from scene ===============================================

	m_dataNodes.clear();
	m_renderableObjects.clear();
	m_lightObjects.clear();

	m_queryContext->BeginFrame(); //call 1 time per frame

	scene->Query3D(m_queryContext, (Frustum*)0, m_dataNodes);

	for (auto& node : m_dataNodes)
	{
		node->TransformTraverse(
			[&](SceneQueriedNode* curNode, const Mat4x4& globalTransform)
			{
				auto sceneNode = curNode->GetSceneNode();

				//if (!sceneNode->IsStateChange()) return false;

				switch (sceneNode->Type())
				{
				case SceneNode::RENDERABLE_NODE:
				{
					auto obj = sceneNode->RenderingObject().renderableObject;
					m_renderableObjects.push_back(obj);

					if (sceneNode->IsStateChange())
					{
						obj->Transform() = globalTransform;
						m_queryContext->DecrementState(curNode);
						//sceneNode->StateChange()--;
					}
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

	for (auto& obj : m_renderableObjects)
	{
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

	m_renderer->Present();

	
	//m_renderer->VisualizeBackgroundRenderPipeline(1);

#ifdef DX11_RENDERER
	DX11Global::renderer->m_dxgiSwapChain->Present(1, 0);
#endif
}
