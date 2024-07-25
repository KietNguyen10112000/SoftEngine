#include "TPPCameraScript.h"

#include "Input/Input.h"

#include "Runtime/Runtime.h"

#include "Core/Random/Random.h"
#include "MainSystem/Rendering/Components/Camera.h"
#include "MainSystem/Rendering/Components/CameraTPP.h"

#include "MainSystem/Physics/Components/CharacterController.h"
#include "MainSystem/Physics/Components/RigidBodyDynamic.h"

#include "FPPCameraScript.h"

#include "MainSystem/Physics/PhysicsSystem.h"

NAMESPACE_BEGIN

void TPPCameraScript::OnStart()
{
	m_controller = GetGameObject()->GetComponentRaw<CharacterController>();

	m_controller->SetGravity(GetGameObject()->GetScene()->GetPhysicsSystem()->GetGravity());

	//m_prevPosY1 = GetGameObject()->ReadGlobalTransformMat().Position().y;
	//m_prevPosY2 = m_prevPosY1;

	m_rotateX = -std::asin(m_viewPoint.y / m_viewPoint.Length());

	// object with mass >= 100 doesn't react when colliding with this cct
	GetGameObject()->GetComponentRaw<CharacterController>()->CCTSetContactFilterCallback(
		[](
			GameObject* self, PhysicsShape* selfShape, PHYSICS_TYPE selfType,
			GameObject* another, PhysicsShape* anotherShape, PHYSICS_TYPE anotherType,
			size_t& pairFlags
		) {
			pairFlags = PhysicsCollisionPairFlag::DETECT_DISCRETE_CONTACT
				| PhysicsCollisionPairFlag::NOTIFY_TOUCH_FOUND
				| PhysicsCollisionPairFlag::NOTIFY_TOUCH_LOST
				| PhysicsCollisionPairFlag::NOTIFY_TOUCH_PERSISTS
				| PhysicsCollisionPairFlag::NOTIFY_CONTACT_POINTS;

			if (anotherType != PHYSICS_TYPE::PHYSICS_TYPE_RIGID_BODY_DYNAMIC || another->GetComponentRaw<RigidBodyDynamic>()->GetMass() < 100)
			{
				// pass this contact to narrow phase
				pairFlags |= PhysicsCollisionPairFlag::SOLVE_CONTACT;
			}
		}
	);
}

void TPPCameraScript::OnUpdate(float dt)
{
	if (!m_camera)
	{
		return;
	}

	if (Input()->IsKeyUp('R'))
	{
		Runtime::Get()->HotReloadScripts();
	}

	if (Input()->IsKeyUp('O') && m_testBody)
	{
		m_testBody->AddForceAtLocalPos({ 0,0,10000 }, { 2.5f,0,0 });
	}

	if (Input()->IsKeyUp('P') && m_testBody)
	{
		m_testBody->AddForceAtLocalPos({ 0,0,-10000 }, { 2.5f,0,0 });
	}

	if (m_camera && m_fppCamScript && Input()->IsKeyUp('V'))
	{
		m_camera->SetTPPEnabled(!m_camera->IsTPPEnabled());
		m_fppCamScript->SetFPPScriptEnable(!m_fppCamScript->IsFPPScriptEnabled());

		if (m_fppCamScript->IsFPPScriptEnabled())
		{
			m_fppCamScript->FPPResetTransform(m_camera->GetView().GetInverse());
		}
	}

	if (m_camera && !m_camera->IsTPPEnabled())
	{
		return;
	}

	if (!Input()->GetCursorLock())
	{
		return;
	}

	float rotateX = 0, rotateY = 0;
	float oriRotateX = m_rotateX;
	if (Input()->IsCursorMoved())
	{
		auto& delta = Input()->GetDeltaCursorPosition();
		rotateY += delta.x * dt * m_rotationSensi;
		rotateX -= delta.y * dt * m_rotationSensi;

		m_rotateX += rotateX;
		m_rotateX = std::max(std::min(m_rotateX, PI / 2.0f - 0.01f), -PI / 2.0f + 0.01f);
		rotateX = m_rotateX - oriRotateX;
	}

	auto trans = Mat4::Identity();
	trans *= Mat4::Rotation(Vec3::Y_AXIS, rotateY);

	auto right = Vec3::UP.Cross(-m_viewPoint.Normal());
	trans *= Mat4::Rotation(right, -rotateX);

	m_viewPoint = (Vec4(m_viewPoint, 1.0f) * trans).xyz();

	if (m_camera)
		m_camera->SetViewPoint(m_viewPoint);

	Vec3 forward = right.Cross(Vec3::UP);

	forward.Normalize();
	right.Normalize();

	Vec3 motion = { 0,0,0 };
	if (Input()->IsKeyDown('W'))
	{
		motion += forward;
	}

	if (Input()->IsKeyDown('S'))
	{
		motion -= forward;
	}

	if (Input()->IsKeyDown('A'))
	{
		motion -= right;
	}

	if (Input()->IsKeyDown('D'))
	{
		motion += right;
	}

	if (motion != Vec3::ZERO)
	{
		motion = m_movingSpeed * dt * motion.Normal();
	}

	static float cooldown = 0;
	if (Input()->IsKeyDown(KEYBOARD::SPACE) && cooldown <= 0 && m_controller->CCTIsOnGround())
	{
		//controller->CCTApplyVelocity({ 0,7,0 });
		m_controller->CCTApplyImpulse({ 0,120,0 });
		cooldown = 1;

		//std::cout << "Jumped\n";
	}

	if ((cooldown -= dt) <= 0)
	{
		cooldown = 0;
	}

	//bool jumped = false;
	//if (Input()->IsKeyDown(KEYBOARD::SPACE))
	//{
	//	m_motionY = 0.5f;
	//	jumped = true;
	//}

	//if (m_motionY > -900)
	//	m_motionY -= 9.81f * dt;

	//auto& globalPosition = GetGameObject()->ReadGlobalTransformMat().Position();

	///*if (!jumped)
	//{
	//	if (m_prevPosY == globalPosition.y)
	//	{
	//		m_motionY = 0;
	//	}
	//}*/
	//if (!jumped)
	//{
	//	if (m_prevPosY1 == globalPosition.y && m_prevPosY1 == m_prevPosY2)
	//	{
	//		m_motionY = 0;
	//	}
	//}

	//m_prevPosY2 = m_prevPosY1;
	//m_prevPosY1 = globalPosition.y;

	//motion.y = m_motionY;

	m_lastMotion = motion;
	if (motion.Length2() != 0)
	{
		m_controller->Move(motion);
	}
}

Handle<ClassMetadata> TPPCameraScript::GetMetadata(size_t sign)
{
	auto metadata = ClassMetadata::For(this);

	return metadata;
}

void TPPCameraScript::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void TPPCameraScript::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void TPPCameraScript::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void TPPCameraScript::SerializeToJson(Serializer* serializer, json& j) const
{
	j["CameraTPP"] = serializer->Serialize(m_camera);
	j["FPPCamScript"] = serializer->Serialize(m_fppCamScript);
}

void TPPCameraScript::DeserializeFromJson(Serializer* serializer, const json& j)
{
	serializer->Deserialize(j["CameraTPP"], m_camera);
	serializer->Deserialize(j["FPPCamScript"], m_fppCamScript);
}

Vec3 TPPCameraScript::GetForwardToCCT() const
{
	auto right = Vec3::UP.Cross(-m_viewPoint.Normal());
	Vec3 forward = right.Cross(Vec3::UP).Normalize();
	return forward;
}

//void TestScript2::OnUpdate(float dt)
//{
//	auto transform = GetLocalTransform();
//	transform.Position() = Vec3::ZERO + m_A * std::sin(m_a) * Vec3::X_AXIS;
//	transform.Position().y = 2.5f;
//	SetLocalTransform(transform);
//
//	m_a += dt * m_speed;
//}
//
//Handle<ClassMetadata> TestScript2::GetMetadata(size_t sign)
//{
//	auto metadata = ClassMetadata::For(this);
//
//	metadata->AddProperty(Accessor::For("A", m_A, this));
//	metadata->AddProperty(Accessor::For("speed", m_speed, this));
//
//	return metadata;
//}

NAMESPACE_END