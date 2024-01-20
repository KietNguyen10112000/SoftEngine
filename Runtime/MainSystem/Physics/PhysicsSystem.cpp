#include "PhysicsSystem.h"

#include "PhysX/PhysX.h"

#include "Components/PhysicsComponent.h"
#include "Components/RigidBodyDynamic.h"

#include "Scene/GameObject.h"

using namespace physx;

NAMESPACE_BEGIN

static PxFilterFlags PhysicsContactReportFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0,
	PxFilterObjectAttributes attributes1, PxFilterData filterData1,
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	PX_UNUSED(attributes0);
	PX_UNUSED(attributes1);
	PX_UNUSED(filterData0);
	PX_UNUSED(filterData1);
	PX_UNUSED(constantBlockSize);
	PX_UNUSED(constantBlock);

	// all initial and persisting reports for everything, with per-point data
	pairFlags = PxPairFlag::eSOLVE_CONTACT | PxPairFlag::eDETECT_DISCRETE_CONTACT
		| PxPairFlag::eNOTIFY_TOUCH_FOUND
		| PxPairFlag::eNOTIFY_TOUCH_PERSISTS
		| PxPairFlag::eNOTIFY_CONTACT_POINTS;

	return PxFilterFlag::eDEFAULT;
}

class PhysXSimulationCallback : public PxSimulationEventCallback
{
	void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) { PX_UNUSED(constraints); PX_UNUSED(count); }

	void onWake(PxActor** actors, PxU32 count) { PX_UNUSED(actors); PX_UNUSED(count); }
	void onSleep(PxActor** actors, PxU32 count) { PX_UNUSED(actors); PX_UNUSED(count); }

	void onTrigger(PxTriggerPair* pairs, PxU32 count) { PX_UNUSED(pairs); PX_UNUSED(count); }
	void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) {}

	void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
	{
		/*PX_UNUSED((pairHeader));
		std::vector<PxContactPairPoint> contactPoints;

		for (PxU32 i = 0; i < nbPairs; i++)
		{
			PxU32 contactCount = pairs[i].contactCount;
			if (contactCount)
			{
				contactPoints.resize(contactCount);
				pairs[i].extractContacts(&contactPoints[0], contactCount);

				for (PxU32 j = 0; j < contactCount; j++)
				{
					gContactPositions.push_back(contactPoints[j].position);
					gContactImpulses.push_back(contactPoints[j].impulse);
				}
			}
		}*/

		std::cout << "collision\n";
	}
};

PhysXSimulationCallback g_PhysXSimulationCallback;

PhysicsSystem::PhysicsSystem(Scene* scene) : MainSystem(scene)
{
	auto physics = PhysX::Get()->GetPxPhysics();
	PxSceneDesc sceneDesc(physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = PhysX::Get()->GetCpuDispatcher();
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
	sceneDesc.userData = this;
	sceneDesc.simulationEventCallback = &g_PhysXSimulationCallback;
	sceneDesc.filterShader = PhysicsContactReportFilterShader;

	m_pxScene = physics->createScene(sceneDesc);

	m_pxControllerManager = PxCreateControllerManager(*m_pxScene);

	m_gravity = reinterpret_cast<const Vec3&>(sceneDesc.gravity);
}

PhysicsSystem::~PhysicsSystem()
{
	m_pxControllerManager->release();
	m_pxScene->release();
}

void PhysicsSystem::ScheduleUpdateImpl(PhysicsComponent* comp)
{
	if (comp->UpdateId() != INVALID_ID)
	{
		return;
	}

	comp->UpdateId() = m_updateList.size();
	comp->IsUpdateIdRemoved() = false;
	m_updateList.push_back(comp);
}

void PhysicsSystem::UnscheduleUpdateImpl(PhysicsComponent* comp)
{
	STD_VECTOR_ROLL_TO_FILL_BLANK(m_updateList, comp, UpdateId());
	comp->UpdateId() = INVALID_ID;
}

void PhysicsSystem::SchedulePostUpdateImpl(PhysicsComponent* comp)
{
	if (comp->PostUpdateId() != INVALID_ID)
	{
		return;
	}

	comp->PostUpdateId() = m_postUpdateList.size();
	comp->IsPostUpdateIdRemoved() = false;
	m_postUpdateList.push_back(comp);
}

void PhysicsSystem::UnschedulePostUpdateImpl(PhysicsComponent* comp)
{
	STD_VECTOR_ROLL_TO_FILL_BLANK(m_postUpdateList, comp, PostUpdateId());
	comp->PostUpdateId() = INVALID_ID;
}

void PhysicsSystem::UnscheduleUpdate(PhysicsComponent* comp)
{
	m_updateListLock.lock();

	if (!comp->IsUpdateIdRemoved())
	{
		m_removeUpdateList.push_back(comp);
		comp->IsUpdateIdRemoved() = true;
	}

	m_updateListLock.unlock();
}

void PhysicsSystem::UnschedulePostUpdate(PhysicsComponent* comp)
{
	m_postUpdateListLock.lock();

	if (!comp->IsPostUpdateIdRemoved())
	{
		m_removePostUpdateList.push_back(comp);
		comp->IsPostUpdateIdRemoved() = true;
	}

	m_postUpdateListLock.unlock();
}

void PhysicsSystem::RebuildUpdateList()
{
	for (auto& comp : m_removeUpdateList)
	{
		UnscheduleUpdateImpl(comp);
	}
	m_removeUpdateList.clear();

	for (auto& comp : m_removePostUpdateList)
	{
		UnschedulePostUpdateImpl(comp);
	}
	m_removePostUpdateList.clear();
}

void PhysicsSystem::ProcessUpdateList()
{
	auto size = m_updateList.size();
	for (size_t i = 0; i < size; i++)
	{
		auto comp = m_updateList[i];
		if (!comp->IsUpdateIdRemoved())
			comp->OnUpdate(m_dt);
	}
}

void PhysicsSystem::ProcessPostUpdateList()
{
	auto size = m_postUpdateList.size();
	for (size_t i = 0; i < size; i++)
	{
		auto comp = m_postUpdateList[i];
		if (!comp->IsPostUpdateIdRemoved())
			comp->OnPostUpdate(m_dt);
	}
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

	if (physics->UpdateId() != INVALID_ID && !physics->IsUpdateIdRemoved())
	{
		UnscheduleUpdateImpl(physics);
	}

	if (physics->PostUpdateId() != INVALID_ID && !physics->IsPostUpdateIdRemoved())
	{
		UnschedulePostUpdateImpl(physics);
	}

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

	m_dt = dt;

	RebuildUpdateList();
	ProcessUpdateList();

	// lose 1 thread T___T
	m_pxScene->fetchResults(true);

	RebuildUpdateList();
	ProcessPostUpdateList();
	RebuildUpdateList();

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