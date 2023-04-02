#include "RenderingSystem.h"

#include <iostream>

#include "Components/Rendering/Rendering.h"

#include "Graphics/Graphics.h"
#include "Graphics/GraphicsCommandList.h"

NAMESPACE_BEGIN

RenderingSystem::RenderingSystem(Scene* scene) : SubSystem(scene, Rendering::COMPONENT_ID)
{
}

void RenderingSystem::PrevIteration(float dt)
{
}

void RenderingSystem::Iteration(float dt)
{
	auto graphics = Graphics::Get();
	graphics->BeginFrame();

	GraphicsCommandList* cmdList = nullptr;
	graphics->BeginCommandList(&cmdList);

	cmdList->ClearScreen({ 0.0f, 0.2f, 0.4f, 1.0f });

	graphics->EndCommandList(&cmdList);

	graphics->EndFrame();
}

void RenderingSystem::PostIteration(float dt)
{
}


NAMESPACE_END
