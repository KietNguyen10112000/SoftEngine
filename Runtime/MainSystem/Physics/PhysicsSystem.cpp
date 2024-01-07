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
}

PhysicsSystem::~PhysicsSystem()
{
}

void PhysicsSystem::BeginModification()
{
}

void PhysicsSystem::AddComponent(MainComponent* comp)
{
	auto physics = (PhysicsComponent*)comp;
	m_pxScene->addActor(*physics->m_pxActor);
}

void PhysicsSystem::RemoveComponent(MainComponent* comp)
{
	auto physics = (PhysicsComponent*)comp;
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
	if (dt <= 0)
	{
		//std::cout << "Fuckkkkkkkkkkkkkkkkkk\n";
		dt = 1.0f / 120.0f;
	}

	m_pxScene->simulate(dt);

	// lose 1 thread T___T
	m_pxScene->fetchResults(true);

	uint32_t activeCount = 0;
	auto actors = m_pxScene->getActiveActors(activeCount);
	for (uint32_t i = 0; i < activeCount; i++)
	{
		auto actor = actors[i];

		{
			auto dynamicRigid = actor->is<PxRigidDynamic>();
			if (dynamicRigid)
			{
				auto comp = (RigidBodyDynamic*)dynamicRigid->userData;
				auto obj = comp->GetGameObject();
				obj->ContributeTransform(comp, RigidBodyDynamic::TransformContributor);
				m_scene->OnObjectTransformChanged(obj);
			}
		}

	}
}

void PhysicsSystem::PostIteration()
{
}

NAMESPACE_END