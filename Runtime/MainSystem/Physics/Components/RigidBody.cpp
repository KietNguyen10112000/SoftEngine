#include "RigidBody.h"

#include "Scene/GameObject.h"

#include "PxPhysicsAPI.h"

#include "Graphics/Graphics.h"
#include "Graphics/DebugGraphics.h"

#include "MainSystem/MainSystemTaskPacking.h"
#include "MainSystem/Physics/PhysicsSystem.h"

#include "../Shapes/PhysicsShape.h"
#include "../FILTER_FLAG.h"
#include "../Joints/Joint.h"

#include "RigidBodyDynamic.h"

using namespace physx;

NAMESPACE_BEGIN

void RigidBody::SetupCollisionStruct()
{
	if (!GetGameObject() || !GetGameObject()->GetScene())
	{
		return;
	}

	auto collisionResult = new PhysicsCollisionResult();
	auto collision = (Collision*)collisionResult->collision.Read();

	auto pxScene = GetGameObject()->GetScene()->GetPhysicsSystem()->m_pxScene;
	auto body = m_pxActor->is<PxRigidActor>();
	auto globalPose = body->getGlobalPose();
	for (auto& shape : m_shapes)
	{
		auto& pxShape = shape->m_pxShape;

		PxOverlapBuffer buffer;
		auto& overlapShape = pxShape->getGeometry();
		PxTransform shapePose = pxShape->getLocalPose().transform(globalPose);

		if (pxScene->overlap(overlapShape, shapePose, buffer))
		{
			for (size_t i = 0; i < buffer.nbTouches; i++)
			{
				auto& hit = buffer.touches[i];
				auto anotherPhysicsComp = (PhysicsComponent*)hit.actor->userData;

				if (anotherPhysicsComp->m_collisionResult)
				{
					auto anotherCollision = anotherPhysicsComp->m_collisionResult->collision.Read();
					for (auto& c : anotherCollision->contacts)
					{
						auto AComp = c->A->GetComponentRaw<PhysicsComponent>();
						auto BComp = c->B->GetComponentRaw<PhysicsComponent>();
						if ((AComp == anotherPhysicsComp || BComp == anotherPhysicsComp)
							&& (AComp == this || BComp == this))
						{
							collision->contacts.push_back(c);
						}
					}
				}
			}
		}
	}

	collision->UpdateContactCount();

	if (m_collisionResult)
	{
		delete m_collisionResult;
	}

	m_collisionResult = collisionResult;
}

void RigidBody::OnTransformChanged()
{
	auto gameObject = GetGameObject();

	auto& globalTransform = gameObject->GetCommittedGlobalTransform();

	//auto pxRigidBody = m_pxActor->is<PxRigidBody>();

	//assert(pxRigidBody && "something wrong here!");

	PxRigidActor* pxRigidBody = m_pxActor->is<PxRigidActor>();

	//auto pxTransform = pxRigidBody->getGlobalPose();

	//if (::memcmp(&m_lastGlobalTransform, &globalTransform, sizeof(Mat4)) != 0)
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

		//m_lastGlobalTransform = globalTransform;
	}
}

void RigidBody::OnDrawDebug()
{
	return;

	auto debugGraphics = Graphics::Get()->GetDebugGraphics();

	if (!debugGraphics)
	{
		return;
	}

	PxRigidActor* pxRigidBody = m_pxActor->is<PxRigidActor>();

	auto& globalMat = GetGameObject()->GetCommittedGlobalTransform();

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

void RigidBody::OnPhysicsFlagSetted(PHYSICS_FLAG flag, bool value)
{
	switch (flag)
	{
	case PHYSICS_FLAG_COLLISION_RESULT:
	{
		if (value)
		{
			SetupCollisionStruct();
		}
		break;
	}
	default:
		break;
	}
}

void RigidBody::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void RigidBody::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void RigidBody::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void RigidBody::SerializeToJson(Serializer* serializer, json& j) const
{
	PhysicsComponent::SerializeToJson(serializer, j);

	{
		auto arr = json::array();
		for (auto& shape : m_shapes)
		{
			arr.push_back(serializer->Serialize(shape));
		}
		j["Shapes"] = arr;
	}

	{
		auto arr = json::array();
		for (auto& joint : m_joints)
		{
			arr.push_back(serializer->Serialize(joint));
		}
		j["Joints"] = arr;
	}
}

void RigidBody::DeserializeFromJson(Serializer* serializer, const json& j)
{
	if (m_shapes.size() == 0)
	{
		PhysicsComponent::DeserializeFromJson(serializer, j);

		auto& arr = j["Shapes"];
		for (size_t i = 0; i < arr.size(); i++)
		{
			SharedPtr<PhysicsShape> shape;
			serializer->Deserialize(arr[i], shape);
			m_shapes.push_back(shape);
			//AddShapeImpl(shape);
		}
	}

	if (m_pxActor && m_joints.size() == 0)
	{
		auto& arr = j["Joints"];
		Handle<Joint> joint;
		for (size_t i = 0; i < arr.size(); i++)
		{
			serializer->Deserialize(arr[i], joint);
			/*auto& idx = joint->m_body0 == this ? joint->m_idx0 : joint->m_idx1;
			idx = m_joints.size();
			m_joints.Push(joint);*/
		}
	}
}

Handle<ClassMetadata> RigidBody::GetMetadata(size_t sign)
{
	auto metadata = mheap::New<ClassMetadata>("RigidBody", this);

	auto shapesMetadata = mheap::New<ClassMetadata>("ShapesArray", this);
	shapesMetadata->AddProperty(
		Accessor(
			"count", this, nullptr,
			[](UnknownAddress& var, Serializable* instance) -> Variant
			{
				auto* self = (RigidBody*)instance;
				return Variant::Of(self->GetShapesCount());
			},
			this
		)
	);
	metadata->AddProperty("Shapes", shapesMetadata);
	for (size_t i = 0; i < m_shapes.size(); i++)
	{
		auto& shape = m_shapes[i];
		shapesMetadata->AddProperty(String::From(i).c_str(), shape->GetMetadata(sign + 1));
	}

	return metadata;
}

void RigidBody::AddShapeImpl(const SharedPtr<PhysicsShape>& shape)
{
	auto body = m_pxActor->is<PxRigidActor>();
	body->attachShape(*shape->m_pxShape);
	m_shapes.push_back(shape);

	if (shape->m_attachedRigidBodyCount++ == 0)
	{
		shape->m_attachedRigidBody = this;
	}
	else
	{
		shape->m_attachedRigidBody = nullptr;
	}

	auto dynamic = body->is<PxRigidDynamic>();
	if (dynamic)
	{
		auto comp = (RigidBodyDynamic*)this;
		PxRigidBodyExt::updateMassAndInertia(*dynamic, comp->GetDensity());
		if (!comp->IsKinematic() && dynamic->getScene() && dynamic->isSleeping()) dynamic->wakeUp();
	}
}

void RigidBody::RemoveShapeImpl(PhysicsShape* shape)
{
	auto body = m_pxActor->is<PxRigidActor>();
	body->detachShape(*shape->m_pxShape);

	if (--(shape->m_attachedRigidBodyCount) == 0)
	{
		shape->m_attachedRigidBody = nullptr;
	}
	m_shapes.erase(std::remove_if(m_shapes.begin(), m_shapes.end(),
		[shape](const SharedPtr<PhysicsShape>& s)
		{
			auto v= s.get() == shape;
			return v;
		}
	));

	auto dynamic = body->is<PxRigidDynamic>();
	if (dynamic)
	{
		auto comp = (RigidBodyDynamic*)this;
		PxRigidBodyExt::updateMassAndInertia(*dynamic, comp->GetDensity());
		if (!comp->IsKinematic() && dynamic->getScene() && dynamic->isSleeping()) dynamic->wakeUp();
	}
}

void RigidBody::SetContactFilterCallback(ContactReportFilterCallback callback)
{
	m_contactFilterCallback = callback;

	MAIN_SYSTEM_TASK_0(
		PhysicsSystem, AsyncTaskRunnerST,
		{
			for (auto& shape : self->m_shapes)
			{
				PxFilterData data = shape->m_pxShape->getSimulationFilterData();
				if (self->m_contactFilterCallback)
				{
					data.word0 |= PHYSICS_FILTER_FLAG::CALLBACK;
				}
				else
				{
					data.word0 &= ~PHYSICS_FILTER_FLAG::CALLBACK;
				}
				shape->m_pxShape->setSimulationFilterData(data);
			}
		}
	);
}

void RigidBody::AddShape(const SharedPtr<PhysicsShape>& shape)
{
	MAIN_SYSTEM_TASK_COMMON_1(PhysicsSystem, AsyncTaskRunnerST, shape,
		{
			self->AddShapeImpl(shape);
		}
	);
}

void RigidBody::RemoveShape(PhysicsShape* shape)
{
	MAIN_SYSTEM_TASK_COMMON_1(PhysicsSystem, AsyncTaskRunnerST, shape,
		{
			self->RemoveShapeImpl(shape);
		}
	);
}

void RigidBody::ScaleBy(float scale)
{
	for (auto& shape : m_shapes)
	{
		shape->ScaleBy(scale);
		auto transform = shape->GetLocalTransform();
		transform.Position() *= scale;
		shape->SetLocalTransform(transform);
	}
}

void RigidBody::SetCollisionMaskForAllShapes(uint32_t mask)
{
	for (auto& shape : m_shapes)
	{
		shape->SetCollisionMask(mask);
	}
}

void RigidBody::SetFamilyNoCollideForAllShapes(bool enable)
{
	for (auto& shape : m_shapes)
	{
		shape->SetFamilyNoCollide(enable);
	}
}

void RigidBody::SetCollisionMaskForGameObject(GameObject* obj, uint32_t mask)
{
	obj->PostTraversal(
		[mask](GameObject* o)
		{
			if (o->HasComponent<RigidBody>())
			{
				o->GetComponentRaw<RigidBody>()->SetCollisionMaskForAllShapes(mask);
			}
		}
	);
}

void RigidBody::SetFamilyNoCollideForGameObject(GameObject* obj, bool enable)
{
	obj->PostTraversal(
		[enable](GameObject* o)
		{
			if (o->HasComponent<RigidBody>())
			{
				o->GetComponentRaw<RigidBody>()->SetFamilyNoCollideForAllShapes(enable);
			}
		}
	);
}

NAMESPACE_END