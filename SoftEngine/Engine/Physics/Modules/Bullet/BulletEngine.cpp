#include "BulletEngine.h"

#include "BulletPhysicsObject.h"

#include "BulletHelper.h"

//std::shared_ptr<btCollisionShape> g_sphereShape;
//std::shared_ptr<btCollisionShape> g_boxShape;
//std::shared_ptr<btCollisionShape> g_capsuleShape;

BulletEngine::BulletEngine()
{
	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);

	m_broadphase = new btDbvtBroadphase();

	m_solver = new btSequentialImpulseConstraintSolver;

	m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfiguration);

	m_dynamicsWorld->setGravity(btVector3(0, -10.0f, 0));


	/*g_sphereShape = std::make_shared<btSphereShape>(1.0f);
	g_boxShape = std::make_shared<btBoxShape>(btVector3(1.0f, 1.0f, 1.0f));
	g_capsuleShape = std::make_shared<btCapsuleShape>(1.0f, 1.0f);*/
}

BulletEngine::~BulletEngine()
{
	/*g_sphereShape.reset();
	g_boxShape.reset();
	g_capsuleShape.reset();*/
}

PhysicsObject* BulletEngine::MakeObject(const Transform& transform, const Shape& shape, const PhysicsMaterial& material)
{
	BulletPhysicsObject* obj = new BulletPhysicsObject();

	auto& btShape = obj->m_shape;
	auto& body = obj->m_object;

	switch (shape.Type())
	{
	case Shape::SPHERE:
		btShape = std::make_shared<btSphereShape>(shape.As<SphereShape>().Radius());
		//btShape = g_sphereShape;
		break;
	case Shape::CAPSULE:
		btShape = std::make_shared<btCapsuleShape>(
			shape.As<CapsuleShape>().Radius(), 
			shape.As<CapsuleShape>().Height()
		);
		//btShape = g_capsuleShape;
		break;
	case Shape::BOX:
		btShape = std::make_shared<btBoxShape>(
			btVector3(
				shape.As<BoxShape>().GetWidth() / 2.0f,
				shape.As<BoxShape>().GetHeight() / 2.0f,
				shape.As<BoxShape>().GetLength() / 2.0f
			)
		);
		//btShape = g_boxShape;
		break;
	default:
		break;
	}

	btTransform btTrans;
	btTrans.getBasis().setRotation(*(btQuaternion*)&transform.Rotation());
	btTrans.setOrigin(BulletHelper::Vec3TobtVector3(transform.Position()));

	btShape->setLocalScaling(BulletHelper::Vec3TobtVector3(transform.m_scale));
	obj->m_scale = transform.m_scale;

	if (material.mass != 0)
	{
		auto& mass = material.mass;

		// dynamic
		btVector3 localInertia(0, 0, 0);
		btShape->calculateLocalInertia(mass, localInertia);
		btDefaultMotionState* myMotionState = new btDefaultMotionState(btTrans);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, btShape.get(), localInertia);


		rbInfo.m_friction = material.friction;
		rbInfo.m_restitution = material.restitution;

		body = new btRigidBody(rbInfo);

		obj->m_dynamic = 1;
	}
	else
	{
		// static
		body = new btCollisionObject();
		body->setCollisionShape(btShape.get());
		body->setCollisionFlags(btCollisionObject::CollisionFlags::CF_STATIC_OBJECT);
		body->setWorldTransform(btTrans);

		body->setFriction(material.friction);
		body->setRestitution(material.restitution);

		obj->m_dynamic = 0;
	}

	return obj;
}

PhysicsObject* BulletEngine::MakeObject(const Transform& transform, const CompoundShape& shape, const CompoundPhysicsMaterial& material)
{
	return nullptr;
}

void BulletEngine::StepSimulation(
	float deltaTime, 
	std::vector<PhysicsObject*>* dynamicObjectsAdd,
	std::vector<PhysicsObject*>* dynamicObjectsRemove)
{
	m_dynamicsWorld->stepSimulation(deltaTime);
}

void BulletEngine::SetGravity(const Vec3& g)
{
	m_dynamicsWorld->setGravity(btVector3(g.x, g.y, g.z));
}

Vec3 BulletEngine::GetGravity()
{
	auto g = m_dynamicsWorld->getGravity();
	return Vec3(g.x(), g.y(), g.z());
}

void BulletEngine::AddObjects(std::vector<PhysicsObject*>& objects)
{
	for (auto& obj : objects)
	{
		auto* btObj = (BulletPhysicsObject*)obj;

		if (btObj->IsDynamic())
		{
			m_dynamicsWorld->addRigidBody(btRigidBody::upcast(btObj->m_object));
		}
		else
		{
			m_dynamicsWorld->addCollisionObject(btObj->m_object);
		}
	}
}

void BulletEngine::RemoveObjects(std::vector<PhysicsObject*>& objects)
{
	for (auto& obj : objects)
	{

	}
}
