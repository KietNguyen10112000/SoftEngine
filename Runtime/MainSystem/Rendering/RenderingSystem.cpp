#include "RenderingSystem.h"

#include "Core/Thread/Thread.h"

#include "Components/Camera.h"

#include "Modules/Graphics/Graphics.h"

//#include "Modules/Graphics/Detail/DX12/Shaders/Common/TypeDef.hlsli"

#include "Modules/Resources/Texture2D.h"

#include "Scene/Scene.h"
#include "Input/Input.h"

#include "DisplayService.h"

#include "Graphics/DebugGraphics.h"

//#include "imgui/imgui.h"

NAMESPACE_BEGIN

RenderingSystem::RenderingSystem(Scene* scene) : MainSystem(scene), m_eventDispatcher(this)
{
	auto graphics = Graphics::Get();

    m_defaultViewport.topLeft = { 0,0 };
    m_defaultViewport.size = { Graphics::Get()->GetWindowWidth(), Graphics::Get()->GetWindowHeight() };

	m_cameras.resize(8 * KB);
	m_collectInputForCameraTasks.resize(8 * KB);
	m_collectInputForCameraParams.resize(8 * KB);

	m_collectInputForCameraRets.resize(128);
	for (auto& ss : m_collectInputForCameraRets)
	{
		ss = m_bvh.NewQuerySession();
	}
}

RenderingSystem::~RenderingSystem()
{
	for (auto& ss : m_collectInputForCameraRets)
	{
		m_bvh.DeleteQuerySession(ss);
	}
	m_collectInputForCameraRets.clear();
}

void RenderingSystem::AddCamera(BaseCamera* camera, CAMERA_PRIORITY priority)
{
	assert(camera->m_activeID == INVALID_ID);

    auto& activeCamera = m_activeCamera[priority];

    camera->m_priority = priority;
	camera->m_activeID = activeCamera.size();
    activeCamera.push_back(camera);

    if (camera->m_pipeline)
    {
        camera->m_pipeline->Bake(camera->m_renderTarget.get(), camera->m_depthBuffer.get(), this);
    }
}

void RenderingSystem::RemoveCamera(BaseCamera* camera)
{
	assert(camera->m_activeID != INVALID_ID);

    auto& activeCamera = m_activeCamera[camera->m_priority];

    if (camera->m_isDisplaying)
    {
        HideCamera(camera);
    }

	STD_VECTOR_ROLL_TO_FILL_BLANK(activeCamera, camera, m_activeID);
	camera->m_activeID = INVALID_ID;
}

void RenderingSystem::CollectInputForEachCamera()
{
	size_t totalCamera = 0;
	m_cameras.clear();
	for (auto& chunk : m_activeCamera)
	{
		totalCamera += chunk.size();
		m_cameras.insert(m_cameras.end(), chunk.begin(), chunk.end());
	}

	if (totalCamera == 0)
	{
		return;
	}

	auto numSS = m_collectInputForCameraRets.size();
	if (totalCamera > numSS)
	{
		m_collectInputForCameraRets.resize(totalCamera);
		for (uint32_t i = numSS; i < totalCamera; i++)
		{
			m_collectInputForCameraRets[i] = m_bvh.NewQuerySession();
		}
	}

	m_collectInputForCameraTasks.resize(totalCamera);
	m_collectInputForCameraParams.resize(totalCamera);
	for (size_t i = 0; i < totalCamera; i++)
	{
		auto& task = m_collectInputForCameraTasks[i];
		auto& params = m_collectInputForCameraParams[i];

		params.camera = m_cameras[i];
		params.renderingSystem = this;
		params.querySession = m_collectInputForCameraRets[i];

		task.Entry() = [](void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_3(CollectInputForCameraParams, p, renderingSystem, camera, querySession);

			auto frustum = Frustum(camera->Projection());
			frustum.Transform(camera->GlobalTransform());
			auto queryStructure = &renderingSystem->m_bvh;

			querySession->ClearPrevQueryResult();
			queryStructure->QueryFrustum(frustum, querySession);

			int x = 3;
		};

		task.Params() = &params;
	}

	TaskSystem::SubmitAndWait(m_collectInputForCameraTasks.data(), totalCamera, Task::CRITICAL);
}

void RenderingSystem::SetBuiltinConstantBufferForCamera(BaseCamera* camera)
{
	m_cameraData.transform = camera->GlobalTransform();
	m_cameraData.proj = camera->Projection();
	m_cameraData.view = camera->GetView();
	m_cameraData.vp = m_cameraData.view * m_cameraData.proj;
	m_cameraData.inversedVp = m_cameraData.proj.GetInverse() * camera->GlobalTransform();

	/*ImGui::Begin("Debug info");
	ImGui::DragFloat4("row1", &m_cameraData.transform[0][0]);
	ImGui::DragFloat4("row2", &m_cameraData.transform[1][0]);
	ImGui::DragFloat4("row3", &m_cameraData.transform[2][0]);
	ImGui::DragFloat4("row4", &m_cameraData.transform[3][0]);
	ImGui::End();*/

	/*m_cameraData.vp = Mat4::Identity().SetLookAtLH({ 10,10,10 }, Vec3(0, 5, 5), Vec3::UP)
		* Mat4::Identity().SetPerspectiveFovLH(PI / 3.0f, 
			StartupConfig::Get().windowWidth / (float)StartupConfig::Get().windowHeight, 0.5f, 1000.0f);*/

	auto& camCBuffer = GetBuiltinConstantBuffers()->GetCameraBuffer();
	camCBuffer->UpdateBuffer(&m_cameraData, sizeof(m_cameraData));
}

void RenderingSystem::RenderForEachCamera()
{
	auto numCamera = m_cameras.size();
	for (size_t i = 0; i < numCamera; i++)
	{
		auto& cam = m_cameras[i];
		auto& pipeline = cam->m_pipeline;
		auto& input = m_collectInputForCameraRets[i]->Result();

		EventDispatcher()->Dispatch(EVENT::EVENT_BEGIN_RENDER_CAMERA, cam);

		SetBuiltinConstantBufferForCamera(cam);

		pipeline->SetInput((RenderingComponent**)input.data(), input.size());
		pipeline->Run();

		EventDispatcher()->Dispatch(EVENT::EVENT_END_RENDER_CAMERA, cam);
	}
}

void RenderingSystem::DisplayAllCamera()
{
	auto displayService = DisplayService::Get();
	displayService->Begin();

	for (auto& cam : m_displayingCamera)
	{
		auto rc = cam.camera->m_renderTarget->GetShaderResource();
		displayService->Display(rc, cam.viewport);
	}

	displayService->End();
}

void RenderingSystem::DisplayCamera(BaseCamera* camera, const GRAPHICS_VIEWPORT& viewport)
{
    camera->m_isDisplaying = true;
	m_displayingCamera.push_back({ camera, viewport });
}

void RenderingSystem::HideCamera(BaseCamera* camera)
{
    camera->m_isDisplaying = false;

	size_t i = 0;
Begin:
	i = 0;
	for (auto& dc : m_displayingCamera)
	{
		if (dc.camera == camera)
		{
			STD_VECTOR_ROLL_TO_FILL_BLANK_2(m_displayingCamera, i);
			goto Begin;
		}
		i++;
	}
}

void RenderingSystem::BeginModification()
{
}

void RenderingSystem::AddComponent(MainComponent* comp)
{
	m_bvh.RecordAddComponent(comp);
}

void RenderingSystem::RemoveComponent(MainComponent* comp)
{
	m_bvh.RecordRemoveComponent(comp);
}

void RenderingSystem::OnObjectTransformChanged(MainComponent* comp)
{
	m_bvh.RecordRefreshComponent(comp);
}

void RenderingSystem::EndModification()
{
	m_bvh.Reconstruct(5'000'000);
}

void RenderingSystem::Iteration(float dt)
{
	GetPrevAsyncTaskRunnerMT()->ProcessAllTasksMT(this);
	GetPrevAsyncTaskRunnerST()->ProcessAllTasks(this);

	auto graphics = Graphics::Get();
	graphics->BeginFrame();

	// do render
	//auto screenRT = graphics->GetScreenRenderTarget();
	//auto screenDS = graphics->GetScreenDepthStencilBuffer();

 //   graphics->SetRenderTargets(1, &screenRT, screenDS);

	////graphics->SetRenderTarget(1, &screenRT, screenDS);
	//graphics->ClearRenderTarget(screenRT, { 0.1f, 0.5f, 0.5f, 1.0f }, 0, 0);
	//graphics->ClearDepthStencil(screenDS, 0, 0);

 //   g_cameraData.vp = Mat4::Identity().SetLookAtLH(cameraPos, cameraPos + Vec3(-1,0,0), Vec3::UP)
 //       * Mat4::Identity().SetPerspectiveFovLH(PI / 3.0f, 
 //          StartupConfig::Get().windowWidth / (float)StartupConfig::Get().windowHeight, 0.5f, 1000.0f);
 //   g_objectData.transform *= Mat4::Rotation(Vec3::UP, dt * PI / 3.0f) * Mat4::Rotation(Vec3::RIGHT, dt * PI / 4.0f);
 //   m_testCameraConstantBuffer->UpdateBuffer(&g_cameraData, sizeof(g_cameraData));
 //   m_testObjectConstantBuffer->UpdateBuffer(&g_objectData, sizeof(g_objectData));

 //   auto params = m_testPipeline->PrepareRenderParams();
 //   params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 0, 1, &m_testCameraConstantBuffer);
 //   params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 1, 1, &m_testObjectConstantBuffer);
 //   params->SetShaderResources(GRAPHICS_SHADER_SPACE::SHADER_SPACE_PS, 0, 1, &m_testTexture2D->m_shaderResource);
 //   graphics->SetGraphicsPipeline(m_testPipeline.get());
 //   
 //   auto vb = m_testVertexBuffer.get();
 //   graphics->DrawInstanced(1, &vb, 36, 1, 0, 0);

 //   graphics->UnsetRenderTargets(1, &screenRT, screenDS);
	//Thread::Sleep(10);

	CollectInputForEachCamera();

	RenderForEachCamera();

#ifdef _DEBUG
	if (!graphics->GetDebugGraphics())
	{
		DisplayAllCamera();
		EventDispatcher()->Dispatch(EVENT::EVENT_RENDER_GUI);
		graphics->EndFrame();
	}
#else
	DisplayAllCamera();
	EventDispatcher()->Dispatch(EVENT::EVENT_RENDER_GUI);
	graphics->EndFrame();
#endif // _DEBUG
}

void RenderingSystem::PrevIteration()
{
}

void RenderingSystem::PostIteration()
{
}

void RenderingSystem::RenderWithDebugGraphics()
{
#ifdef _DEBUG
	auto& camCBuffer = GetBuiltinConstantBuffers()->GetCameraBuffer();
	auto graphics = Graphics::Get();
	auto debugGraphics = graphics->GetDebugGraphics();

	if (!debugGraphics)
	{
		return;
	}

	for (auto& cam : m_displayingCamera)
	{
		SetBuiltinConstantBufferForCamera(cam.camera);
		debugGraphics->RenderToTarget(cam.camera->m_renderTarget.get(), cam.camera->m_depthBuffer.get(), camCBuffer);
	}

	DisplayAllCamera();
	EventDispatcher()->Dispatch(EVENT::EVENT_RENDER_GUI);
	graphics->EndFrame();
#endif // _DEBUG

}


NAMESPACE_END