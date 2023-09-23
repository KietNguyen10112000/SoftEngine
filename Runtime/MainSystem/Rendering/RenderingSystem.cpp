#include "RenderingSystem.h"

#include "Components/Camera.h"

#include "Modules/Graphics/Graphics.h"
#include <Core/Thread/Thread.h>

NAMESPACE_BEGIN

RenderingSystem::RenderingSystem(Scene* scene) : MainSystem(scene)
{
}

RenderingSystem::~RenderingSystem()
{
}

void RenderingSystem::AddCamera(Camera* camera)
{
	assert(camera->m_activeID == INVALID_ID);

	camera->m_activeID = m_activeCamera.size();
	m_activeCamera.push_back(camera);
}

void RenderingSystem::RemoveCamera(Camera* camera)
{
	assert(camera->m_activeID != INVALID_ID);

	STD_VECTOR_ROLL_TO_FILL_BLANK(m_activeCamera, camera, m_activeID);
	camera->m_activeID = INVALID_ID;
}

void RenderingSystem::DisplayCamera(Camera* camera, const GRAPHICS_VIEWPORT& viewport)
{
	m_displayingCamera.push_back({ camera, viewport });
}

void RenderingSystem::HideCamera(Camera* camera)
{
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

void RenderingSystem::Iteration(float dt)
{
	m_bvh.Reconstruct(5'000'000);

	ProcessAllCmds(GetPrevServer(), this);

	auto graphics = Graphics::Get();
	graphics->BeginFrame();

	// do render

	auto screenRT = graphics->GetScreenRenderTarget();
	auto screenDS = graphics->GetScreenDepthStencilBuffer();

	//graphics->SetRenderTarget(1, &screenRT, screenDS);
	graphics->ClearRenderTarget(screenRT, { 0.1f, 0.5f, 0.5f, 1.0f }, 0, 0);
	graphics->ClearDepthStencil(screenDS, 0, 0);

	Thread::Sleep(14);

	graphics->EndFrame(true);
}

void RenderingSystem::PrevIteration()
{
	UpdateCurrentServer();
}

void RenderingSystem::PostIteration()
{
}


NAMESPACE_END