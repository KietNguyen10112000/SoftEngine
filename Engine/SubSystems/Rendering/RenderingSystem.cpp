#include "RenderingSystem.h"

#include <iostream>

#include "Components/Rendering/Rendering.h"

NAMESPACE_BEGIN

RenderingSystem::RenderingSystem(Scene* scene) : SubSystem(scene, Rendering::COMPONENT_ID)
{
}

void RenderingSystem::PrevIteration(float dt)
{
}

void RenderingSystem::Iteration(float dt)
{
	std::cout << "RenderingSystem::Iteration()\n";
}

void RenderingSystem::PostIteration(float dt)
{
}


NAMESPACE_END
