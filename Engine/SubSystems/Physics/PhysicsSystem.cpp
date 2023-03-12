#include "PhysicsSystem.h"

#include <iostream>

#include "Components/Physics/Physics.h"

NAMESPACE_BEGIN

PhysicsSystem::PhysicsSystem(Scene* scene) : SubSystem(scene, Physics::COMPONENT_ID)
{
}

void PhysicsSystem::PrevIteration(float dt)
{
}

void PhysicsSystem::Iteration(float dt)
{
	std::cout << "PhysicsSystem::Iteration()\n";
}

void PhysicsSystem::PostIteration(float dt)
{
}

NAMESPACE_END