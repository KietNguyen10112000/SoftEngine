#include "CharacterControllerCapsule.h"

#include "PhysX/PhysX.h"
#include "MainSystem/Physics/PhysicsSystem.h"

#include "Graphics/DebugGraphics.h"
#include "Graphics/Graphics.h"

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
	pxDesc.reportCallback = (decltype(pxDesc.reportCallback))g_defaultPxControllerHitCallbackPtr;
	pxDesc.scaleCoeff = 1.0f;
	pxDesc.contactOffset = 0.00001f;

	m_pxCharacterController = scene->GetPhysicsSystem()->m_pxControllerManager->createController(pxDesc);

	m_pxCharacterController->getActor()->userData = this;
}

void CharacterControllerCapsule::OnDrawDebug()
{
	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (debugGraphics)
	{
		auto pxController = (PxCapsuleController*)m_pxCharacterController;

		auto& pos = pxController->getPosition();

		Vec4 color = Vec4(0, 0, 0, 1);
		if (m_collisionResult)
		{
			auto size = m_collisionResult->collision.Read()->contactPairs.size();
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

void CharacterControllerCapsule::Serialize(Serializer* serializer)
{
}

void CharacterControllerCapsule::Deserialize(Serializer* serializer)
{
}

void CharacterControllerCapsule::CleanUp()
{
}

Handle<ClassMetadata> CharacterControllerCapsule::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void CharacterControllerCapsule::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
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

NAMESPACE_END