#include "CharacterController.h"

#include "PhysX/PhysX.h"

#include "MainSystem/Physics/PhysicsSystem.h"

#include "Scene/Scene.h"
#include "Scene/GameObject.h"

using namespace physx;

NAMESPACE_BEGIN

class CharacterControllerHitCallback : public PxUserControllerHitReport
{
public:
	void onShapeHit(const PxControllerShapeHit& hit) override
	{
		auto dynamicRigidActor = hit.actor->is<PxRigidDynamic>();
		auto staticRigidActor = hit.actor->is<PxRigidStatic>();

		if (!staticRigidActor && !dynamicRigidActor)
		{
			return;
		}

		if (dynamicRigidActor && !dynamicRigidActor->isSleeping())
		{
			return;
		}

		auto controller = (CharacterController*)hit.controller->getActor()->userData;
		auto system = controller->GetGameObject()->GetScene()->GetPhysicsSystem();

		auto& activeComponentsHasContact = system->m_activeComponentsHasContact;

		PxActor* actors[2] = { hit.controller->getActor(), hit.actor };

		for (auto a : actors)
		{
			auto comp = (PhysicsComponent*)a->userData;
			if (comp && comp->HasPhysicsFlag(PHYSICS_FLAG_ENABLE_COLLISION)
				&& comp->m_collisionResult->lastActiveIterationCount != system->GetScene()->GetIterationCount())
			{
				comp->m_collisionResult->lastActiveIterationCount = system->GetScene()->GetIterationCount();
				activeComponentsHasContact.push_back(comp);
			}
		}

		auto A = (PhysicsComponent*)actors[0]->userData;
		auto B = (PhysicsComponent*)actors[1]->userData;

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

		CollisionContactPoint contactPoint;
		contactPoint.position = reinterpret_cast<const Vec3&>(hit.worldPos);
		contactPoint.normal = reinterpret_cast<const Vec3&>(hit.worldNormal);
		contactPoint.impluse = reinterpret_cast<const Vec3&>(hit.dir * hit.length);

		contactPair->contacts.push_back(contactPoint);
	}

	void onControllerHit(const PxControllersHit& hit) override
	{
	}

	void onObstacleHit(const PxControllerObstacleHit& hit) override
	{
	}

};

PxControllerFilters g_defaultPxControllerFilters;
CharacterControllerHitCallback g_defaultPxControllerHitCallback;
void* g_defaultPxControllerHitCallbackPtr = &g_defaultPxControllerHitCallback;

CharacterController::~CharacterController()
{
	PX_RELEASE(m_pxCharacterController);
}

void CharacterController::TransformContributor(GameObject* object, Transform& local, Mat4& global, void* self)
{
	auto controller = (CharacterController*)self;
	auto gameObject = controller->GetGameObject();
	auto scene = gameObject->GetScene();
	auto pxController = controller->m_pxCharacterController;

	if (gameObject->m_lastWriteLocalTransformIterationCount == scene->GetIterationCount())
	{
		return;
	}

	auto& pxPosition = pxController->getPosition();

	global.SetIdentity();
	global.SetPosition(pxPosition.x, pxPosition.y, pxPosition.z);

	controller->m_lastGlobalTransform = global;

	gameObject->m_isNeedRecalculateLocalTransform = true;
}

void CharacterController::OnPhysicsTransformChanged()
{
	auto obj = GetGameObject();
	obj->ContributeTransform(this, CharacterController::TransformContributor);
	obj->GetScene()->OnObjectTransformChanged(obj);
}

void CharacterController::OnUpdate(float dt)
{
	m_velocity += m_gravity * dt;

	// TODO 
}

void CharacterController::OnPostUpdate(float dt)
{
	Vec3 disp = m_velocity * dt;
	m_pxCharacterController->move(reinterpret_cast<const PxVec3&>(disp), 0.01f, dt, g_defaultPxControllerFilters);
}

void CharacterController::OnTransformChanged()
{
	auto gameObject = GetGameObject();
	auto& globalTransform = gameObject->ReadGlobalTransformMat();

	if (::memcmp(&m_lastGlobalTransform, &globalTransform, sizeof(Mat4)) != 0)
	{
		auto& pos = globalTransform.Position();
		PxExtendedVec3 position = { pos.x, pos.y, pos.z };
		m_pxCharacterController->setPosition(position);
	}
}

void CharacterController::Move(const Vec3& disp, float minDist)
{
	auto system = GetGameObject()->GetScene()->GetPhysicsSystem();
	auto taskRunner = system->AsyncTaskRunner();

	struct Param
	{
		CharacterController* controller;
		Vec3 disp;
		float minDist;
		float dt;
	};

	auto task = taskRunner->CreateTask(
		[](PhysicsSystem* system, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_REF_4(Param, p, controller, disp, minDist, dt);
			controller->m_pxCharacterController->move(reinterpret_cast<const PxVec3&>(disp), minDist, dt, g_defaultPxControllerFilters);
		}
	);

	auto param = taskRunner->CreateParam<Param>(&task);
	param->controller = this;
	param->disp = disp;
	param->minDist = minDist;
	param->dt = GetGameObject()->GetScene()->Dt();

	taskRunner->RunAsync(this, &task);
}

void CharacterController::SetGravity(const Vec3& g)
{
	auto system = GetGameObject()->GetScene()->GetPhysicsSystem();
	auto taskRunner = system->AsyncTaskRunner();

	struct Param
	{
		CharacterController* controller;
		Vec3 g;
	};

	auto task = taskRunner->CreateTask(
		[](PhysicsSystem* system, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_REF_2(Param, p, controller, g);

			controller->m_gravity = g;

			if (g == Vec3::ZERO)
			{
				system->UnscheduleUpdate(controller);
				system->UnschedulePostUpdate(controller);
				return;
			}

			system->ScheduleUpdate(controller);
			system->SchedulePostUpdate(controller);
		}
	);

	auto param = taskRunner->CreateParam<Param>(&task);
	param->controller = this;
	param->g = g;

	taskRunner->RunAsync(this, &task);
}

NAMESPACE_END