#include "RigidBody.h"

#include "Scene/GameObject.h"

#include "PxPhysicsAPI.h"

#include "Graphics/Graphics.h"
#include "Graphics/DebugGraphics.h"

using namespace physx;

NAMESPACE_BEGIN

void RigidBody::OnTransformChanged()
{
	auto gameObject = GetGameObject();

	auto& globalTransform = gameObject->ReadGlobalTransformMat();

	//auto pxRigidBody = m_pxActor->is<PxRigidBody>();

	//assert(pxRigidBody && "something wrong here!");

	PxRigidActor* pxRigidBody = m_pxActor->is<PxRigidActor>();

	//auto pxTransform = pxRigidBody->getGlobalPose();

	if (::memcmp(&m_lastGlobalTransform, &globalTransform, sizeof(Mat4)) != 0)
	{
		Vec3 scale;
		Vec3 pos;
		Quaternion rot;
		globalTransform.Decompose(scale, rot, pos);

		PxTransform pxTransform;
		pxTransform.p = reinterpret_cast<PxVec3&>(pos);
		pxTransform.q.x = rot.x;
		pxTransform.q.y = rot.y;
		pxTransform.q.z = rot.z;
		pxTransform.q.w = rot.w;

		pxRigidBody->setGlobalPose(pxTransform);

		m_lastGlobalTransform = globalTransform;
	}
}

void RigidBody::OnDrawDebug()
{
	auto debugGraphics = Graphics::Get()->GetDebugGraphics();

	if (!debugGraphics)
	{
		return;
	}

	PxRigidActor* pxRigidBody = m_pxActor->is<PxRigidActor>();

	auto& globalMat = GetGameObject()->ReadGlobalTransformMat();

	Transform transform = {};
	globalMat.Decompose(transform.Scale(), transform.Rotation(), transform.Position());
	//auto pos = reinterpret_cast<const Vec3&>(transform.p);

	Vec4 color = Vec4(0,0,0,1);
	if (m_collisionResult)
	{
		auto size = m_collisionResult->GetContactPairsCount();
		if (size == 1)
		{
			color = { 1,0,0,1 };
		}

		if (size == 2)
		{
			color = { 1,1,0,1 };
		}
	}

	PxShape* shapes[128] = {};
	auto numShape = pxRigidBody->getShapes(shapes, 128, 0);
	for (size_t i = 0; i < numShape; i++)
	{
		auto shape = shapes[i];
		auto& geometry = shape->getGeometry();
		auto type = geometry.getType();
		switch (type)
		{
		case physx::PxGeometryType::eSPHERE: 
		{
			auto* sphere = (PxSphereGeometry*)&geometry;
			debugGraphics->DrawSphere(Sphere(transform.Position(), sphere->radius), color);
			break;
		}
		case physx::PxGeometryType::ePLANE:
			break;
		case physx::PxGeometryType::eCAPSULE:
			break;
		case physx::PxGeometryType::eBOX:
		{
			auto* box = (PxBoxGeometry*)&geometry;

			/*Transform _transform = {};
			_transform.Position() = pos;
			_transform.Rotation().x = transform.q.x;
			_transform.Rotation().y = transform.q.y;
			_transform.Rotation().z = transform.q.z;
			_transform.Rotation().w = transform.q.w;
			_transform.Scale() = Vec3(box->halfExtents.x, box->halfExtents.y, box->halfExtents.z);
			_transform.Scale() += 0.01f;*/

			Transform _transform = transform;
			_transform.Scale() += 0.1f;

			debugGraphics->DrawCube(_transform.ToTransformMatrix(), color);
			break;
		}
		case physx::PxGeometryType::eCONVEXMESH:
			break;
		case physx::PxGeometryType::ePARTICLESYSTEM:
			break;
		case physx::PxGeometryType::eTETRAHEDRONMESH:
			break;
		case physx::PxGeometryType::eTRIANGLEMESH:
			break;
		case physx::PxGeometryType::eHEIGHTFIELD:
			break;
		case physx::PxGeometryType::eHAIRSYSTEM:
			break;
		case physx::PxGeometryType::eCUSTOM:
			break;
		case physx::PxGeometryType::eGEOMETRY_COUNT:
			break;
		case physx::PxGeometryType::eINVALID:
			break;
		default:
			break;
		}
	}
}

NAMESPACE_END