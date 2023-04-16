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
}

void RenderingSystem::PrevIteration(float dt)
{
}

void RenderingSystem::Iteration(float dt)
{
	static Mat4 xAxis = Mat4::Identity() * Mat4::Scaling(10000.0f, 0.05f, 0.05f);
	static Mat4 yAxis = Mat4::Identity() * Mat4::Scaling(0.05f, 10000.0f, 0.05f);
	static Mat4 zAxis = Mat4::Identity() * Mat4::Scaling(0.05f, 0.05f, 10000.0f);

	GraphicsCommandList* cmdList = nullptr;

	auto graphics = Graphics::Get();

	if (!graphics) return;

	auto dbg = graphics->GetDebugGraphics();

	graphics->Bind(this);

	graphics->BeginFrame(&cmdList);

	cmdList->ClearScreen({ 0.0f, 0.2f, 0.4f, 1.0f });

	if (!m_cameraObjects.empty())
	{
		auto mainCam = m_cameraObjects[0];
		graphics->BeginCamera(mainCam->GetComponentRaw<Camera>());

		dbg->DrawCube(xAxis, { 1.0f,0.0f,0.0f,1.0f });
		dbg->DrawCube(yAxis, { 0.0f,1.0f,0.0f,1.0f });
		dbg->DrawCube(zAxis, { 0.0f,0.0f,1.0f,1.0f });


		//dbg->DrawCube({}, {});

		graphics->EndCamera(mainCam->GetComponentRaw<Camera>());
	}

	graphics->EndFrame(&cmdList);
}

void RenderingSystem::PostIteration(float dt)
{
}


NAMESPACE_END
