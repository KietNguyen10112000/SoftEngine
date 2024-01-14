#include "CharacterController.h"

#include "PhysX/PhysX.h"

#include "MainSystem/Physics/PhysicsSystem.h"

#include "Scene/Scene.h"
#include "Scene/GameObject.h"

using namespace physx;

NAMESPACE_BEGIN

PxControllerFilters g_defaultPxControllerFilters;

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

NAMESPACE_END