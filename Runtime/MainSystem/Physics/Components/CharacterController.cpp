#include "CharacterController.h"

#include "PhysX/PhysX.h"

#include "MainSystem/Physics/PhysicsSystem.h"

#include "Scene/Scene.h"
#include "Scene/GameObject.h"

#include "../Materials/PhysicsMaterial.h"

using namespace physx;

NAMESPACE_BEGIN

class CharacterControllerHitCallback : public PxUserControllerHitReport
{
public:
	void onShapeHit(const PxControllerShapeHit& hit) override
	{
		//std::cout << "onShapeHit all " << "\n";
		return;
		/*auto dynamicRigidActor = hit.actor->is<PxRigidDynamic>();
		auto staticRigidActor = hit.actor->is<PxRigidStatic>();

		if (!staticRigidActor && !dynamicRigidActor)
		{
			return;
		}*/

		/*if (dynamicRigidActor && !dynamicRigidActor->isSleeping())
		{
			return;
		}*/

		//auto controller = (CharacterController*)hit.controller->getActor()->userData;
		//auto system = controller->GetGameObject()->GetScene()->GetPhysicsSystem();

		//std::cout << "onShapeHit all " << system->GetScene()->GetIterationCount() << "\n";

		//auto& activeComponentsHasContact = system->m_activeComponentsHasContact;

		//PxActor* actors[2] = { hit.controller->getActor(), hit.actor };

		//for (auto a : actors)
		//{
		//	auto comp = (PhysicsComponent*)a->userData;
		//	if (comp && comp->HasPhysicsFlag(PHYSICS_FLAG_ENABLE_COLLISION)
		//		&& comp->m_collisionResult->lastActiveIterationCount != system->GetScene()->GetIterationCount())
		//	{
		//		comp->m_collisionResult->lastActiveIterationCount = system->GetScene()->GetIterationCount();
		//		activeComponentsHasContact.push_back(comp);
		//	}
		//}

		////if (hit.actor->is<PxRigidDynamic>())
		////{
		////	//int x = 3;
		////	//std::cout << "onShapeHit dynamic " << system->GetScene()->GetIterationCount() << "\n";
		////}

		//auto A = (PhysicsComponent*)actors[0]->userData;
		//auto B = (PhysicsComponent*)actors[1]->userData;

		//auto AContactPairs = (A && A->m_collisionResult) ? &A->m_collisionResult->collision.ForceWrite()->contactPairs : nullptr;
		//auto BContactPairs = (B && B->m_collisionResult) ? &B->m_collisionResult->collision.ForceWrite()->contactPairs : nullptr;

		//if (!AContactPairs && !BContactPairs)
		//{
		//	return;
		//}

		//SharedPtr<CollisionContactPair> contactPair = std::make_shared<CollisionContactPair>();

		//contactPair->A = A->GetGameObject();
		//contactPair->B = B->GetGameObject();

		//if (AContactPairs)
		//{
		//	AContactPairs->push_back(contactPair);
		//}

		//if (BContactPairs)
		//{
		//	BContactPairs->push_back(contactPair);
		//}

		//CollisionContactPoint contactPoint;
		//contactPoint.position = reinterpret_cast<const Vec3&>(hit.worldPos);
		//contactPoint.normal = reinterpret_cast<const Vec3&>(hit.worldNormal);
		//contactPoint.impulse = reinterpret_cast<const Vec3&>(hit.dir * hit.length);

		//contactPair->contacts.push_back(contactPoint);
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

bool CharacterController::IsHasNextMove()
{
	auto& disp = m_sumDisp[GetGameObject()->GetScene()->GetPrevDeferBufferIdx()];
	return m_lastMoveIterationCount >= GetGameObject()->GetScene()->GetIterationCount() - 1 
		|| disp != Vec3::ZERO 
		|| m_gravity != Vec3::ZERO 
		|| m_velocity != Vec3::ZERO;
}

void CharacterController::OnPhysicsTransformChanged()
{
	auto obj = GetGameObject();
	obj->ContributeTransform(this, CharacterController::TransformContributor);
	obj->GetScene()->OnObjectTransformChanged(obj);
}

void CharacterController::OnUpdate(float dt)
{
	auto mass = m_mass;

	if (HasCollisionAnyChanged())
	{
		m_isOnGround = false;
		m_collisionPlanes.clear();

		Vec3 sumF = Vec3::ZERO;

		auto collisionCount = m_collisionResult->GetContactPointsCount();

		////if (collisionCount == 0)
		//{
		//	m_velocity = Vec3::ZERO;
		//}

		/*if (collisionCount == 1)
		{
			int x = 3;
		}*/

		auto gForce = m_gravity * mass;
		auto eachForce = gForce / (float)collisionCount;

		m_collisionResult->ForEachContactPairs(
			[&](const SharedPtr<CollisionContact>& contact, const SharedPtr<CollisionContactPair>& pair)
			{
				auto& contactPoints = pair->contactPoints;
				auto F = eachForce / contactPoints.size();
				auto nF = F.Normal();

				bool isA = contact->A == GetGameObject() ? true : false;

				for (auto& point : contactPoints)
				{
					auto normal = point.normal;
					if (isA)
					{
						normal = -normal;
					}

					// normal is AB, A is this controller

					CollisionPlane plane;
					plane.normal = -normal;
					plane.staticFriction = point.staticFriction;
					plane.dynamicFriction = point.dynamicFriction;
					plane.isApplyedDynamicFriction = false;
					if (normal.Dot(m_gravity) > 0.01f)
					{
						// ground
						plane.isGround = true;

						m_isOnGround = true;
					}
					else
					{
						// not ground
						plane.isGround = false;

						//sumF += F;
					}

					if (normal.Dot(m_velocity) > 0.01f)
					{
						m_velocity = Vec3::ZERO;
					}

					if (plane.isGround) 
					{
						auto cosA = normal.Dot(nF);
						auto Fn = cosA * F.Length() * normal;
						auto Ft = F - Fn;

						auto FnLen = Fn.Length();

						auto staticFrictionForce = FnLen * point.staticFriction;
						auto dynamicFrictionForce = FnLen * point.dynamicFriction;

						/*if (std::abs(normal.y) != 1)
						{
							int x = 3;
						}*/

						// win static friction and dynamic friction, so this force make cct move
						auto FtLen = Ft.Length();
						if (staticFrictionForce < FnLen && FtLen > dynamicFrictionForce)
						{
							sumF += (Ft - (FtLen - dynamicFrictionForce) * Ft.Normal());

							plane.isApplyedDynamicFriction = true;
						}

						/*if (staticFrictionForce < FnLen)
						{
							sumF += (Ft);
						}*/
					}

					//if (plane.isGround)
					m_collisionPlanes.push_back(plane);
				}
			}
		);

		m_sumF = sumF;
	}

	//if (m_sumF == Vec3::ZERO && !HasCollisionContactPairs())
	if ((m_sumF == Vec3::ZERO && !HasCollisionContactPairs()) || !m_isOnGround)
	{
		m_sumF = m_gravity * mass;
		//m_velocity.y = -10.0f;
	}

	m_velocity += (m_sumF / mass)  * dt;

	auto& disp = m_sumDisp[GetGameObject()->GetScene()->GetPrevDeferBufferIdx()];

	if (m_collisionPlanes.size() != 0)
	//if (HasCollisionContactPairs())
	{
		// slide over first ground
		for (auto& plane : m_collisionPlanes)
		{
			if (!plane.isGround)
			{
				continue;
			}

			auto& firstPlane = plane;
			//auto& firstContact = m_collisionResult->collision.Read()->contacts[0];
			auto& firstPoint = firstPlane.position;//firstContact->contactPairs[0]->contactPoints[0];

			auto& normal = firstPlane.normal;//firstPoint.normal;
			/*bool isA = firstContact->A == GetGameObject() ? true : false;
			if (!isA)
			{
				normal = -normal;
			}*/

			float dot = normal.Dot(m_gravity);

			if (dot < 0)
			{
				auto dispLen = disp.Length();

				if (dispLen != 0)
				{
					auto dispDir = disp.Normal();
					//auto rotation = Quaternion::RotationFromTo(Vec3::UP, normal);
					//dispDir = (Vec4(dispDir, 0.0f) * Mat4::Rotation(rotation)).xyz();

					//disp = dispDir * dispLen;

					auto plane = Plane(firstPoint, normal);
					if (plane.Project(firstPoint + dispDir, m_gravity.Normal(), dispDir))
					{
						dispDir -= firstPoint;
						dispDir.Normalize();
						disp = dispDir * dispLen;
					}

				}
			}

			break;
		}

		uint32_t numNotAppliedDynamicFriction = 0;
		auto vN = m_velocity.Normal();
		for (auto& plane : m_collisionPlanes)
		{
			if (!plane.isGround || plane.isApplyedDynamicFriction)
			{
				continue;
			}

			if (vN.Dot(plane.normal) < -0.01f)
			{
				plane.isGroundForMotion = true;

				numNotAppliedDynamicFriction++;
			}
		}
		
		if (numNotAppliedDynamicFriction != 0 && m_velocity != Vec3::ZERO)
		{
			Vec3 sumV = Vec3::ZERO;
			Vec3 projV;
			auto vEach = m_velocity / (float)numNotAppliedDynamicFriction;
			auto nVEach = vEach.Normal();
			for (auto& plane : m_collisionPlanes)
			{
				if (!plane.isGround || plane.isApplyedDynamicFriction || !plane.isGroundForMotion)
				{
					continue;
				}

				auto cosA = plane.normal.Dot(nVEach);
				auto Vn = cosA * vEach.Length() * plane.normal;
				auto Vt = vEach - Vn;

				Vt = Vt * (1 - plane.dynamicFriction);

				sumV += (Vt + Vn);
			}

			m_velocity = sumV;
		}
	}
}

void CharacterController::OnPostUpdate(float dt)
{
	auto& disp = m_sumDisp[GetGameObject()->GetScene()->GetPrevDeferBufferIdx()];

	disp += m_velocity * dt;
	m_pxCharacterController->move(reinterpret_cast<const PxVec3&>(disp), 0.0f, dt, g_defaultPxControllerFilters);

	//std::cout << disp.y << "\n";

	disp = Vec3::ZERO;
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

		m_lastGlobalTransform = globalTransform;
	}
}

void CharacterController::Move(const Vec3& disp)
{
	auto scene = GetGameObject()->GetScene();
	auto iteration = scene->GetIterationCount();
	if (m_lastMoveIterationCount == iteration)
	{
		return;
	}

	m_lastMoveIterationCount = iteration;

	auto system = scene->GetPhysicsSystem();
	auto taskRunner = system->AsyncTaskRunner();

	struct Param
	{
		CharacterController* controller;
		//Vec3 disp;
		//float minDist;
		float dt;
	};

	auto task = taskRunner->CreateTask(
		[](PhysicsSystem* system, void* p)
		{
			//TASK_SYSTEM_UNPACK_PARAM_REF_4(Param, p, controller, disp, minDist, dt);
			TASK_SYSTEM_UNPACK_PARAM_REF_2(Param, p, controller, dt);

			if (controller->m_gravity != Vec3::ZERO || controller->m_velocity != Vec3::ZERO)
			{
				return;
			}

			auto& disp = controller->m_sumDisp[controller->GetGameObject()->GetScene()->GetPrevDeferBufferIdx()];
			controller->m_pxCharacterController->move(reinterpret_cast<const PxVec3&>(disp), 0.0f, dt, g_defaultPxControllerFilters);
			disp = Vec3::ZERO;
		}
	);

	auto param = taskRunner->CreateParam<Param>(&task);
	param->controller = this;
	//param->disp = disp;
	//param->minDist = minDist;
	param->dt = GetGameObject()->GetScene()->Dt();

	taskRunner->RunAsync(this, &task);

	m_sumDisp[scene->GetCurrentDeferBufferIdx()] += disp;
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

void CharacterController::CCTApplyImpulse(const Vec3& impulse)
{
	auto system = GetGameObject()->GetScene()->GetPhysicsSystem();
	auto taskRunner = system->AsyncTaskRunner();

	struct Param
	{
		CharacterController* controller;
		Vec3 addVelocity;
	};

	auto task = taskRunner->CreateTask(
		[](PhysicsSystem* system, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_REF_2(Param, p, controller, addVelocity);

			controller->m_velocity += addVelocity;
		}
	);

	auto param = taskRunner->CreateParam<Param>(&task);
	param->controller = this;
	param->addVelocity = impulse / m_mass;

	taskRunner->RunAsync(this, &task);
}

bool CharacterController::CCTIsOnGround()
{
	return m_isOnGround;
}

NAMESPACE_END