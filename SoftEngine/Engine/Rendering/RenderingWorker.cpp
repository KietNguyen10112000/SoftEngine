#include "RenderingWorker.h"

#include "Core/Renderer.h"

#include "Engine/Engine.h"
#include "Engine/Scene/Scene.h"
#include "Engine/WorkerConfig.h"

#include "Components/SpaceCoordinate.h"
#include "Components/AABBRenderer.h"

#ifdef DX11_RENDERER
#include "RenderAPI/DX11/DX11Global.h"
#endif

struct RenderingWorker::Data
{
	std::vector<SceneObject*> objects;
	std::vector<SharedPtr<SceneSharedObject>> sharedObjects;
	std::vector<IRenderableObject*> renderableObjects;
	std::vector<LightID> lights;
};


RenderingWorker::RenderingWorker(Engine* engine) :
	m_engine(engine)
{
	m_renderer = engine->Renderer();

	m_queryContext = engine->CurrentScene()->NewQueryContext(RENDERING_WORKER_ID);

	m_aabbRenderer = new AABBRenderer();

	m_data = new RenderingWorker::Data();
}

RenderingWorker::~RenderingWorker()
{
	delete m_aabbRenderer;
	delete m_data;
}

void RenderingWorker::Update()
{
	auto& objects = m_data->objects;
	auto& sharedObjects = m_data->sharedObjects;
	auto& renderableObjects = m_data->renderableObjects;
	auto& lightObjects = m_data->lights;

	RunSynch(RENDERING_TASK_HINT::RUN_AT_BEGIN_FRAME);

	if (!m_needReRender)
	{
		m_renderer->PresentLastFrame();
		//m_renderer->VisualizeBackgroundRenderPipeline(0);
		Sleep(15);
		return;
	}

	auto* scene = m_engine->CurrentScene();

	//==========================Query from scene ===============================================

	objects.clear();
	sharedObjects.clear();
	renderableObjects.clear();
	lightObjects.clear();

	m_queryContext->BeginQuery(); //call 1 time per frame

	scene->Query3DObjects(m_queryContext, (Frustum*)0, objects);

	for (auto& obj : objects)
	{
		obj->TransformTraverse(
			[&](SceneObject* curObj, const Mat4x4& globalTransform)
			{
				switch (curObj->Type())
				{
				case SceneObject::RENDERABLE_OBJECT:
				{
					auto rdObj = curObj->RenderingObject().renderableObject;

					if (curObj->DataChanged())
					{
						rdObj->Transform() = globalTransform;
						curObj->m_aabb = rdObj->GetAABB();
						curObj->DataChanged() = false;
					}

					// large external data case
					if (curObj->ExternalDataChanged())
					{
						rdObj->ReadExternalDataFrom(curObj->ExternalData());
						curObj->ExternalDataChanged() = false;
					}

					renderableObjects.push_back(rdObj);
					m_aabbRenderer->Add(curObj->m_aabb);
				}
				break;
				case SceneObject::CAMERA_OBJECT:
				{
					auto camera = curObj->RenderingObject().camera;

					auto& externalData = curObj->ExternalData();

					if (curObj->DataChanged())
					{
						camera->ProjectionMatrix() = externalData.As<Mat4x4>();
						camera->Transform() = globalTransform;
						camera->MVP() = GetInverse(camera->Transform()) * camera->ProjectionMatrix();

						curObj->DataChanged() = false;

						// known what extract is camera external data
						curObj->ExternalDataChanged() = false;
					}
					
				}
				break;
				case SceneObject::LIGHT_OBJECT:
				{
					auto lightId = curObj->RenderingObject().lightID;

					auto& externalData = curObj->ExternalData();

					auto& light = m_renderer->LightSystem()->GetLight(lightId);

					if (curObj->DataChanged() || light.type == LIGHT_TYPE::CSM_DIRECTIONAL_LIGHT)
					{
						light = externalData.As<Light>();
						m_renderer->LightSystem()->UpdateLight(lightId);
						m_renderer->LightSystem()->UpdateShadow(lightId);
						
						curObj->DataChanged() = false;
					}

					lightObjects.push_back(lightId);
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

	m_queryContext->EndQuery(); //call 1 time per frame
	scene->QueryContextReleaseReadingObjects(m_queryContext, objects);
	scene->QueryContextReleaseReadingObjects(m_queryContext, sharedObjects);
	//==========================End query======================================================

	m_renderer->LightSystem()->Update();

	static float clsCol[] = { 0,0,0,0 };
	m_renderer->ClearFrame(clsCol);

	//Sleep(12);

	for (auto& obj : renderableObjects)
	{
		obj->Update(m_engine);
		obj->Render(m_renderer);
	}

	for (auto& light : lightObjects)
	{
		if (m_renderer->LightSystem()->BeginShadow(light))
		{
			for (auto& obj : renderableObjects)
			{
				obj->Render(m_renderer);
			}
			m_renderer->LightSystem()->EndShadow(light);
		}
	}

	RunSynch(RENDERING_TASK_HINT::RUN_BEFORE_PRESENT_TO_SCREEN);
	RunSynch(RENDERING_TASK_HINT::RUN_AUDIO);

	m_renderer->Present();

	m_renderer->BeginUI(1);
	m_engine->m_spaceCoord->Render(m_renderer);
	m_aabbRenderer->Present(m_renderer);
	m_renderer->EndUI();

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
