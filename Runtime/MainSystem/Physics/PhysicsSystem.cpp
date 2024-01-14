#include "PhysicsSystem.h"

#include "PhysX/PhysX.h"

#include "Components/PhysicsComponent.h"
#include "Components/RigidBodyDynamic.h"

#include "Scene/GameObject.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsSystem::PhysicsSystem(Scene* scene) : MainSystem(scene)
{
	auto physics = PhysX::Get()->GetPxPhysics();
	PxSceneDesc sceneDesc(physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = PhysX::Get()->GetCpuDispatcher();
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;

	m_pxScene = physics->createScene(sceneDesc);

	m_pxControllerManager = PxCreateControllerManager(*m_pxScene);
}

PhysicsSystem::~PhysicsSystem()
{
	m_pxControllerManager->release();
	m_pxScene->release();
}

void PhysicsSystem::BeginModification()
{
}

void PhysicsSystem::AddComponent(MainComponent* comp)
{
	auto physics = (PhysicsComponent*)comp;
	if (physics->m_pxActor)
		m_pxScene->addActor(*physics->m_pxActor);
}

void PhysicsSystem::RemoveComponent(MainComponent* comp)
{
	auto physics = (PhysicsComponent*)comp;
	if (physics->m_pxActor)
		m_pxScene->removeActor(*physics->m_pxActor);
}

void PhysicsSystem::OnObjectTransformChanged(MainComponent* comp)
{
}

void PhysicsSystem::EndModification()
{
}

void PhysicsSystem::PrevIteration()
{
}

void PhysicsSystem::Iteration(float dt)
{
	GetPrevAsyncTaskRunnerMT()->ProcessAllTasksMT(this);
	GetPrevAsyncTaskRunnerST()->ProcessAllTasks(this);
	GetPrevAsyncTaskRunner()->ProcessAllTasks(this);

	if (dt <= 0)
	{
		dt = 1.0f / 120.0f;
	}

	m_pxScene->simulate(dt);

	// lose 1 thread T___T
	m_pxScene->fetchResults(true);

	{
		uint32_t activeCount = 0;
		auto actors = m_pxScene->getActiveActors(activeCount);
		for (uint32_t i = 0; i < activeCount; i++)
		{
			auto actor = actors[i];
			auto comp = (PhysicsComponent*)actor->userData;

			if (comp)
			{
				comp->OnPhysicsTransformChanged();
			}

		}
	}
}

void PhysicsSystem::PostIteration()
{
}

NAMESPACE_END