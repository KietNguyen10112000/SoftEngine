#include "TestScript.h"

#include "Input/Input.h"

#include "Runtime/Runtime.h"

#include "Core/Random/Random.h"
#include "MainSystem/Rendering/Components/Camera.h"

#include "MainSystem/Physics/Components/CharacterController.h"

NAMESPACE_BEGIN

void TestScript::OnStart()
{
	controller = GetGameObject()->GetComponentRaw<CharacterController>();
	m_prevPosY1 = GetGameObject()->ReadGlobalTransformMat().Position().y;
	m_prevPosY2 = m_prevPosY1;
}

void TestScript::OnUpdate(float dt)
{
	if (!Input()->GetCursorLock())
	{
		return;
	}

	Vec3 motion = { 0,0,0 };
	auto d = 15 * dt;
	if (Input()->IsKeyDown(KEYBOARD::UP))
	{
		motion.x += -d;
	}

	if (Input()->IsKeyDown(KEYBOARD::DOWN))
	{
		motion.x += d;
	}

	if (Input()->IsKeyDown(KEYBOARD::LEFT))
	{
		motion.z += -d;
	}

	if (Input()->IsKeyDown(KEYBOARD::RIGHT))
	{
		motion.z += d;
	}

	bool jumped = false;
	if (Input()->IsKeyDown(KEYBOARD::SPACE))
	{
		m_motionY = 0.2f;
		jumped = true;
	}

	if (m_motionY > -900)
		m_motionY -= 9.81f * dt;

	auto& globalPosition = GetGameObject()->ReadGlobalTransformMat().Position();

	/*if (!jumped)
	{
		if (m_prevPosY == globalPosition.y)
		{
			m_motionY = 0;
		}
	}*/
	if (!jumped)
	{
		if (m_prevPosY1 == globalPosition.y && m_prevPosY1 == m_prevPosY2)
		{
			m_motionY = 0;
		}
	}

	m_prevPosY2 = m_prevPosY1;
	m_prevPosY1 = globalPosition.y;

	motion.y = m_motionY;

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