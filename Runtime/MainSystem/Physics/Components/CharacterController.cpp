#include "CharacterController.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "MainSystem/Physics/PhysicsSystem.h"
#include "MainSystem/MainSystemTaskPacking.h"

#include "Scene/Scene.h"
#include "Scene/GameObject.h"

#include "../Materials/PhysicsMaterial.h"
#include "../Shapes/PhysicsShape.h"
#include "../FILTER_FLAG.h"
#include "../Query/PhysicsQueryFilterCallback.h"

#include "Common/Actions/ActionInterpolation.h"
#include "MainSystem/Animation/Utils/Animation.h"

using namespace physx;

NAMESPACE_BEGIN

class CCTDefaultFilterCallBack : public PxQueryFilterCallback
{
public:
	CharacterController* m_cct = nullptr;

	CCTDefaultFilterCallBack(CharacterController* cct) : m_cct(cct)
	{

	}

	// Inherited via PxQueryFilterCallback
	PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override
	{
		auto comp = (PhysicsComponent*)actor->userData;
		if (comp->GetGameObject()->GetRoot() == m_cct->GetGameObject()->GetRoot())
		{
			return PxQueryHitType::eNONE;
		}

		if (m_cct->m_filterCallback)
		{
			auto obj = ((PhysicsComponent*)actor->userData)->GetGameObject();
			PhysicsHitFlags flags = uint32_t(queryFlags);
			return PxQueryHitType::Enum(m_cct->m_filterCallback->PrevFilter(obj, (PhysicsShape*)shape->userData, flags));
		}

		return PxQueryHitType::eTOUCH;
	}
	PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit, const PxShape* shape, const PxRigidActor* actor) override
	{
		return PxQueryHitType::eTOUCH;
	}
};

class CharacterControllerHitCallback : public PxUserControllerHitReport
{
public:
	void onShapeHit(const PxControllerShapeHit& hit) override
	{
		auto cct = (CharacterController*)hit.controller->getUserData();
		auto& collisionPlanes = *cct->m_collisionPlanesBuffer.Write();
		auto& planes = collisionPlanes.planes;

		auto& plane = planes.emplace_back();
		plane.position = PhysXUtils::ToVec3(hit.worldPos);
		plane.normal = PhysXUtils::ToVec3(hit.worldNormal);
		plane.shape = (PhysicsShape*)hit.shape->userData;
		plane.object = ((PhysicsComponent*)hit.actor->userData)->GetGameObject();
		plane.isGround = plane.TestGround(cct->m_gravity);

		if (plane.isGround)
		{
			collisionPlanes.groundCount++;
		}
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


void CharacterControllerDesc::ToPxDesc(void* pxDesc)
{
	auto& desc = *(PxControllerDesc*)pxDesc;
	desc.material = material->m_pxMaterial;
	desc.slopeLimit = slopeLimit;
	desc.stepOffset = stepOffset;
	desc.contactOffset = contactOffset;
}

void CharacterControllerDesc::ToJson(Serializer* serializer, json& j)
{
	j["Material"] = serializer->Serialize(material);
	j["SlopeLimit"] = slopeLimit;
	j["StepOffset"] = stepOffset;
	j["ContactOffset"] = contactOffset;
}

void CharacterControllerDesc::FromJson(Serializer* serializer, const json& j)
{
	serializer->Deserialize(j["Material"], material);

	if (j.contains("SlopeLimit"))
	{
		slopeLimit = j["SlopeLimit"];
		stepOffset = j["StepOffset"];
		contactOffset = j["ContactOffset"];
	}
}

CharacterController::CharacterController()
{
	m_defaultCCTFilterCallback = new CCTDefaultFilterCallBack(this);
}

CharacterController::~CharacterController()
{
	if (m_pxCharacterController)
		m_pxCharacterController->setUserData(nullptr);

	if (m_defaultCCTFilterCallback)
	{
		delete m_defaultCCTFilterCallback;
		m_defaultCCTFilterCallback = nullptr;
	}

	PX_RELEASE(m_pxCharacterController);
}

void CharacterController::ReduceVelocityByCollisionPlanes(float dt)
{
	// block all velocity vectors that direct to collision planes
	auto& collisionPlanes = CCTGetCollisionPlanes();

	if (collisionPlanes.groundCount == 0 || m_velocity.Length2() < 0.00001f)
	{
		/*if (m_velocity.Length() < 0.0001f)
		{
			m_velocity = Vec3::ZERO;
		}*/
		return;
	}
	Vec3 sumV = Vec3::ZERO;

	auto invDVelcity = -m_velocity.Normal();
	auto VLength = m_velocity.Length();

	//auto ourStaticFriction = m_shapes[0]->GetFirstMaterial()->GetStaticFriction();
	auto ourDynamicFriction = m_shapes[0]->GetFirstMaterial()->GetDynamicFriction();

	if (m_overrideVelocityDynamicFriction != 0.0f)
	{
		ourDynamicFriction = m_overrideVelocityDynamicFriction;
	}

	auto slopeLimit = CCTGetSlopeLimit();

	int groundCount = 0;
	for (auto& plane : collisionPlanes.planes)
	{
		if (!plane.TestGround(m_velocity))
		{
			//sumV += m_velocity;
			continue;
		}
		auto cosA = plane.normal.Dot(invDVelcity);
		//std::cout << cosA << "\n";
		if (cosA < slopeLimit)
		{
			//sumV += m_velocity;
			continue;
		}

		groundCount++;
		//if (std::abs(cosA - 1.0f) < 0.00001f)
		//{
		//	continue;
		//}

		//auto Vn = -VLength * cosA * plane.normal;
		//auto Vt = m_velocity - Vn;

		//auto material = plane.shape->GetFirstMaterial().get();
		////auto staticFriction = material->GetStaticFriction();
		//auto dynamicFriction = std::clamp((material->GetDynamicFriction() + ourDynamicFriction) / 2.0f, 0.0f, 1.0f);

		//Vt = (1.0f - dynamicFriction) * Vt;

		//sumV += Vt;
	}

	/*if (groundCount != 0)
	{
		sumV /= float(groundCount);
		m_velocity = sumV;
	}*/

	//std::cout << "groundCount: " << groundCount << "\n";
	if (groundCount != 0 || m_velocity.Length() < 0.0001f)
	{
		m_velocity = Vec3::ZERO;
	}
}

void CharacterController::ApplyGravity(float dt)
{
	auto& collisionPlanes = CCTGetCollisionPlanes();
	if (collisionPlanes.groundCount == 0)
	{
		m_velocity += m_gravity * dt;
		return;
	}

	Vec3 sumG = Vec3::ZERO;

	auto eachG = m_gravity / float(collisionPlanes.groundCount);
	auto invDGravity = -m_gravity.Normal();
	auto GLength = eachG.Length();

	auto ourStaticFriction = m_shapes[0]->GetFirstMaterial()->GetStaticFriction();
	auto ourDynamicFriction = m_shapes[0]->GetFirstMaterial()->GetDynamicFriction();

	if (m_overrideGravityStaticFriction != 0.0f)
	{
		ourStaticFriction = m_overrideGravityStaticFriction;
	}

	if (m_overrideGravityDynamicFriction != 0.0f)
	{
		ourDynamicFriction = m_overrideGravityDynamicFriction;
	}

	for (auto& plane : collisionPlanes.planes)
	{
		if (!plane.isGround)
		{
			return;
		}

		auto cosA = plane.normal.Dot(invDGravity);
		if (std::abs(cosA - 1.0f) < 0.00001f)
		{
			continue;
		}

		auto Gn = -GLength * cosA * plane.normal;
		auto Gt = eachG - Gn;

		auto material = plane.shape->GetFirstMaterial().get();
		auto staticFriction = std::clamp((material->GetStaticFriction() + ourStaticFriction) / 2.0f, 0.0f, 1.0f);
		auto dynamicFriction = std::clamp((material->GetDynamicFriction() + ourDynamicFriction) / 2.0f, 0.0f, 1.0f);

		auto Ft = Gt.Length2();
		auto Fstatic = Gn.Length2() * staticFriction;

		// static force wins dynamic force, no contribution here
		if (Fstatic >= Ft)
		{
			continue;
		}

		Gt = dynamicFriction * Gt;
		sumG += Gt;
	}

	//std::cout << "sumG: " << sumG.x << ", " << sumG.y << ", " << sumG.z << "\n";
	//auto temp = m_velocity;
	m_velocity += sumG * dt;
	//assert(temp.y >= m_velocity.y);
}

void CharacterController::ApplyAditionRotation(float dt)
{
	//CCTSetRotationImpl(m_rotation * m_additionRotation);
}

void CharacterController::CCTSetRotationImpl(const Quaternion& rotation)
{
	m_rotation = rotation;
	auto up = Mat4::Rotation(rotation).Up().Normal();
	auto curUp = PhysXUtils::ToVec3(m_pxCharacterController->getUpDirection()).Normal();
	if (AngleBetween(up, curUp) > 0.001f)
	{
		m_pxCharacterController->setUpDirection(reinterpret_cast<const PxVec3&>(up));
		std::cout << "SetUpDirection\n";
	}

	OnPhysicsTransformChanged();
}

void CharacterController::Wake()
{
	
}

void CharacterController::OnPhysicsTransformChanged()
{
	auto obj = GetGameObject();

	auto pxController = m_pxCharacterController;
	auto& pxPosition = pxController->getPosition();

	Mat4 global = Mat4::Identity();
	global *= Mat4::Rotation(m_rotation);
	global.SetPosition(pxPosition.x, pxPosition.y, pxPosition.z);
	obj->SetGlobalTransform(global, COMPONENT_ID);
}

void CharacterController::OnUpdate(float dt)
{
	//auto mass = m_mass;

	//if (HasCollisionAnyChanged())
	//{
	//	m_isOnGround = false;
	//	m_collisionPlanes.clear();

	//	Vec3 sumF = Vec3::ZERO;

	//	auto collisionCount = m_collisionResult->GetContactPointsCount();

	//	////if (collisionCount == 0)
	//	//{
	//	//	m_velocity = Vec3::ZERO;
	//	//}

	//	/*if (collisionCount == 1)
	//	{
	//		int x = 3;
	//	}*/

	//	auto gForce = m_gravity * mass;
	//	auto eachForce = gForce / (float)collisionCount;

	//	m_collisionResult->ForEachContactPairs(
	//		[&](const SharedPtr<CollisionContact>& contact, const SharedPtr<CollisionContactPair>& pair)
	//		{
	//			auto& contactPoints = pair->contactPoints;
	//			auto F = eachForce / contactPoints.size();
	//			auto nF = F.Normal();

	//			bool isA = contact->A == GetGameObject() ? true : false;

	//			for (auto& point : contactPoints)
	//			{
	//				auto normal = point.normal;
	//				if (isA)
	//				{
	//					normal = -normal;
	//				}

	//				// normal is AB, A is this controller

	//				CollisionPlane plane;
	//				plane.normal = -normal;
	//				plane.staticFriction = point.staticFriction;
	//				plane.dynamicFriction = point.dynamicFriction;
	//				plane.isApplyedDynamicFriction = false;
	//				if (normal.Dot(m_gravity) > 0.01f)
	//				{
	//					// ground
	//					plane.isGround = true;

	//					m_isOnGround = true;
	//				}
	//				else
	//				{
	//					// not ground
	//					plane.isGround = false;

	//					//sumF += F;
	//				}

	//				if (normal.Dot(m_velocity) > 0.01f)
	//				{
	//					m_velocity = Vec3::ZERO;
	//				}

	//				if (plane.isGround) 
	//				{
	//					auto cosA = normal.Dot(nF);
	//					auto Fn = cosA * F.Length() * normal;
	//					auto Ft = F - Fn;

	//					auto FnLen = Fn.Length();

	//					auto staticFrictionForce = FnLen * point.staticFriction;
	//					auto dynamicFrictionForce = FnLen * point.dynamicFriction;

	//					/*if (std::abs(normal.y) != 1)
	//					{
	//						int x = 3;
	//					}*/

	//					// win static friction and dynamic friction, so this force make cct move
	//					auto FtLen = Ft.Length();
	//					if (staticFrictionForce < FnLen && FtLen > dynamicFrictionForce)
	//					{
	//						sumF += (Ft - (FtLen - dynamicFrictionForce) * Ft.Normal());

	//						plane.isApplyedDynamicFriction = true;
	//					}

	//					/*if (staticFrictionForce < FnLen)
	//					{
	//						sumF += (Ft);
	//					}*/
	//				}

	//				//if (plane.isGround)
	//				m_collisionPlanes.push_back(plane);
	//			}
	//		}
	//	);

	//	m_sumF = sumF;
	//}

	////if (m_sumF == Vec3::ZERO && !HasCollisionContactPairs())
	//if ((m_sumF == Vec3::ZERO && !HasCollisionContactPairs()) || !m_isOnGround)
	//{
	//	m_sumF = m_gravity * mass;
	//	//m_velocity.y = -10.0f;
	//}

	//m_velocity += (m_sumF / mass)  * dt;

	//auto& disp = m_lastDisp;
	//disp = m_sumDisp[GetGameObject()->GetScene()->GetPrevDeferBufferIdx()];
	//m_sumDisp[GetGameObject()->GetScene()->GetPrevDeferBufferIdx()] = Vec3::ZERO;

	////std::cout << json(disp) << "\n";

	//if (m_collisionPlanes.size() != 0)
	////if (HasCollisionContactPairs())
	//{
	//	// slide over first ground
	//	for (auto& plane : m_collisionPlanes)
	//	{
	//		if (!plane.isGround)
	//		{
	//			continue;
	//		}

	//		auto& firstPlane = plane;
	//		//auto& firstContact = m_collisionResult->collision.Read()->contacts[0];
	//		auto& firstPoint = firstPlane.position;//firstContact->contactPairs[0]->contactPoints[0];

	//		auto& normal = firstPlane.normal;//firstPoint.normal;
	//		/*bool isA = firstContact->A == GetGameObject() ? true : false;
	//		if (!isA)
	//		{
	//			normal = -normal;
	//		}*/

	//		float dot = normal.Dot(m_gravity);

	//		if (dot < 0)
	//		{
	//			auto dispLen = disp.Length();

	//			if (dispLen != 0)
	//			{
	//				auto dispDir = disp.Normal();
	//				//auto rotation = Quaternion::RotationFromTo(Vec3::UP, normal);
	//				//dispDir = (Vec4(dispDir, 0.0f) * Mat4::Rotation(rotation)).xyz();

	//				//disp = dispDir * dispLen;

	//				auto plane = Plane(firstPoint, normal);
	//				if (plane.Project(firstPoint + dispDir, m_gravity.Normal(), dispDir))
	//				{
	//					dispDir -= firstPoint;
	//					dispDir.Normalize();
	//					disp = dispDir * dispLen;
	//				}

	//			}
	//		}

	//		break;
	//	}

	//	uint32_t numNotAppliedDynamicFriction = 0;
	//	if (m_velocity != Vec3::ZERO)
	//	{
	//		auto vN = m_velocity.Normal();
	//		for (auto& plane : m_collisionPlanes)
	//		{
	//			if (!plane.isGround || plane.isApplyedDynamicFriction)
	//			{
	//				continue;
	//			}

	//			if (vN.Dot(plane.normal) < -0.01f)
	//			{
	//				plane.isGroundForMotion = true;

	//				numNotAppliedDynamicFriction++;
	//			}
	//		}
	//	}
	//	
	//	if (numNotAppliedDynamicFriction != 0 && m_velocity != Vec3::ZERO)
	//	{
	//		Vec3 sumV = Vec3::ZERO;
	//		Vec3 projV;
	//		auto vEach = m_velocity / (float)numNotAppliedDynamicFriction;
	//		auto nVEach = vEach.Normal();
	//		for (auto& plane : m_collisionPlanes)
	//		{
	//			if (!plane.isGround || plane.isApplyedDynamicFriction || !plane.isGroundForMotion)
	//			{
	//				continue;
	//			}

	//			auto cosA = plane.normal.Dot(nVEach);
	//			auto Vn = cosA * vEach.Length() * plane.normal;
	//			auto Vt = vEach - Vn;

	//			Vt = Vt * (1 - plane.dynamicFriction);

	//			sumV += (Vt + Vn);
	//		}

	//		m_velocity = sumV;
	//	}
	//}
}

void CharacterController::OnPrevUpdate(float dt)
{
	auto scene = GetGameObject()->GetScene();
	auto& disp = m_sumDisp;//[scene->GetPrevDeferBufferIdx()];

	if (m_isEnableGravity && m_gravity != Vec3::ZERO)
	{
		ApplyGravity(dt);
	}

	if (m_isEnableAdditionRotation)
	{
		ApplyAditionRotation(dt);
	}

	ReduceVelocityByCollisionPlanes(dt);

	m_lock.lock();
	m_committedVelocity = m_velocity;
	m_lock.unlock();

	disp += m_velocity * dt;
	
	if (disp.Length() > 0.0001f)
	{
		scene->BeginWrite<false>(m_collisionPlanesBuffer);
		auto p = m_collisionPlanesBuffer.Write();
		p->planes.clear();
		p->groundCount = 0;

		//std::cout << "m_velocity: " << m_velocity.x << ", " << m_velocity.y << ", " << m_velocity.z << '\n';
		//std::cout << "Move: " << disp.x << ", " << disp.y << ", " << disp.z << "\n\n";

		auto flag = m_pxCharacterController->move(reinterpret_cast<const PxVec3&>(disp), 0.0001f, dt,
			PxControllerFilters(nullptr, m_defaultCCTFilterCallback, nullptr));

		//std::cout << flag.isSet(PxControllerCollisionFlag::eCOLLISION_DOWN) << "\n";

		scene->EndWrite(m_collisionPlanesBuffer);
	}

	disp = Vec3::ZERO;
}

Handle<ClassMetadata> CharacterController::GetMetadata(size_t sign)
{
	auto meta = mheap::New<ClassMetadata>("CharacterController", this);

	{
		auto accessor = Accessor(
			"Contact Offset",
			1,
			[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
			{
				auto obj = (CharacterController*)instance;
				obj->CCTSetContactOffset(std::max(input.As<float>(), 0.0f));
			},
			[](UnknownAddress& var, Serializable* instance) -> Variant
			{
				auto obj = (CharacterController*)instance;
				return Variant::Of(obj->CCTGetContactOffset());
			},
			this
		);
		meta->AddProperty(accessor);
	}

	{
		auto accessor = Accessor(
			"Step Offset",
			1,
			[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
			{
				auto obj = (CharacterController*)instance;
				obj->CCTSetStepOffset(std::max(input.As<float>(), 0.0f));
			},
			[](UnknownAddress& var, Serializable* instance) -> Variant
			{
				auto obj = (CharacterController*)instance;
				return Variant::Of(obj->CCTGetStepOffset());
			},
			this
		);
		meta->AddProperty(accessor);
	}

	{
		auto accessor = Accessor(
			"Slop Limit",
			1,
			[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
			{
				auto obj = (CharacterController*)instance;
				obj->CCTSetSlopeLimit(std::clamp(input.As<float>(), 0.0f, 1.0f));
			},
			[](UnknownAddress& var, Serializable* instance) -> Variant
			{
				auto obj = (CharacterController*)instance;
				return Variant::Of(obj->CCTGetSlopeLimit());
			},
			this
		);
		meta->AddProperty(accessor);
	}

	return meta;
}

void CharacterController::OnTransformChanged()
{
	if (!m_pxCharacterController)
	{
		return;
	}

	auto gameObject = GetGameObject();
	auto& globalTransform = gameObject->GetCommittedGlobalTransform();

	Vec3 pos, scale;
	globalTransform.Decompose(scale, m_rotation, pos);

	auto rotationMat = Mat4::Rotation(m_rotation);

	//if (::memcmp(&m_lastGlobalTransform, &globalTransform, sizeof(Mat4)) != 0)
	{
		auto& pos = globalTransform.Position();
		PxExtendedVec3 position = { pos.x, pos.y, pos.z };
		m_pxCharacterController->setPosition(position);

		auto curUp = PhysXUtils::ToVec3(m_pxCharacterController->getUpDirection()).Normal();
		auto up = rotationMat.Up().Normal();
		if (AngleBetween(up, curUp) > 0.001f)
		{
			std::cout << "CharacterController::OnTransformChanged --- SetUpDirection\n";
			m_pxCharacterController->setUpDirection(reinterpret_cast<const PxVec3&>(up));
		}
		m_lastRotation = m_rotation;
	}
}

void CharacterController::Move(const Vec3& disp)
{
	auto scene = GetGameObject()->GetScene();

#ifdef _DEBUG
	auto iteration = scene->GetIterationCount();
	auto& atom = (std::atomic<size_t>&)m_lastMoveIterationCount;
	if (atom.exchange(iteration) == iteration)
	{
		// to correct character movement, Move() function should be called only one time per frame. 
		//assert(0 && "Multiple actors called move() in a frame.");
		//return;
		std::cout << "[WARN]: Multiple actors called move() in a single frame.\n";
	}
#endif // _DEBUG

	MAIN_SYSTEM_TASK_1(
		PhysicsSystem, AsyncTaskRunner, disp,
		{
			self->m_sumDisp += disp;
		}
	);

	////m_lastMoveIterationCount = iteration;

	//auto system = scene->GetPhysicsSystem();
	//auto taskRunner = system->AsyncTaskRunnerST();

	//struct Param
	//{
	//	CharacterController* controller;
	//	float dt;
	//};

	//auto task = taskRunner->CreateTask(
	//	[](PhysicsSystem* system, void* p)
	//	{
	//		TASK_SYSTEM_UNPACK_PARAM_REF_2(Param, p, controller, dt);

	//		if (controller->m_isEnableGravity || (controller->m_gravity != Vec3::ZERO || controller->m_velocity != Vec3::ZERO))
	//		{
	//			return;
	//		}

	//		auto& disp = controller->m_sumDisp[controller->GetGameObject()->GetScene()->GetPrevDeferBufferIdx()];
	//		controller->m_pxCharacterController->move(reinterpret_cast<const PxVec3&>(disp), 0.0f, dt, 
	//			PxControllerFilters(nullptr, controller->m_defaultCCTFilterCallback, nullptr));

	//		disp = Vec3::ZERO;
	//	}
	//);

	//auto param = taskRunner->CreateParam<Param>(&task);
	//param->controller = this;
	//param->dt = GetGameObject()->GetScene()->Dt();

	//taskRunner->RunAsync(this, &task);

	//m_sumDisp[scene->GetCurrentDeferBufferIdx()] += disp;
}

void CharacterController::SetGravity(const Vec3& g)
{
	MAIN_SYSTEM_TASK_1(
		PhysicsSystem, AsyncTaskRunner, g,
		{
			self->m_gravity = g;
		}
	);
}

void CharacterController::SetGravityEnabled(bool enable)
{
	if (m_isEnableGravity == enable)
	{
		return;
	}
	m_isEnableGravity = enable;

	MAIN_SYSTEM_TASK_1(
		PhysicsSystem, AsyncTaskRunner, enable,
		{
			if (enable)
			{
				if (self->m_countScheduleUpdate++ == 0)
				{
					//system->ScheduleUpdate(self);
					system->SchedulePrevUpdate(self);
				}
			}

			if (!enable)
			{
				//system->UnscheduleUpdate(self);
				system->UnschedulePrevUpdate(self);
			}
		}
	);
}

void CharacterController::CCTSetVelocity(const Vec3& velocity)
{
	MAIN_SYSTEM_TASK_1(
		PhysicsSystem, AsyncTaskRunner, velocity,
		{
			self->m_velocity = velocity;
		}
	);
}

Vec3 CharacterController::CCTGetVelocity() const
{
	return m_velocity;
}

void CharacterController::CCTApplyVelocity(const Vec3& velocity)
{
	MAIN_SYSTEM_TASK_1(
		PhysicsSystem, AsyncTaskRunner, velocity,
		{
			self->m_velocity += velocity;
		}
	);
}

void CharacterController::CCTApplyImpulse(const Vec3& impulse)
{
	CCTApplyVelocity(impulse / m_mass);
}

bool CharacterController::CCTIsOnGround()
{
	return m_isOnGround;
}

//void CharacterController::CCTSetContactFilterCallback(RigidBody::ContactReportFilterCallback callback)
//{
//	m_contactFilterCallback = callback;
//
//	MAIN_SYSTEM_TASK_0(
//		PhysicsSystem, AsyncTaskRunnerST,
//		{
//			PxShape* shape = nullptr;
//			auto pxActor = self->m_pxCharacterController->getActor();
//			pxActor->getShapes(&shape, 1);
//
//			PxFilterData data = shape->getSimulationFilterData();
//			data.word0 |= (PHYSICS_FILTER_FLAG::CALLBACK | PHYSICS_FILTER_FLAG::CCT);
//			shape->setSimulationFilterData(data);
//		}
//	);
//}

void CharacterController::CCTSetRotation(const Quaternion& rotation)
{
	MAIN_SYSTEM_TASK_1(
		PhysicsSystem, AsyncTaskRunnerST, rotation,
		{
			self->CCTSetRotationImpl(rotation);
		}
	);

	m_lastRotation = rotation;
}

void CharacterController::CCTSetAdditionRotationEnabled(bool enable)
{
	MAIN_SYSTEM_TASK_1(
		PhysicsSystem, AsyncTaskRunnerST, enable,
		{
			self->m_isEnableAdditionRotation = enable;
		}
	);
}

void CharacterController::CCTSetAdditionRotation(const Quaternion& rotation)
{
	MAIN_SYSTEM_TASK_1(
		PhysicsSystem, AsyncTaskRunnerST, rotation,
		{
			self->m_additionRotation = rotation;
		}
	);
}

const CharacterController::CollisionPlanes& CharacterController::CCTGetCollisionPlanes()
{
	// to use this function, ctt must be in a gravity field
	assert(m_isEnableGravity);
	
	return *m_collisionPlanesBuffer.Read();
}

Vec3 CharacterController::GetGravity() const
{
	return m_gravity;
}

Vec3 CharacterController::GetVelocity() const
{
	m_lock.lock();
	auto ret = m_committedVelocity;
	m_lock.unlock();
	return ret;
}

void CharacterController::CCTSetSlopeLimit(float cosAngle)
{
	m_pDerivedDesc->slopeLimit = cosAngle;
	MAIN_SYSTEM_TASK_1(
		PhysicsSystem, AsyncTaskRunnerST, cosAngle,
		{
			self->m_pxCharacterController->setSlopeLimit(cosAngle);
		}
	);
}

float CharacterController::CCTGetSlopeLimit() const
{
	return m_pDerivedDesc->slopeLimit;
}

void CharacterController::CCTSetStepOffset(float stepOffset)
{
	m_pDerivedDesc->stepOffset = stepOffset;
	MAIN_SYSTEM_TASK_1(
		PhysicsSystem, AsyncTaskRunnerST, stepOffset,
		{
			self->m_pxCharacterController->setStepOffset(stepOffset);
		}
	);
}

float CharacterController::CCTGetStepOffset() const
{
	return m_pDerivedDesc->stepOffset;
}

void CharacterController::CCTSetContactOffset(float contactOffset)
{
	m_pDerivedDesc->contactOffset = contactOffset;
	MAIN_SYSTEM_TASK_1(
		PhysicsSystem, AsyncTaskRunnerST, contactOffset,
		{
			self->m_pxCharacterController->setContactOffset(contactOffset);
		}
	);
}

float CharacterController::CCTGetContactOffset() const
{
	return m_pDerivedDesc->contactOffset;
}

void CharacterController::CCTSetFilterCallback(const SharedPtr<PhysicsQueryFilterCallback>& filter)
{
	MAIN_SYSTEM_TASK_1(
		PhysicsSystem, AsyncTaskRunnerST, filter,
		{
			self->m_filterCallback = filter;
		}
	);
}

NAMESPACE_END