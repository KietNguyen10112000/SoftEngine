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
			//| PxPairFlag::eNOTIFY_TOUCH_PERSISTS
			| PxPairFlag::eNOTIFY_CONTACT_POINTS;
		break;
	default:
		pairFlags = PxPairFlag::eSOLVE_CONTACT
			| PxPairFlag::eDETECT_DISCRETE_CONTACT
			| PxPairFlag::eNOTIFY_TOUCH_FOUND
			| PxPairFlag::eNOTIFY_TOUCH_LOST
			//| PxPairFlag::eNOTIFY_TOUCH_PERSISTS
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
		//std::cout << "CONTACT\n";

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

		if (nbPairs == 0)
		{
			//std::cout << "CONTACT LOST\n";
			assert(0);
			return;
		}

		/*if (pairHeader.flags)
		{
			return;
		}*/

		/*{
			for (PxU32 i = 0; i < nbPairs; i++)
			{
				if (pairs[i].flags.isSet(PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH))
				{
					std::cout << "CONTACT LOST\n";
					return;
				}
			}
		}*/

		auto A = (PhysicsComponent*)pairHeader.actors[0]->userData;
		auto B = (PhysicsComponent*)pairHeader.actors[1]->userData;

		if (pairs[0].flags.isSet(PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH))
		{
			auto AEndContactPairs = (A && A->m_collisionResult) ? &A->m_collisionResult->collision.ForceWrite()->endContactPairs : nullptr;
			auto BEndContactPairs = (B && B->m_collisionResult) ? &B->m_collisionResult->collision.ForceWrite()->endContactPairs : nullptr;

			CollisionEndContactPair endContactPair = { A->GetGameObject(), B->GetGameObject() };

			if (AEndContactPairs)
			{
				AEndContactPairs->push_back(endContactPair);
			}

			if (BEndContactPairs)
			{
				std::swap(endContactPair.A, endContactPair.B);
				BEndContactPairs->push_back(endContactPair);
			}

			return;
		}

		assert(pairs[0].flags.isSet(PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH));

		//auto AType = A->GetPhysicsType();
		//auto BType = B->GetPhysicsType();

		//{
		//	CharacterController* controller = nullptr;
		//	if (AType == PHYSICS_TYPE_CHARACTER_CONTROLLER)
		//	{
		//		controller = (CharacterController*)A;
		//	}

		//	if (BType == PHYSICS_TYPE_CHARACTER_CONTROLLER)
		//	{
		//		controller = (CharacterController*)B;
		//		assert((void*)controller != (void*)A);
		//	}

		//	/*if (controller)
		//	{
		//		std::cout << "CONTACT CONTROLLER --- " << controller->IsHasNextMove() << "\n";
		//	}*/

		//	if (controller && controller->IsHasNextMove())
		//	{
		//		//std::cout << "CONTACT CONTROLLER\n";
		//		return;
		//	}
		//}

		auto AContactPairs = (A && A->m_collisionResult) ? &A->m_collisionResult->collision.ForceWrite()->contactPairs : nullptr;
		auto BContactPairs = (B && B->m_collisionResult) ? &B->m_collisionResult->collision.ForceWrite()->contactPairs : nullptr;

		if (!AContactPairs && !BContactPairs)
		{
			return;
		}

		SharedPtr<CollisionContactPair> contactPair = std::make_shared<CollisionContactPair>();

		contactPair->A = A->GetGameObject();
		contactPair->B = B->GetGameObject();

		if (AContactPairs)
		{
			AContactPairs->push_back(contactPair);
		}

		if (BContactPairs)
		{
			BContactPairs->push_back(contactPair);
		}

		auto AActor = (PxRigidActor*)pairHeader.actors[0];
		auto BActor = (PxRigidActor*)pairHeader.actors[1];

		/*PxShape* shape = nullptr;
		PxMaterial* material = nullptr;
		PhysicsShape* myShape = nullptr;*/

		for (PxU32 i = 0; i < nbPairs; i++)
		{
			const PxContactPair& cp = pairs[i];

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

					contactPair->contacts.push_back(dst);

					/*PxU32 internalFaceIndex0 = flippedContacts ?
						iter.getFaceIndex1() : iter.getFaceIndex0();
					PxU32 internalFaceIndex1 = flippedContacts ?
						iter.getFaceIndex0() : iter.getFaceIndex1();*/
					//...
					nbContacts++;
				}
			}
		}

		//for (PxU32 i = 0; i < nbPairs; i++)
		//{
		//	PxU32 contactCount = pairs[i].contactCount;
		//	if (contactCount)
		//	{
		//		if (contactCount > m_contactPairPoints.size())
		//			m_contactPairPoints.resize(contactCount);

		//		auto& pair = pairs[i];
		//		pair.extractContacts(&m_contactPairPoints[0], contactCount);

		//		for (size_t j = 0; j < contactCount; j++)
		//		{
		//			auto& pxPoint = m_contactPairPoints[j];
		//			CollisionContactPoint contactPoint;
		//			contactPoint.position = reinterpret_cast<const Vec3&>(pxPoint.position);
		//			contactPoint.normal = reinterpret_cast<const Vec3&>(pxPoint.normal);
		//			contactPoint.impluse = reinterpret_cast<const Vec3&>(pxPoint.impulse);

		//			/*AActor->getShapes(&shape, 1, pxPoint.internalFaceIndex0);
		//			myShape = (PhysicsShape*)shape->userData;
		//			contactPoint.ASurfaceMaterial = myShape->GetFirstMaterial();

		//			BActor->getShapes(&shape, 1, pxPoint.internalFaceIndex1);
		//			myShape = (PhysicsShape*)shape->userData;
		//			contactPoint.BSurfaceMaterial = myShape->GetFirstMaterial();*/

		//			contactPair->contacts.push_back(contactPoint);
		//		}
		//	}
		//}

		//std::cout << "collision\n";
	}
};

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
	//// collect sleeping collision pairs from previous frame
	//for (auto comp : m_activeComponentsHasContact)
	//{
	//	auto obj = comp->GetGameObject();
	//	auto prevCollision = comp->m_collisionResult->collision.Read();
	//	auto curCollision = comp->m_collisionResult->collision.ForceWrite();

	//	auto compType = comp->GetPhysicsType();

	//	for (auto& pair : curCollision->contactPairs)
	//	{
	//		auto B = pair->A == obj ? pair->B : pair->A;
	//		B->GetComponentRaw<PhysicsComponent>()->isInPrevFrame[0] = true;
	//	}

	//	/*if (compType == PHYSICS_TYPE_CHARACTER_CONTROLLER)
	//	{
	//		continue;
	//	}*/

	//	for (auto& pair : prevCollision->contactPairs)
	//	{
	//		auto B = pair->A == obj ? pair->B : pair->A;

	//		auto& BisInCurFrame = B->GetComponentRaw<PhysicsComponent>()->isInPrevFrame[0];

	//		if (BisInCurFrame)
	//		{
	//			continue;
	//		}

	//		auto BpxActor = B->GetComponentRaw<PhysicsComponent>()->m_pxActor;

	//		if (BpxActor->getScene() != m_pxScene)
	//		{
	//			continue;
	//		}
	//		
	//		auto BdynamicRigidActor = BpxActor->is<PxRigidDynamic>();

	//		/*if (!BdynamicRigidActor)
	//		{
	//			curCollision->contactPairs.push_back(pair);
	//		}*/

	//		if (BdynamicRigidActor && BdynamicRigidActor->isSleeping())
	//		{
	//			curCollision->contactPairs.push_back(pair);
	//		}
	//	}

	//	for (auto& pair : curCollision->contactPairs)
	//	{
	//		auto B = pair->A == obj ? pair->B : pair->A;
	//		B->GetComponentRaw<PhysicsComponent>()->isInPrevFrame[0] = false;
	//	}
	//}

	for (auto comp : m_activeComponentsHasContact)
	{
		auto obj = comp->GetGameObject();
		auto prevCollision = comp->m_collisionResult->collision.Read();
		auto curCollision = comp->m_collisionResult->collision.ForceWrite();

		auto& curContactPairsBegin = curCollision->contactPairs;

		auto& curContactPairs = curCollision->contactPairs;
		curCollision->collisionBeginCount = curContactPairs.size();

		auto& prevContactPairs = prevCollision->contactPairs;

		auto& curContactPairsEnded = curCollision->endContactPairs;
		for (auto& pair : curContactPairsEnded)
		{
			pair.B->GetComponentRaw<PhysicsComponent>()->isInPrevFrame[0] = true;
		}

		auto length = prevContactPairs.size();
		for (size_t i = 0; i < length; i++)
		{
			auto& pair = prevContactPairs[i];
			auto B = pair->A == obj ? pair->B : pair->A;
			if (B->GetComponentRaw<PhysicsComponent>()->isInPrevFrame[0] != true)
			{
				// still in contact
				curContactPairs.push_back(pair);
				continue;
			}

			// lost contact
			curCollision->collisionEnd.push_back(i);
		}

		for (auto& pair : curContactPairsEnded)
		{
			pair.B->GetComponentRaw<PhysicsComponent>()->isInPrevFrame[0] = false;
		}
	}

	//// collect collision begin, collision end
	//for (auto comp : m_activeComponentsHasContact)
	//{
	//	auto obj = comp->GetGameObject();
	//	auto prevCollision = comp->m_collisionResult->collision.Read();
	//	auto curCollision = comp->m_collisionResult->collision.ForceWrite();

	//	auto compType = comp->GetPhysicsType();

	//	auto& collisionBegin = comp->m_collisionResult->collision.ForceWrite()->collisionBegin;
	//	auto& collisionEnd = comp->m_collisionResult->collision.ForceWrite()->collisionEnd;

	//	for (auto& pair : prevCollision->contactPairs)
	//	{
	//		auto B = pair->A == obj ? pair->B : pair->A;
	//		B->GetComponentRaw<PhysicsComponent>()->isInPrevFrame[0] = true;
	//	}

	//	size_t i = 0;
	//	for (auto& pair : curCollision->contactPairs)
	//	{
	//		auto B = pair->A == obj ? pair->B : pair->A;
	//		auto& BisInPrevFrame = B->GetComponentRaw<PhysicsComponent>()->isInPrevFrame[0];

	//		if (BisInPrevFrame == false)
	//		{
	//			collisionBegin.push_back(i);
	//		}

	//		BisInPrevFrame = false;

	//		i++;
	//	}

	//	i = 0;
	//	for (auto& pair : prevCollision->contactPairs)
	//	{
	//		auto B = pair->A == obj ? pair->B : pair->A;
	//		auto& BisInPrevFrame = B->GetComponentRaw<PhysicsComponent>()->isInPrevFrame[0];

	//		if (BisInPrevFrame == true)
	//		{
	//			collisionEnd.push_back(i);
	//		}

	//		BisInPrevFrame = false;

	//		i++;
	//	}
	//}

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