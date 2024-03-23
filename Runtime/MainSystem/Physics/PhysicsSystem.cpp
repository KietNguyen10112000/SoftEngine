#include "PhysicsSystem.h"

#include "PhysX/PhysX.h"

#include "Components/PhysicsComponent.h"
#include "Components/RigidBodyDynamic.h"
#include "Components/CharacterController.h"

#include "Shapes/PhysicsShape.h"

#include "Scene/GameObject.h"

#include "FILTER_DATA.h"

using namespace physx;

NAMESPACE_BEGIN

static PxFilterFlags PhysicsContactReportFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0,
	PxFilterObjectAttributes attributes1, PxFilterData filterData1,
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	PX_UNUSED(attributes0);
	PX_UNUSED(attributes1);
	//PX_UNUSED(filterData0);
	//PX_UNUSED(filterData1);
	PX_UNUSED(constantBlockSize);
	PX_UNUSED(constantBlock);

	auto filterDataWord0 = filterData0.word0 != 0 ? filterData0.word0 : filterData1.word0;
	switch (filterDataWord0)
	{
	case PHYSICS_FILTER_DATA_CCT:
		pairFlags =  PxPairFlag::eDETECT_DISCRETE_CONTACT
			| PxPairFlag::eNOTIFY_TOUCH_FOUND
			| PxPairFlag::eNOTIFY_TOUCH_LOST
			| PxPairFlag::eNOTIFY_TOUCH_PERSISTS
			| PxPairFlag::eNOTIFY_CONTACT_POINTS;
		break;
	default:
		pairFlags = PxPairFlag::eSOLVE_CONTACT
			| PxPairFlag::eDETECT_DISCRETE_CONTACT
			| PxPairFlag::eNOTIFY_TOUCH_FOUND
			| PxPairFlag::eNOTIFY_TOUCH_LOST
			| PxPairFlag::eNOTIFY_TOUCH_PERSISTS
			| PxPairFlag::eNOTIFY_CONTACT_POINTS;
		break;
	}

	return PxFilterFlag::eDEFAULT;
}

class PhysXSimulationCallback : public PxSimulationEventCallback
{
	PhysicsSystem* m_system = nullptr;

	std::vector<PxContactPairPoint> m_contactPairPoints;

public:
	PhysXSimulationCallback(PhysicsSystem* system) : m_system(system) {};

	void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) { PX_UNUSED(constraints); PX_UNUSED(count); }

	void onWake(PxActor** actors, PxU32 count) { PX_UNUSED(actors); PX_UNUSED(count); }
	void onSleep(PxActor** actors, PxU32 count) { PX_UNUSED(actors); PX_UNUSED(count); }

	void onTrigger(PxTriggerPair* pairs, PxU32 count) { PX_UNUSED(pairs); PX_UNUSED(count); }
	void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) {}

	void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
	{
		auto& activeComponentsHasContact = m_system->m_activeComponentsHasContact;
		for (auto a : pairHeader.actors)
		{
			auto comp = (PhysicsComponent*)a->userData;
			if (comp && comp->HasPhysicsFlag(PHYSICS_FLAG_ENABLE_COLLISION) 
				&& comp->m_collisionResult->lastActiveIterationCount != m_system->GetScene()->GetIterationCount())
			{
				comp->m_collisionResult->lastActiveIterationCount = m_system->GetScene()->GetIterationCount();
				activeComponentsHasContact.push_back(comp);
			}
		}

		auto AActor = (PxRigidActor*)pairHeader.actors[0];
		auto BActor = (PxRigidActor*)pairHeader.actors[1];

		auto A = (PhysicsComponent*)AActor->userData;
		auto B = (PhysicsComponent*)BActor->userData;

		auto AContacts = (A && A->m_collisionResult) ? &A->m_collisionResult->collision.ForceWrite()->contacts : nullptr;
		auto BContacts = (B && B->m_collisionResult) ? &B->m_collisionResult->collision.ForceWrite()->contacts : nullptr;

		if (!AContacts && !BContacts)
		{
			return;
		}

		SharedPtr<CollisionContact> contact = std::make_shared<CollisionContact>();
		contact->A = A->GetGameObject();
		contact->B = B->GetGameObject();

		if (AContacts)
		{
			AContacts->push_back(contact);
		}

		if (BContacts)
		{
			BContacts->push_back(contact);
		}

		for (PxU32 i = 0; i < nbPairs; i++)
		{
			const PxContactPair& cp = pairs[i];

			auto AShape = (PhysicsShape*)cp.shapes[0]->userData;
			auto BShape = (PhysicsShape*)cp.shapes[1]->userData;

			if (cp.events.isSet(PxPairFlag::eNOTIFY_TOUCH_LOST))
			{
				contact->endContactPairs.push_back({ AShape, BShape });
				continue;
			}
			else if (cp.events.isSet(PxPairFlag::eNOTIFY_TOUCH_FOUND))
			{
				contact->beginContactPairsIds.push_back(contact->contactPairs.size());
			} 
			else if (cp.events.isSet(PxPairFlag::eNOTIFY_TOUCH_PERSISTS))
			{
				contact->modifiedContactPairsIds.push_back(contact->contactPairs.size());
			}

			auto contactPair = std::make_shared<CollisionContactPair>();
			contact->contactPairs.push_back(contactPair);
			contactPair->AShape = AShape;
			contactPair->BShape = BShape;

			PxContactStreamIterator iter(cp.contactPatches, cp.contactPoints, cp.getInternalFaceIndices(), cp.patchCount, cp.contactCount);

			const PxReal* impulses = cp.contactImpulses;

			PxU32 flippedContacts = (cp.flags & PxContactPairFlag::eINTERNAL_CONTACTS_ARE_FLIPPED);
			PxU32 hasImpulses = (cp.flags & PxContactPairFlag::eINTERNAL_HAS_IMPULSES);
			PxU32 nbContacts = 0;

			while (iter.hasNextPatch())
			{
				iter.nextPatch();
				while (iter.hasNextContact())
				{
					iter.nextContact();

					CollisionContactPoint dst = {};
					dst.position = reinterpret_cast<const Vec3&>(iter.getContactPoint());
					dst.normal = reinterpret_cast<const Vec3&>(iter.getContactNormal());
					dst.impulse = hasImpulses ? dst.normal * impulses[nbContacts] : Vec3::ZERO;
					dst.staticFriction = iter.getStaticFriction();
					dst.dynamicFriction = iter.getDynamicFriction();

					contactPair->contactPoints.push_back(dst);

					/*PxU32 internalFaceIndex0 = flippedContacts ?
						iter.getFaceIndex1() : iter.getFaceIndex0();
					PxU32 internalFaceIndex1 = flippedContacts ?
						iter.getFaceIndex0() : iter.getFaceIndex1();*/
					//...
					nbContacts++;
				}
			}
		}

	}
};

//class PhysXSimulationCallback_ : public PxContactModifyCallback
//{
//
//};

PhysicsSystem::PhysicsSystem(Scene* scene) : MainSystem(scene)
{
	InitializeAsyncTaskRunnerForMainComponent(m_asyncTaskRunner);

	auto callback = new (&m_physxSimulationCallback) PhysXSimulationCallback(this);

	auto physics = PhysX::Get()->GetPxPhysics();
	PxSceneDesc sceneDesc(physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = PhysX::Get()->GetCpuDispatcher();
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
	sceneDesc.userData = this;
	sceneDesc.simulationEventCallback = callback;
	sceneDesc.filterShader = PhysicsContactReportFilterShader;
	sceneDesc.kineKineFilteringMode = PxPairFilteringMode::eKEEP;
	sceneDesc.staticKineFilteringMode = PxPairFilteringMode::eKEEP;
	//sceneDesc.contactModifyCallback = 

	m_pxScene = physics->createScene(sceneDesc);

	m_pxControllerManager = PxCreateControllerManager(*m_pxScene);

	m_gravity = reinterpret_cast<const Vec3&>(sceneDesc.gravity);
}

PhysicsSystem::~PhysicsSystem()
{
	((PhysXSimulationCallback*)(&m_physxSimulationCallback))->~PhysXSimulationCallback();

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

void PhysicsSystem::ProcessCollisionList()
{
	// collect sleeping collision pairs from previous frame

	for (auto& comp : m_activeComponentsHasContact)
	{
		auto obj = comp->GetGameObject();
		auto prevCollision = comp->m_collisionResult->collision.Read();
		auto curCollision = comp->m_collisionResult->collision.ForceWrite();

		auto& prevContacts = prevCollision->contacts;
		auto& curContacts = curCollision->contacts;

		// calculate count
		{
			size_t count1 = 0;
			size_t count2 = 0;
			for (auto& contact : curContacts)
			{
				count1 += contact->beginContactPairsIds.size();
				count2 += contact->endContactPairs.size();
			}

			curCollision->beginContactPairsCount = count1;
			curCollision->endContactPairsCount = count2;
		}

		// refering back curent contact with previous contact
		{
			uint32_t idx = 0;
			for (auto& contact : curContacts)
			{
				auto B = contact->A == obj ? contact->B : contact->A;
				B->GetComponentRaw<PhysicsComponent>()->m_refContactIdx[0] = idx;
				idx++;
			}

			for (auto& contact : prevContacts)
			{
				auto B = contact->A == obj ? contact->B : contact->A;
				auto& refIdx = B->GetComponentRaw<PhysicsComponent>()->m_refContactIdx[0];

				//contact->newCollisionContactIdx = INVALID_ID;
				contact->oldCollisionContact = (CollisionContact*)(INVALID_ID - 1);
				if (refIdx != (uint32_t)INVALID_ID)
				{
					curContacts[refIdx]->oldCollisionContact = contact.get();
					//contact->newCollisionContactIdx = refIdx;

					//refIdx = (uint32_t)INVALID_ID;
				}
			}
		}

		// process contact pairs
		{
			for (auto& contact : curContacts)
			{
				auto oldContact = contact->oldCollisionContact;
				if (oldContact == (void*)INVALID_ID)
				{
					continue;
				}

				auto BIdx = contact->A == obj ? 1 : 0;

				for (auto& pair : contact->contactPairs)
				{
					auto& BShape = pair->shapes[BIdx];
					BShape->m_inFrameType[0] = 1;
				}

				for (auto& pair : contact->endContactPairs)
				{
					auto& BShape = pair.shapes[BIdx];
					BShape->m_inFrameType[0] = 2;
				}

				uint32_t idx = 0;
				auto oldBIdx = oldContact->A == obj ? 1 : 0;
				for (auto& pair : oldContact->contactPairs)
				{
					auto& BShape = pair->shapes[oldBIdx];
					auto& inFrameType = BShape->m_inFrameType[0];

					// this pair is still in touch and nothing changed
					if (inFrameType == 0)
					{
						contact->contactPairs.push_back(pair);
					}
					else if (inFrameType == 2)
					{
						contact->endContactPairsIds.push_back(idx);
					}

					idx++;
				}

				for (auto& pair : contact->contactPairs)
				{
					auto& BShape = pair->shapes[BIdx];
					BShape->m_inFrameType[0] = 0;
				}

				for (auto& pair : contact->endContactPairs)
				{
					auto& BShape = pair.shapes[BIdx];
					BShape->m_inFrameType[0] = 0;
				}
			}
		}

		// process contact
		{
			for (auto& contact : prevContacts)
			{
				auto B = contact->A == obj ? contact->B : contact->A;
				auto& refIdx = B->GetComponentRaw<PhysicsComponent>()->m_refContactIdx[0];

				if (refIdx == (uint32_t)INVALID_ID)
				{
					curContacts.push_back(contact);
				}
			}

			// reset m_refContactIdx
			for (auto& contact : curContacts)
			{
				auto B = contact->A == obj ? contact->B : contact->A;
				B->GetComponentRaw<PhysicsComponent>()->m_refContactIdx[0] = (uint32_t)INVALID_ID;
			}
		}

		// remove all lost contacts
		{
			for (size_t i = 0; i < curContacts.size(); i++)
			{
				auto& contact = curContacts[i];
				if (contact->oldCollisionContact == (void*)INVALID_ID || contact->oldCollisionContact == (void*)(INVALID_ID - 1))
				{
					continue;
				}

				if (contact->endContactPairs.size() == contact->oldCollisionContact->contactPairs.size())
				{
					curCollision->endContacts.push_back(contact->oldCollisionContact);
					STD_VECTOR_ROLL_TO_FILL_BLANK_2(curContacts, i);
					i--;
				}
			}
		}

		// calculate count
		{
			size_t count1 = 0;
			size_t count2 = 0;
			for (auto& contact : curContacts)
			{
				for (auto& pair : contact->contactPairs)
				{
					count2 += pair->contactPoints.size();
				}

				count1 += contact->contactPairs.size();
			}

			curCollision->contactPairsCount = count1;
			curCollision->contactPointsCount = count2;
		}
	}

	// update for other subsystems
	for (auto comp : m_activeComponentsHasContact)
	{
		m_scene->BeginWrite<false>(comp->m_collisionResult->collision);
		m_scene->EndWrite(comp->m_collisionResult->collision);
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
	for (auto comp : m_activeComponentsHasContact)
	{
		comp->m_collisionResult->collision.ForceWrite()->Clear();
	}
	m_activeComponentsHasContact.clear();

	GetPrevAsyncTaskRunnerMT()->ProcessAllTasksMT(this);
	GetPrevAsyncTaskRunnerST()->ProcessAllTasks(this);
	GetPrevAsyncTaskRunner()->ProcessAllTasks(this);

	if (dt <= 0 || dt >= 0.1f)
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

	ProcessCollisionList();

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