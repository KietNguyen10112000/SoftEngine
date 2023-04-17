#include "RenderingSystem.h"

#include <iostream>

#include "Components/Rendering/Rendering.h"
#include "Components/Rendering/Camera.h"

#include "Graphics/Graphics.h"
#include "Graphics/GraphicsCommandList.h"
#include "Graphics/DebugGraphics.h"

NAMESPACE_BEGIN

RenderingSystem::RenderingSystem(Scene* scene) : SubSystem(scene, Rendering::COMPONENT_ID)
{
	m_dynamicQuerySession = scene->NewDynamicQuerySession();
}

void RenderingSystem::PrevIteration(float dt)
{
}

void RenderingSystem::Iteration(float dt)
{
	constexpr float width = 0.05f;
	static Mat4 xAxis = Mat4::Identity() * Mat4::Scaling(10000.0f, width, width);
	static Mat4 yAxis = Mat4::Identity() * Mat4::Scaling(width, 10000.0f, width);
	static Mat4 zAxis = Mat4::Identity() * Mat4::Scaling(width, width, 10000.0f);
	static Mat4 rootPoint = Mat4::Identity() * Mat4::Scaling(0.1f, 0.1f, 0.1f);

	GraphicsCommandList* cmdList = nullptr;

	auto graphics = Graphics::Get();

	if (!graphics) return;

	auto dbg = graphics->GetDebugGraphics();

	graphics->Bind(this);

	graphics->BeginFrame(&cmdList);

	cmdList->ClearScreen({ 0.0f, 0.2f, 0.4f, 1.0f });

	size_t sumObj = 0;

	if (!m_cameraObjects.empty())
	{
		auto mainCam = m_cameraObjects[0];
		auto cam = mainCam->GetComponentRaw<Camera>();

		Frustum frustum = Frustum(cam->GetProj());

		m_dynamicQuerySession->Clear();
		m_scene->AABBDynamicQueryFrustum(frustum, m_dynamicQuerySession.get());

		graphics->BeginCamera(cam);

		dbg->DrawCube(xAxis, { 1.0f,0.0f,0.0f,1.0f });
		dbg->DrawCube(yAxis, { 0.0f,1.0f,0.0f,1.0f });
		dbg->DrawCube(zAxis, { 0.0f,0.0f,1.0f,1.0f });
		dbg->DrawCube(rootPoint, { 1.0f,1.0f,1.0f,1.0f });

		
		{
			auto it = m_scene->m_tempObjects.begin();
			auto end = m_scene->m_tempObjects.end();
			while (it != end)
			{
				auto gameObject = *it;
				if (!frustum.IsOverlap(gameObject->GetAABB()))
				{
					sumObj++;
					dbg->DrawAABox(gameObject->GetAABB(), { 0.5f,0.0f,0.0f,1.0f });
				}
				/*else
				{
					dbg->DrawAABox(gameObject->GetAABB(), { 0.5f,0.5f,0.5f,1.0f });
				}*/
				it++;
			}
		}

		{
			auto it = m_dynamicQuerySession->begin;
			auto end = m_dynamicQuerySession->end;
			//auto it = m_scene->m_tempObjects.begin();
			//auto end = m_scene->m_tempObjects.end();
			while (it != end)
			{
				sumObj++;
				auto gameObject = *it;
				dbg->DrawAABox(gameObject->GetAABB(), { 0.5f,0.5f,0.5f,1.0f });
				it++;
			}
		}
		

		//dbg->DrawCube({}, {});

		graphics->EndCamera(cam);
	}

	assert(sumObj >= m_scene->m_tempObjects.size() - 2);

	graphics->EndFrame(&cmdList);
}

void RenderingSystem::PostIteration(float dt)
{
}


NAMESPACE_END
