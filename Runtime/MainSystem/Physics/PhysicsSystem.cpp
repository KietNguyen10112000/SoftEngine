#include "PhysicsSystem.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "Components/PhysicsComponent.h"
#include "Components/RigidBodyDynamic.h"
#include "Components/CharacterController.h"

#include "MainSystem/Animation/AnimationSystem.h"

#include "Joints/Joint.h"

#include "Shapes/PhysicsShape.h"

#include "Scene/GameObject.h"

#include "FILTER_FLAG.h"

#include "Query/ActionPhysicsSweep.h"
#include "Query//ActionPhysicsOverlap.h"

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

	pairFlags = PxPairFlag::eSOLVE_CONTACT
		| PxPairFlag::eDETECT_DISCRETE_CONTACT
		| PxPairFlag::eNOTIFY_TOUCH_FOUND
		| PxPairFlag::eNOTIFY_TOUCH_LOST
		| PxPairFlag::eNOTIFY_TOUCH_PERSISTS
		| PxPairFlag::eNOTIFY_CONTACT_POINTS;

	PxFilterFlags ret = PxFilterFlag::eDEFAULT;

	if ((filterData0.word0 & PHYSICS_FILTER_FLAG::FAMILY_NO_COLLIDE) && (filterData1.word0 & PHYSICS_FILTER_FLAG::FAMILY_NO_COLLIDE))
	{
		ret |= PxFilterFlag::eCALLBACK;
	}

	auto filterDataWord0 = filterData0.word0 | filterData1.word0;

	if (PxFilterObjectIsKinematic(attributes0) && PxFilterObjectIsKinematic(attributes1))
	{
		pairFlags &= ~PxPairFlag::eSOLVE_CONTACT;
	}

	if ((filterData0.word1 & filterData1.word1) == 0)
	{
		//pairFlags.clear(PxPairFlag::eSOLVE_CONTACT);
		ret |= PxFilterFlag::eSUPPRESS;
	}

	return ret
		| ((filterDataWord0 & PHYSICS_FILTER_FLAG::CALLBACK) ? PxFilterFlag::eCALLBACK : PxFilterFlag::eDEFAULT);
}

class PhysXSimulationCallback : public PxSimulationEventCallback
{
	PhysicsSystem* m_system = nullptr;

	std::vector<PxContactPairPoint> m_contactPairPoints;

public:
	PhysXSimulationCallback(PhysicsSystem* system) : m_system(system) {};

	void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) 
	{
		auto& brokenJoints = m_system->m_brokenJoints;
		for (PxU32 i = 0; i < count; i++)
		{
			if (PxConstraintExtIDs::eJOINT == constraints[i].type)
			{
				PxJoint* pxJoint = reinterpret_cast<PxJoint*>(constraints[i].externalReference);
				Joint* joint = (Joint*)pxJoint->userData;
				brokenJoints.push_back(joint);
				//joint->RemoveJointFromBodies();
			}
		}
	}

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
			if (comp && comp->m_collisionResult
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
		contact->A = A->m_lastGameObject;
		contact->B = B->m_lastGameObject;

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

class PhysXSimulationFilterCallback : public PxSimulationFilterCallback
{
	// Inherited via PxSimulationFilterCallback
	PxFilterFlags pairFound(PxU64 pairID, 
		PxFilterObjectAttributes attributes0, 
		PxFilterData filterData0, 
		const PxActor* a0, const PxShape* s0, 
		PxFilterObjectAttributes attributes1, 
		PxFilterData filterData1, 
		const PxActor* a1, const PxShape* s1, 
		PxPairFlags& pairFlags) override
	{
		auto AComp = (PhysicsComponent*)a0->userData;
		auto BComp = (PhysicsComponent*)a1->userData;

		auto A = AComp->m_lastGameObject;
		auto B = BComp->m_lastGameObject;

		auto AShape = (PhysicsShape*)s0->userData;
		auto BShape = (PhysicsShape*)s1->userData;

		auto AType = AComp->GetPhysicsType();
		auto BType = BComp->GetPhysicsType();

		size_t myPairFlags = 0;

		if (
			(AType == PHYSICS_TYPE_RIGID_BODY_STATIC || AType == PHYSICS_TYPE_RIGID_BODY_DYNAMIC)
			&& (BType == PHYSICS_TYPE_RIGID_BODY_STATIC || BType == PHYSICS_TYPE_RIGID_BODY_DYNAMIC)
		) {
			// rigid vs rigid

			auto ABody = (RigidBody*)AComp;
			auto BBody = (RigidBody*)BComp;

			if (ABody->m_contactFilterCallback)
			{
				ABody->m_contactFilterCallback(A, AShape, AType, B, BShape, BType, *((size_t*)&myPairFlags));
			}

			if (BBody->m_contactFilterCallback)
			{
				BBody->m_contactFilterCallback(B, BShape, BType, A, AShape, AType, *((size_t*)&myPairFlags));
			}
		}

		if (AType == PHYSICS_TYPE_CHARACTER_CONTROLLER)
		{
			auto A_CCT = (CharacterController*)AComp;
			A_CCT->m_contactFilterCallback(A, AShape, AType, B, BShape, BType, *((size_t*)&myPairFlags));
		}

		if (BType == PHYSICS_TYPE_CHARACTER_CONTROLLER)
		{
			auto B_CCT = (CharacterController*)BComp;
			B_CCT->m_contactFilterCallback(B, BShape, BType, A, AShape, AType, *((size_t*)&myPairFlags));
		}

		pairFlags = (PxPairFlags)myPairFlags;

		if (myPairFlags == 0)
		{
			return PxFilterFlag::eSUPPRESS;
		}

		if ((filterData0.word0 & PHYSICS_FILTER_FLAG::FAMILY_NO_COLLIDE) && (filterData1.word0 & PHYSICS_FILTER_FLAG::FAMILY_NO_COLLIDE))
		{
			if (AComp->GetGameObject()->GetCommittedRoot() == BComp->GetGameObject()->GetCommittedRoot())
			{
				//pairFlags.clear(PxPairFlag::eSOLVE_CONTACT);
				//pairFlags &= ~PxPairFlag::eSOLVE_CONTACT;
				return PxFilterFlag::eSUPPRESS;
			}
			else if (uint32_t(pairFlags) == 0)
			{
				return PhysicsContactReportFilterShader(attributes0, filterData0, attributes1, filterData1, pairFlags, 0, 0);
			}
		}

		if (PxFilterObjectIsKinematic(attributes0) && PxFilterObjectIsKinematic(attributes1)) 
		{ 
			pairFlags &= ~PxPairFlag::eSOLVE_CONTACT; 
		}

		return PxFilterFlag::eDEFAULT;
	}
	void pairLost(PxU64 pairID, PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, bool objectRemoved) override
	{
	}
	bool statusChange(PxU64& pairID, PxPairFlags& pairFlags, PxFilterFlags& filterFlags) override
	{
		return false;
	}
};

void PhysicsSystem::PhysicsSystemDependenciesResolver::Resolve(GameObjectDependenciesRecorder* recorder, GameObject* input)
{
	/*if (!input->HasComponent(PhysicsComponent::COMPONENT_ID))
	{
		return;
	}*/

	auto comp = (PhysicsComponent*)input->m_mainComponents[PhysicsComponent::COMPONENT_ID].Get();
	
	auto type = comp->GetPhysicsType();
	switch (type)
	{
	case soft::PHYSICS_TYPE_RIGID_BODY_STATIC:
	case soft::PHYSICS_TYPE_RIGID_BODY_DYNAMIC:
	case soft::PHYSICS_TYPE_CHARACTER_CONTROLLER:
	{
		auto rigidBody = (RigidBody*)comp;
		for (auto& joint : rigidBody->m_joints)
		{
			if (joint->IsBroken())
			{
				continue;
			}

			auto another = joint->m_body0.Get() == rigidBody ? joint->m_body1.Get() : joint->m_body0.Get();
			if (another->m_object == nullptr)
			{
				int x = 3;
			}
			recorder->Record(another->m_object);
		}
		break;
	}
	default:
		break;
	}
}

PhysicsSystem::PhysicsSystem(Scene* scene) : MainSystem(scene)
{
	InitializeAsyncTaskRunnerForMainComponent(m_asyncTaskRunner);

	auto callback = new (&m_physxSimulationCallback) PhysXSimulationCallback(this);
	auto filterCallback = new (&m_physXSimulationFilterCallback) PhysXSimulationFilterCallback();

	auto physics = PhysX::Get()->GetPxPhysics();
	PxSceneDesc sceneDesc(physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = PhysX::Get()->GetCpuDispatcher();
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
	sceneDesc.userData = this;
	sceneDesc.filterShader = PhysicsContactReportFilterShader;
	sceneDesc.kineKineFilteringMode = PxPairFilteringMode::eKEEP;
	sceneDesc.staticKineFilteringMode = PxPairFilteringMode::eKEEP;

	sceneDesc.simulationEventCallback = callback;
	sceneDesc.filterCallback = filterCallback;
	//sceneDesc.contactModifyCallback = 

	m_pxScene = physics->createScene(sceneDesc);

	m_pxControllerManager = PxCreateControllerManager(*m_pxScene);

	m_gravity = reinterpret_cast<const Vec3&>(sceneDesc.gravity);
}

PhysicsSystem::~PhysicsSystem()
{
	int x = 3;
}

void PhysicsSystem::Finalize()
{
	((PhysXSimulationCallback*)(&m_physxSimulationCallback))->~PhysXSimulationCallback();
	((PhysXSimulationFilterCallback*)(&m_physXSimulationFilterCallback))->~PhysXSimulationFilterCallback();

	auto count = m_pxControllerManager->getNbControllers();
	for (size_t i = 0; i < count; i++)
	{
		auto pxCct = m_pxControllerManager->getController(i);
		auto cct = (CharacterController*)pxCct->getUserData();
		if (cct)
		{
			cct->m_pxCharacterController = nullptr;
		}
	}

	m_pxControllerManager->release();
	m_pxScene->release();
}

void PhysicsSystem::SchedulePrevUpdateImpl(PhysicsComponent* comp)
{
	if (comp->PrevUpdateId() != uint32_t(INVALID_ID))
	{
		return;
	}

	comp->PrevUpdateId() = m_prevUpdateList.size();
	comp->IsPrevUpdateIdRemoved() = false;
	m_prevUpdateList.push_back(comp);
}

void PhysicsSystem::UnschedulePrevUpdateImpl(PhysicsComponent* comp)
{
	STD_VECTOR_ROLL_TO_FILL_BLANK(m_prevUpdateList, comp, PrevUpdateId());
	comp->PrevUpdateId() = INVALID_ID;
}

void PhysicsSystem::ScheduleUpdateImpl(PhysicsComponent* comp)
{
	if (comp->UpdateId() != uint32_t(INVALID_ID))
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

//void PhysicsSystem::SchedulePostUpdateImpl(PhysicsComponent* comp)
//{
//	if (comp->PostUpdateId() != INVALID_ID)
//	{
//		return;
//	}
//
//	comp->PostUpdateId() = m_postUpdateList.size();
//	comp->IsPostUpdateIdRemoved() = false;
//	m_postUpdateList.push_back(comp);
//}
//
//void PhysicsSystem::UnschedulePostUpdateImpl(PhysicsComponent* comp)
//{
//	STD_VECTOR_ROLL_TO_FILL_BLANK(m_postUpdateList, comp, PostUpdateId());
//	comp->PostUpdateId() = INVALID_ID;
//}

void PhysicsSystem::UnschedulePrevUpdate(PhysicsComponent* comp)
{
	m_prevUpdateListLock.lock();

	if (!comp->IsPrevUpdateIdRemoved())
	{
		m_removePrevUpdateList.push_back(comp);
		comp->IsPrevUpdateIdRemoved() = true;
	}

	m_prevUpdateListLock.unlock();
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

//void PhysicsSystem::UnschedulePostUpdate(PhysicsComponent* comp)
//{
//	m_postUpdateListLock.lock();
//
//	if (!comp->IsPostUpdateIdRemoved())
//	{
//		m_removePostUpdateList.push_back(comp);
//		comp->IsPostUpdateIdRemoved() = true;
//	}
//
//	m_postUpdateListLock.unlock();
//}

void PhysicsSystem::RebuildUpdateList()
{
	for (auto& comp : m_removePrevUpdateList)
	{
		UnschedulePrevUpdateImpl(comp);
	}
	m_removePrevUpdateList.clear();

	for (auto& comp : m_removeUpdateList)
	{
		UnscheduleUpdateImpl(comp);
	}
	m_removeUpdateList.clear();

	/*for (auto& comp : m_removePostUpdateList)
	{
		UnschedulePostUpdateImpl(comp);
	}
	m_removePostUpdateList.clear();*/
}

void PhysicsSystem::ProcessPrevUpdateList()
{
	auto size = m_prevUpdateList.size();
	for (size_t i = 0; i < size; i++)
	{
		auto comp = m_prevUpdateList[i];
		if (!comp->IsPrevUpdateIdRemoved())
			comp->OnPrevUpdate(m_dt);
	}
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

//void PhysicsSystem::ProcessPostUpdateList()
//{
//	auto size = m_postUpdateList.size();
//	for (size_t i = 0; i < size; i++)
//	{
//		auto comp = m_postUpdateList[i];
//		if (!comp->IsPostUpdateIdRemoved())
//			comp->OnPostUpdate(m_dt);
//	}
//}

void PhysicsSystem::ProcessCollisionList()
{
	// collect sleeping collision pairs from previous frame

	for (auto& comp : m_activeComponentsHasContact)
	{
		auto obj = comp->m_lastGameObject;
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

void PhysicsSystem::FlushAsyncTasks()
{
	GetPrevAsyncTaskRunnerMT()->ProcessAllTasksMT(this);
	GetPrevAsyncTaskRunnerST()->ProcessAllTasks(this);
	GetPrevAsyncTaskRunner()->ProcessAllTasks(this);
}

void PhysicsSystem::BeginModification()
{
}

void PhysicsSystem::AddComponent(MainComponent* comp)
{
	auto physics = (PhysicsComponent*)comp;

	physics->m_lastGameObject = comp->GetGameObject();

	if (physics->m_pxActor)
		m_pxScene->addActor(*physics->m_pxActor);
}

void PhysicsSystem::RemoveComponent(MainComponent* comp)
{
	auto physics = (PhysicsComponent*)comp;

	if (physics->PrevUpdateId() != INVALID_ID && !physics->IsPrevUpdateIdRemoved())
	{
		UnschedulePrevUpdateImpl(physics);
	}

	if (physics->UpdateId() != INVALID_ID && !physics->IsUpdateIdRemoved())
	{
		UnscheduleUpdateImpl(physics);
	}

	/*if (physics->PostUpdateId() != INVALID_ID && !physics->IsPostUpdateIdRemoved())
	{
		UnschedulePostUpdateImpl(physics);
	}*/

	if (physics->m_pxActor)
		m_pxScene->removeActor(*physics->m_pxActor);

	GetCurrentTrash().Push(comp);
}

void PhysicsSystem::OnObjectTransformChanged(MainComponent* comp)
{
}

void PhysicsSystem::EndModification()
{
}

void PhysicsSystem::PrevIteration()
{
	assert(m_serialQueriesDebugBeginEndCall == 0);
	m_isQueryAvailable = false;
}

void PhysicsSystem::Iteration(float dt)
{
	for (auto comp : m_activeComponentsHasContact)
	{
		comp->m_collisionResult->collision.ForceWrite()->Clear();
	}
	m_activeComponentsHasContact.clear();

	GetCurrentTrash().clear();

	GetPrevAsyncTaskRunnerMT()->ProcessAllTasksMT(this);
	GetPrevAsyncTaskRunnerST()->ProcessAllTasks(this);
	GetPrevAsyncTaskRunner()->ProcessAllTasks(this);

	if (dt <= 0 || dt >= 0.1f)
	{
		dt = 1.0f / 120.0f;
	}

	m_dt = dt;

	RebuildUpdateList();
	ProcessPrevUpdateList();

	{
		GetScene()->GetAnimationSystem()->PrevPhysicsSimulationUpdate();
	}

	m_pxScene->simulate(dt);

	for (auto& joint : m_brokenJoints)
	{
		joint->RemoveJointFromBodies();
	}
	m_brokenJoints.clear();

	RebuildUpdateList();
	ProcessUpdateList();

	// lose 1 thread T___T
	m_pxScene->fetchResults(true);

	m_isQueryAvailable = true;

	{
		TaskSystem::PrepareHandle(&m_otherSubsystemsCallbackWaitingHandle);

		// animation post call
		{
			Task task = {};
			task.Params() = GetScene()->GetAnimationSystem();
			task.Entry() = [](void* p)
			{
				auto animationSystem = (AnimationSystem*)p;
				animationSystem->PostPhysicsSimulationUpdate();
			};

			TaskSystem::Submit(&m_otherSubsystemsCallbackWaitingHandle, task, Task::CRITICAL);
		}

		// flush queries
		{
			Task task = {};
			task.Params() = this;
			task.Entry() = [](void* p)
			{
				auto system = (PhysicsSystem*)p;
				system->FlushAllQueries();
			};

			TaskSystem::Submit(&m_otherSubsystemsCallbackWaitingHandle, task, Task::CRITICAL);
		}

		// flush serial queries
		{
			Task task = {};
			task.Params() = this;
			task.Entry() = [](void* p)
				{
					auto system = (PhysicsSystem*)p;
					system->FlushAllSerialQueries();
				};

			TaskSystem::Submit(&m_otherSubsystemsCallbackWaitingHandle, task, Task::CRITICAL);
		}

		// flush direct queries
		{
			Task task = {};
			task.Params() = this;
			task.Entry() = [](void* p)
			{
				auto system = (PhysicsSystem*)p;
				system->FlushAllDirectQueries();
			};

			TaskSystem::Submit(&m_otherSubsystemsCallbackWaitingHandle, task, Task::CRITICAL);
		}
	}

	RebuildUpdateList();
	/*ProcessPostUpdateList();
	RebuildUpdateList();*/

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

	{
		TaskSystem::WaitForHandle(&m_otherSubsystemsCallbackWaitingHandle);
	}
}

void PhysicsSystem::PostIteration()
{
}

#define PhysicsSystem_PxSweepHit_Convert(ownHit, pxHit)							\
{																				\
ownHit.position = PhysXUtils::ToVec3(pxHit.position);							\
ownHit.normal = PhysXUtils::ToVec3(pxHit.normal);								\
ownHit.obj = ((PhysicsComponent*)pxHit.actor->userData)->GetGameObject();		\
ownHit.shape = ((PhysicsShape*)pxHit.shape->userData);							\
}

class PhysicsSystem_QueryFilterCallback : public PxQueryFilterCallback
{
public:
	PhysicsQueryFilterCallback* m_userCallback = nullptr;

	PhysicsSystem_QueryFilterCallback(PhysicsQueryFilterCallback* userCallback) : m_userCallback(userCallback)
	{

	}

	// Inherited via PxQueryFilterCallback
	PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override
	{
		auto comp = (PhysicsComponent*)actor->userData;
		PhysicsHitFlags myFlag = uint8_t(queryFlags);
		auto status = m_userCallback->PrevFilter(comp->GetGameObject(), (PhysicsShape*)shape->userData, myFlag);
		return PxQueryHitType::Enum(status);
	}

	PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit, const PxShape* shape, const PxRigidActor* actor) override
	{
		auto comp = (PhysicsComponent*)actor->userData;
		auto status = m_userCallback->PostFilter(comp->GetGameObject(), (PhysicsShape*)shape->userData, {});
		return PxQueryHitType::Enum(status);
	}
};

bool PhysicsSystem::SweepImpl(PhysicsSweepResult& output, const PhysicsShape* shape, const Transform& startTransform, const Vec3& distance, PhysicsQueryFilterCallback* filter)
{
	PhysicsSystem_QueryFilterCallback callback(filter);

	PxQueryFilterData filterData = PxQueryFilterData();

	filterData.flags = PxQueryFlag::eDYNAMIC | PxQueryFlag::eSTATIC |
		PxQueryFlag::eANY_HIT;

	if (filter)
	{
		filterData.flags |= PxQueryFlag::ePREFILTER;
	}

	const PxU32 bufferSize = 256;
	PxSweepHit hitBuffer[bufferSize];
	PxSweepBuffer hit(hitBuffer, bufferSize);

	/*struct MySweepCallback : public PxSweepCallback
	{
		PhysicsSweepResult* output = nullptr;

		MySweepCallback(PhysicsSweepResult* output) : output(output) {};

		virtual PxAgain processTouches(const PxSweepHit* buffer, PxU32 nbHits) override
		{

		}
	};

	MySweepCallback callback(&output);*/

	auto status = m_pxScene->sweep(
		shape->m_pxShape->getGeometry(),
		PhysXUtils::ToPxTransform(startTransform),
		PhysXUtils::ToPxVec3(distance.Normal()),
		distance.Length(),
		hit, PxHitFlag::eDEFAULT,
		filterData, (filter ? &callback : nullptr)
	);

	//std::cout << "Length: " << distance.Length() << "\n";

	//PxHitFlag::eASSUME_NO_INITIAL_OVERLAP

	output.hasBlock = hit.hasBlock;
	if (hit.hasBlock)
	{
		PhysicsSystem_PxSweepHit_Convert(output.block, hit.block);
	}

	output.touches.reserve(output.touches.size() + hit.nbTouches);
	for (size_t i = 0; i < hit.nbTouches; i++)
	{
		auto& ownHit = output.touches.emplace_back();
		PhysicsSystem_PxSweepHit_Convert(ownHit, hit.touches[i]);
	}

	return status;
}

bool PhysicsSystem::OverlapImpl(PhysicsOverlapResult& output, const PhysicsShape* shape, const Transform& startTransform, PhysicsQueryFilterCallback* filter)
{
	PhysicsSystem_QueryFilterCallback callback(filter);

	PxQueryFilterData filterData = PxQueryFilterData();

	filterData.flags = PxQueryFlag::eDYNAMIC | PxQueryFlag::eSTATIC |
		PxQueryFlag::eANY_HIT;

	if (filter)
	{
		filterData.flags |= PxQueryFlag::ePREFILTER;
	}

	PxOverlapBuffer hit;
	auto status = m_pxScene->overlap(
		shape->m_pxShape->getGeometry(),
		PhysXUtils::ToPxTransform(startTransform),
		hit,
		filterData, (filter ? &callback : nullptr)
	);

	/*output.hasBlock = hit.hasBlock;
	if (hit.hasBlock)
	{
		PhysicsSystem_PxSweepHit_Convert(output.block, hit.block);
	}

	output.touches.reserve(output.touches.size() + hit.nbTouches);
	for (size_t i = 0; i < hit.nbTouches; i++)
	{
		auto& ownHit = output.touches.emplace_back();
		PhysicsSystem_PxSweepHit_Convert(ownHit, hit.touches[i]);
	}*/

	return status;
}

void PhysicsSystem::FlushAllQueries()
{
	assert(m_isQueryAvailable);

	while (m_numWritingQueries.load(std::memory_order_relaxed) != 0)
	{
		Thread::Yield();
	}

	// TODO: multi-threaded execute queries
	for (auto& q : m_queries)
	{
		q->ExecuteQuery();
	}
	m_queries.Clear();
}

void PhysicsSystem::ExecuteSerialQueries(SerialQueries* queries)
{
	auto& arr = queries->queries;
	for (size_t i = 0; i < arr.size(); i++)
	{
		auto prev = i == 0 ? nullptr : arr[i - 1].get();
		auto cur = arr[i].get();
		if (queries->checker == nullptr || queries->checker(this, prev, cur))
		{
			cur->ExecuteQuery();
		}
	}
}

void PhysicsSystem::FlushAllSerialQueries()
{
	assert(m_isQueryAvailable);

	while (m_numWritingSerialQueries.load(std::memory_order_relaxed) != 0)
	{
		Thread::Yield();
	}

	// TODO: multi-threaded execute queries
	for (auto& q : m_serialQueries)
	{
		ExecuteSerialQueries(q);
		delete q;
	}
	m_serialQueries.Clear();
}

SharedPtr<ActionBase> PhysicsSystem::Sweep(const SweepResultCallback& callback,
	const PhysicsShape* shape, const Transform& startTransform, const Vec3& distance, 
	const SharedPtr<PhysicsQueryFilterCallback>& filter)
{
	auto ret = ActionPhysicsSweep::Create(this, GetScene()->GetIterationCount() + 1);
	ret->m_callback = callback;
	ret->m_shape = ((PhysicsShape*)shape)->shared_from_this();
	ret->m_startPosition = startTransform.GetPosition();
	ret->m_startRotation = startTransform.GetRotation();
	ret->m_sweepDistance = distance;
	ret->m_filter = filter;

	RecordOrExecuteQuery(ret);

	return ret;
}

SharedPtr<ActionBase> PhysicsSystem::Overlap(const OverlapResultCallback& callback, 
	const PhysicsShape* shape, const Transform& transform, const SharedPtr<PhysicsQueryFilterCallback>& filter)
{
	auto ret = ActionPhysicsOverlap::Create(this, GetScene()->GetIterationCount() + 1);
	ret->m_callback = callback;
	ret->m_shape = ((PhysicsShape*)shape)->shared_from_this();
	ret->m_startPosition = transform.GetPosition();
	ret->m_startRotation = transform.GetRotation();
	ret->m_filter = filter;

	RecordOrExecuteQuery(ret);

	return ret;
}

ID PhysicsSystem::BeginSerialQuery(const QueryPrevCheckCallback& prevCheckCallback)
{
#ifdef _DEBUG
	++m_serialQueriesDebugBeginEndCall;
#endif // _DEBUG

	auto serialQuery = new SerialQueries();
	serialQuery->checker = prevCheckCallback;
	return ID(serialQuery);
}

void PhysicsSystem::EndSerialQuery(ID serialQueryID)
{
#ifdef _DEBUG
	--m_serialQueriesDebugBeginEndCall;
#endif // _DEBUG

	++m_numWritingSerialQueries;

	if (m_isQueryAvailable)
	{
		--m_numWritingSerialQueries;
		ExecuteSerialQueries((SerialQueries*)serialQueryID);
		delete (SerialQueries*)serialQueryID;
		return;
	}

	m_serialQueries.Add((SerialQueries*)serialQueryID);

	--m_numWritingSerialQueries;
}

SharedPtr<ActionPhysicsQuery> PhysicsSystem::SerialSweep(
	ID serialQueryID,
	const SweepResultCallback& callback, 
	const PhysicsShape* shape, 
	const Transform& startTransform, 
	const Vec3& distance,
	const SharedPtr<PhysicsQueryFilterCallback>& filter)
{
	auto ret = ActionPhysicsSweep::Create(this, GetScene()->GetIterationCount() + 1);
	ret->m_callback = callback;
	ret->m_shape = ((PhysicsShape*)shape)->shared_from_this();
	ret->m_startPosition = startTransform.GetPosition();
	ret->m_startRotation = startTransform.GetRotation();
	ret->m_sweepDistance = distance;
	ret->m_filter = filter;

	((SerialQueries*)serialQueryID)->queries.push_back(ret);

	return ret;
}

SharedPtr<ActionPhysicsQuery> PhysicsSystem::SerialOverlap(ID serialQueryID, 
	const OverlapResultCallback& callback, const PhysicsShape* shape, const Transform& transform, const SharedPtr<PhysicsQueryFilterCallback>& filter)
{
	auto ret = ActionPhysicsOverlap::Create(this, GetScene()->GetIterationCount() + 1);
	ret->m_callback = callback;
	ret->m_shape = ((PhysicsShape*)shape)->shared_from_this();
	ret->m_startPosition = transform.GetPosition();
	ret->m_startRotation = transform.GetRotation();
	ret->m_filter = filter;

	((SerialQueries*)serialQueryID)->queries.push_back(ret);

	return ret;
}

///
/// >>>>>>>>>>>>>>>> Direct query section >>>>>>>>>>>>>>>>>
/// 
bool PhysicsSystem::DirectQueryInterface::Sweep(PhysicsSweepResult& output, 
	const PhysicsShape* shape, const Transform& startTransform, const Vec3& distance, PhysicsQueryFilterCallback* filter)
{
	return m_system->SweepImpl(output, shape, startTransform, distance, filter);
}

void PhysicsSystem::ImplDirectQuery(const std::function<void(DirectQueryInterface*)>& callback, ReentrantLock* lock)
{
	DirectQueryInterface interface(this);
	if (lock) lock->lock();
	callback(&interface);
	if (lock) lock->unlock();
}

void PhysicsSystem::FlushAllDirectQueries()
{
	assert(m_isQueryAvailable);

	while (m_numWritingDirectQueries.load(std::memory_order_relaxed) != 0)
	{
		Thread::Yield();
	}

	// TODO: multi-threaded execute queries
	for (auto& q : m_directQueries)
	{
		ImplDirectQuery(q.callback, q.lock);
	}
	m_directQueries.Clear();
}

void PhysicsSystem::Query(const QueryCallback& callback, ReentrantLock* lock)
{
	++m_numWritingDirectQueries;

	if (m_isQueryAvailable)
	{
		--m_numWritingDirectQueries;
		ImplDirectQuery(callback, lock);
		return;
	}

	m_directQueries.Add({ callback, lock });

	--m_numWritingDirectQueries;
}

///
/// <<<<<<<<<<<<<<<< End section <<<<<<<<<<<<<<<<<<<<
///

NAMESPACE_END