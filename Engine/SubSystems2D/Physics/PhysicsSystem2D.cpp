#include "PhysicsSystem2D.h"

#include "Core/TemplateUtils/TemplateUtils.h"

#include "Components2D/Script/Script2D.h"

#include <iostream>

NAMESPACE_BEGIN

PhysicsSystem2D::PhysicsSystem2D(Scene2D* scene) : SubSystem2D(scene, Script2D::COMPONENT_ID)
{
	m_querySession = scene->NewQuerySession();
}

PhysicsSystem2D::~PhysicsSystem2D()
{
	m_scene->DeleteQuerySession(m_querySession);
}

void PhysicsSystem2D::PrevIteration(float dt)
{
}

void PhysicsSystem2D::Iteration(float dt)
{
	
}

void PhysicsSystem2D::PostIteration(float dt)
{
}

void PhysicsSystem2D::AddSubSystemComponent(SubSystemComponent2D* comp)
{
	
}

void PhysicsSystem2D::RemoveSubSystemComponent(SubSystemComponent2D* comp)
{
	
}

NAMESPACE_END
