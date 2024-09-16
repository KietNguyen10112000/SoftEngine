#include "CharacterControllerCapsule.h"

#include "PhysX/PhysX.h"
#include "MainSystem/Physics/PhysicsSystem.h"
#include "MainSystem/MainSystemTaskPacking.h"

#include "Scene/Scene.h"
#include "Scene/GameObject.h"

#include "Graphics/DebugGraphics.h"
#include "Graphics/Graphics.h"

#include "../FILTER_FLAG.h"

#include "../Shapes/PhysicsShapeCapsule.h"
#include "../Materials/PhysicsMaterial.h"

#include "Scene/GameObject.h"

using namespace physx;

NAMESPACE_BEGIN

extern void* g_defaultPxControllerHitCallbackPtr;

CharacterControllerCapsule::CharacterControllerCapsule()
{
	m_desc.capsule = Capsule({ 0,0,0 }, 1.0f, 0.5f);
	m_desc.material = std::make_shared<PhysicsMaterial>(0.5f, 0.5f, 0.5f);
}

CharacterControllerCapsule::CharacterControllerCapsule(const CharacterControllerCapsuleDesc& desc)
{
	m_desc = desc;
}

CharacterControllerCapsule::~CharacterControllerCapsule()
{
	m_pxActor = nullptr;
}

void CharacterControllerCapsule::InitializeCCT(Scene* scene)
{
	m_pDerivedDesc = &m_desc;

	auto& desc = m_desc;

	PxCapsuleControllerDesc pxDesc = {};
	pxDesc.height = desc.capsule.m_height;
	pxDesc.radius = desc.capsule.m_radius;
	pxDesc.upDirection = reinterpret_cast<const PxVec3&>(desc.capsule.m_up);
	pxDesc.position = PxExtendedVec3(desc.capsule.m_center.x, desc.capsule.m_center.y, desc.capsule.m_center.z);
	pxDesc.reportCallback = (decltype(pxDesc.reportCallback))g_defaultPxControllerHitCallbackPtr;
	pxDesc.scaleCoeff = 1.0f;
	
	m_pDerivedDesc->ToPxDesc(&pxDesc);

	m_pxCharacterController = scene->GetPhysicsSystem()->m_pxControllerManager->createController(pxDesc);
	m_pxCharacterController->setUserData(this);

	auto pxActor = m_pxCharacterController->getActor();
	pxActor->userData = this;

	PxShape* shape = nullptr;
	pxActor->getShapes(&shape, 1);
	assert(shape != nullptr);

	m_shape = PhysicsShapeCapsule::MakeDummy(shape, desc.capsule.m_height, desc.capsule.m_radius, desc.material);
	m_shapes.push_back(m_shape);

	m_shape->m_attachedRigidBody = this;

	//if (shape)
	{
		PxFilterData data;
		data.word0 = PHYSICS_FILTER_FLAG::CCT;
		shape->setSimulationFilterData(data);

		shape->setGeometry(PxCapsuleGeometry(pxDesc.radius + 2.0f * pxDesc.contactOffset + 0.05f, pxDesc.height / 2.0f + 2.0f * pxDesc.contactOffset + 0.05f));
	}

	auto mass = m_pxCharacterController->getActor()->getMass();
	mass = mass <= 0 ? 1 : mass;
	m_mass = mass;

	m_pxActor = pxActor;
}

void CharacterControllerCapsule::OnDrawDebug()
{
	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (debugGraphics && m_pxCharacterController)
	{
		auto pxController = (PxCapsuleController*)m_pxCharacterController;

		auto& pos = GetGameObject()->GetCommittedGlobalTransform().Position();//pxController->getPosition();

		/*if (pos.y > 70)
		{
			std::cout << "===========================> " << pos.y << "\n";
		}*/

		Vec4 color = Vec4(0, 0, 0, 1);
		if (m_collisionResult)
		{
			auto size = m_collisionResult->GetContactPairsCount();
			if (size == 1)
			{
				color = { 1,0,0,1 };
			}

			if (size == 2)
			{
				color = { 0,1,0,1 };
			}

			if (size > 2)
			{
				color = { 0,0,1,1 };
			}
		}

		debugGraphics->DrawCapsule(
			Capsule(
				reinterpret_cast<const Vec3&>(pxController->getUpDirection()),
				Vec3(pos.x, pos.y, pos.z),
				pxController->getHeight(),
				pxController->getRadius()
			),
			color
		);
	}
}

void CharacterControllerCapsule::OnComponentAdded()
{
	InitializeCCT(GetGameObject()->GetScene());
}

void CharacterControllerCapsule::OnComponentRemoved()
{
	assert(m_pxCharacterController != nullptr);

	m_shape = nullptr;
	m_pxCharacterController->release();
	m_pxCharacterController = nullptr;
}

AABox CharacterControllerCapsule::GetGlobalAABB()
{
	return AABox();
}

void CharacterControllerCapsule::CCTSetRadius(float radius)
{
	m_desc.capsule.m_radius = radius;
	MAIN_SYSTEM_TASK_1(
		PhysicsSystem, AsyncTaskRunnerST, radius,
		{
			auto cct = (PxCapsuleController*)self->m_pxCharacterController;
			cct->setRadius(std::max(radius, 0.0f));
		}
	);
}

float CharacterControllerCapsule::CCTGetRadius() const
{
	return m_desc.capsule.m_radius;
}

void CharacterControllerCapsule::CCTSetHeight(float height)
{
	m_desc.capsule.m_height = height;
	MAIN_SYSTEM_TASK_1(
		PhysicsSystem, AsyncTaskRunnerST, height,
		{
			auto cct = (PxCapsuleController*)self->m_pxCharacterController;
			cct->setHeight(std::max(height, 0.0f));
		}
	);
}

float CharacterControllerCapsule::CCTGetHeight() const
{
	return m_desc.capsule.m_height;
}

void CharacterControllerCapsule::CCTSetClimbMode(CLIMB_MODE::ENUM mode)
{
	MAIN_SYSTEM_TASK_1(
		PhysicsSystem, AsyncTaskRunnerST, mode,
		{
			auto cct = (PxCapsuleController*)self->m_pxCharacterController;
			cct->setClimbingMode(PxCapsuleClimbingMode::Enum(mode));
		}
	);
}

CharacterControllerCapsule::CLIMB_MODE::ENUM CharacterControllerCapsule::CCTGetClimbMode() const
{
	auto cct = (PxCapsuleController*)m_pxCharacterController;
	return CLIMB_MODE::ENUM(cct->getClimbingMode());
}

void CharacterControllerCapsule::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void CharacterControllerCapsule::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void CharacterControllerCapsule::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void CharacterControllerCapsule::SerializeToJson(Serializer* serializer, json& j) const
{
	PhysicsComponent::SerializeToJson(serializer, j);

	json jDesc;
	jDesc["Capsule"] = m_desc.capsule;

	m_pDerivedDesc->ToJson(serializer, jDesc);

	j["Desc"] = jDesc;
}

void CharacterControllerCapsule::DeserializeFromJson(Serializer* serializer, const json& j)
{
	m_pDerivedDesc = &m_desc;
	PhysicsComponent::DeserializeFromJson(serializer, j);

	auto& jDesc = j["Desc"];
	m_desc.capsule = jDesc["Capsule"];
	
	m_pDerivedDesc->FromJson(serializer, jDesc);
}

Handle<ClassMetadata> CharacterControllerCapsule::GetMetadata(size_t sign)
{
	auto meta = CharacterController::GetMetadata(sign);

	{
		auto accessor = Accessor(
			"Capsule Radius",
			1,
			[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
			{
				auto obj = (CharacterControllerCapsule*)instance;
				obj->CCTSetRadius(std::max(input.As<float>(), 0.0f));
			},
			[](UnknownAddress& var, Serializable* instance) -> Variant
			{
				auto obj = (CharacterControllerCapsule*)instance;
				return Variant::Of(obj->CCTGetRadius());
			},
			this
		);
		meta->AddProperty(accessor);
	}

	{
		auto accessor = Accessor(
			"Capsule Height",
			1,
			[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
			{
				auto obj = (CharacterControllerCapsule*)instance;
				obj->CCTSetHeight(std::max(input.As<float>(), 0.0f));
			},
			[](UnknownAddress& var, Serializable* instance) -> Variant
			{
				auto obj = (CharacterControllerCapsule*)instance;
				return Variant::Of(obj->CCTGetHeight());
			},
			this
		);
		meta->AddProperty(accessor);
	}

	meta->SetName(GetClassName());
	return meta;
}

void CharacterControllerCapsule::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END