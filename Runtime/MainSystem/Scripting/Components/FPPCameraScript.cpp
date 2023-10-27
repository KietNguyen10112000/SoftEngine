#include "FPPCameraScript.h"

#include "Input/Input.h"

NAMESPACE_BEGIN

void FPPCameraScript::OnStart()
{
	auto transform = GetGameObject()->GetLocalTransform().ToTransformMatrix();

	m_position = transform.Position();
	Vec3 direction = transform.Forward().Normal();
	m_rotateX = asin(direction.y);
	m_rotateY = atan2(direction.x, direction.z);
}

void FPPCameraScript::OnUpdate(float dt)
{
	auto trans = Mat4::Identity();
	trans *= Mat4::Rotation(Vec3::Y_AXIS, m_rotateY);

	auto right = trans.Right().Normal();
	trans *= Mat4::Rotation(right, -m_rotateX);

	auto forward = trans.Forward().Normal();

	if (m_rotateZ != 0)
	{
		trans *= Mat4::Rotation(forward, m_rotateZ);
	}

	auto d = m_speed * dt;
	if (Input()->IsKeyDown('W'))
	{
		m_position += forward * d;
	}

	if (Input()->IsKeyDown('S'))
	{
		m_position -= forward * d;
	}

	if (Input()->IsKeyDown('A'))
	{
		m_position -= right * d;
	}

	if (Input()->IsKeyDown('D'))
	{
		m_position += right * d;
	}

	if (Input()->IsKeyUp(KEYBOARD::ESC))
	{
		std::cout << "ESC pressed\n";
		Input()->SetCursorLock(!Input()->GetCursorLock());
	}

	if (Input()->GetCursorLock() && Input()->IsCursorMoved())
	{
		auto& delta = Input()->GetDeltaCursorPosition();
		m_rotateY += delta.x * dt * m_rotationSensi;
		m_rotateX += delta.y * dt * m_rotationSensi;

		m_rotateX = std::max(std::min(m_rotateX, PI / 2.0f), -PI / 2.0f);
	}

	auto transform = Transform();
	transform.Position() = m_position;
	transform.Rotation().SetFromMat4(trans);
	SetLocalTransform(transform);
}

NAMESPACE_END