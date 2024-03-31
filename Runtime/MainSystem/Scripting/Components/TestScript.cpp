#include "TestScript.h"

#include "Input/Input.h"

#include "Runtime/Runtime.h"

#include "Core/Random/Random.h"
#include "MainSystem/Rendering/Components/Camera.h"
#include "MainSystem/Rendering/Components/CameraTPP.h"

#include "MainSystem/Physics/Components/CharacterController.h"

#include "FPPCameraScript.h"

NAMESPACE_BEGIN

void TestScript::OnStart()
{
	controller = GetGameObject()->GetComponentRaw<CharacterController>();
	m_prevPosY1 = GetGameObject()->ReadGlobalTransformMat().Position().y;
	m_prevPosY2 = m_prevPosY1;

	m_rotateX = -std::asin(m_viewPoint.y / m_viewPoint.Length());
}

void TestScript::OnUpdate(float dt)
{
	if (Input()->IsKeyUp(KEYBOARD::ESC))
	{
		std::cout << "ESC pressed\n";
		Input()->SetCursorLock(!Input()->GetCursorLock());
	}

	if (Input()->IsKeyUp('1'))
	{
		std::cout << "1 pressed\n";
		m_camera->SetTPPEnabled(!m_camera->IsTPPEnabled());
		m_fppCamScript->SetFPPScriptEnable(!m_fppCamScript->IsFPPScriptEnabled());

		if (m_fppCamScript->IsFPPScriptEnabled())
		{
			m_fppCamScript->FPPResetTransform(m_camera->GetView().GetInverse());
		}
	}

	if (!m_camera->IsTPPEnabled())
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

	Vec3 motion = { 0,0,0 };
	auto d = 15 * dt;
	if (Input()->IsKeyDown('W'))
	{
		motion += d * forward;
	}

	if (Input()->IsKeyDown('S'))
	{
		motion -= d * forward;
	}

	if (Input()->IsKeyDown('A'))
	{
		motion -= d * right;
	}

	if (Input()->IsKeyDown('D'))
	{
		motion += d * right;
	}

	static float cooldown = 0;
	if (Input()->IsKeyDown(KEYBOARD::SPACE) && cooldown <= 0 && controller->CCTIsOnGround())
	{
		//controller->CCTApplyVelocity({ 0,7,0 });
		controller->CCTApplyImpulse({ 0,120,0 });
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

	if (motion.Length2() != 0)
	{
		controller->Move(motion);
	}
}

Handle<ClassMetadata> TestScript::GetMetadata(size_t sign)
{
	auto metadata = ClassMetadata::For(this);

	return metadata;
}

void TestScript2::OnUpdate(float dt)
{
	auto transform = GetLocalTransform();
	transform.Position() = Vec3::ZERO + m_A * std::sin(m_a) * Vec3::X_AXIS;
	transform.Position().y = 2.5f;
	SetLocalTransform(transform);

	m_a += dt * m_speed;
}

Handle<ClassMetadata> TestScript2::GetMetadata(size_t sign)
{
	auto metadata = ClassMetadata::For(this);

	metadata->AddProperty(Accessor::For("A", m_A, this));
	metadata->AddProperty(Accessor::For("speed", m_speed, this));

	return metadata;
}

NAMESPACE_END