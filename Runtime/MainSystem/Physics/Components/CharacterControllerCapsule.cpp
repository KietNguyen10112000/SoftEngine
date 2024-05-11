#include "CharacterControllerCapsule.h"

#include "PhysX/PhysX.h"
#include "MainSystem/Physics/PhysicsSystem.h"

#include "Graphics/DebugGraphics.h"
#include "Graphics/Graphics.h"

#include "../FILTER_DATA.h"

#include "../Shapes/PhysicsShapeCapsule.h"

#include "Scene/GameObject.h"

using namespace physx;

NAMESPACE_BEGIN

extern void* g_defaultPxControllerHitCallbackPtr;

CharacterControllerCapsule::CharacterControllerCapsule(Scene* scene, const CharacterControllerCapsuleDesc& desc)
{
	PxCapsuleControllerDesc pxDesc = {};
	pxDesc.height = desc.capsule.m_height;
	pxDesc.radius = desc.capsule.m_radius;
	pxDesc.upDirection = reinterpret_cast<const PxVec3&>(desc.capsule.m_up);
	pxDesc.position = PxExtendedVec3(desc.capsule.m_center.x, desc.capsule.m_center.y, desc.capsule.m_center.z);
	pxDesc.material = desc.material->m_pxMaterial;
	//pxDesc.reportCallback = (decltype(pxDesc.reportCallback))g_defaultPxControllerHitCallbackPtr;
	pxDesc.scaleCoeff = 1.0f;
	pxDesc.contactOffset = 0.01f;

	m_pxCharacterController = scene->GetPhysicsSystem()->m_pxControllerManager->createController(pxDesc);

	auto pxActor = m_pxCharacterController->getActor();
	pxActor->userData = this;

	PxShape* shape = nullptr;
	pxActor->getShapes(&shape, 1);

	m_shape = PhysicsShapeCapsule::MakeDummy(shape, desc.capsule.m_height, desc.capsule.m_radius, desc.material, true);

	if (shape)
	{
		PxFilterData data;
		data.word0 = PHYSICS_FILTER_DATA_CCT;
		shape->setSimulationFilterData(data);

		shape->setGeometry(PxCapsuleGeometry(pxDesc.radius + 2.0f * pxDesc.contactOffset + 0.05f, pxDesc.height / 2.0f + 2.0f * pxDesc.contactOffset + 0.05f));
	}

	auto mass = m_pxCharacterController->getActor()->getMass();
	mass = mass <= 0 ? 1 : mass;
	m_mass = mass;
}

void CharacterControllerCapsule::OnDrawDebug()
{
	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (debugGraphics)
	{
		auto pxController = (PxCapsuleController*)m_pxCharacterController;

		auto& pos = GetGameObject()->ReadGlobalTransformMat().Position();//pxController->getPosition();

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
}

void CharacterControllerCapsule::OnComponentRemoved()
{
}

AABox CharacterControllerCapsule::GetGlobalAABB()
{
	return AABox();
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
}

void CharacterControllerCapsule::DeserializeFromJson(Serializer* serializer, const json& j)
{
}

Handle<ClassMetadata> CharacterControllerCapsule::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void CharacterControllerCapsule::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END