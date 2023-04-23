#pragma once

#include "../Script.h"

#include "Input/Input.h"

#include "Components/Rendering/Camera.h"

#include "imgui/imgui.h"

NAMESPACE_BEGIN

class FPPCameraScript : public Script
{
public:
	float m_rotateX = 0;
	float m_rotateY = 0;
	float m_rotateZ = 0;
	Vec3 m_position = {};

	float m_speed = 10;
	float m_rotationSensi = 0.12f;

	Mat4 m_mat = Mat4::Identity();

	~FPPCameraScript()
	{
		std::cout << "FPPCameraScript::~FPPCameraScript()\n";
	}

	virtual void OnStart() override
	{
		auto transform = GetObject()->GetTransformMat4();

		m_position = transform.Position();
		Vec3 direction = transform.Forward().Normal();
		m_rotateX = asin(direction.y);
		m_rotateY = atan2(direction.x, direction.z);
	}

	virtual void OnUpdate(float dt) override
	{
		auto& transform = TransformMat4();
		auto trans = Mat4::Identity();
		trans *= Mat4::Rotation(Vec3::Y_AXIS, m_rotateY);

		auto right = trans.Right().Normal();
		trans *= Mat4::Rotation(right, -m_rotateX);

		auto forward = trans.Forward().Normal();

		if (m_rotateZ != 0)
		{
			trans *= Mat4::Rotation(forward, m_rotateZ);
		}

		trans *= Mat4::Translation(m_position);
		transform = trans;

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

		if (Input()->IsKeyPressed(KEYBOARD::ESC))
		{
			Input()->SetCursorLock(!Input()->GetCursorLock());
		}

		if (Input()->GetCursorLock() && Input()->IsCursorMoved())
		{
			auto& delta = Input()->GetDeltaCursorPosition();
			m_rotateY += delta.x * dt * m_rotationSensi;
			m_rotateX -= delta.y * dt * m_rotationSensi;

			m_rotateX = std::max(std::min(m_rotateX, PI / 2.0f), -PI / 2.0f);
		}
	}

	virtual void OnGUI() override
	{
		/*m_mat *= Mat4::Rotation(Vec3::UP, 0.016f * PI / 3.0f);
		auto dbg = Graphics::Get()->GetDebugGraphics();
		dbg->DrawCube(Mat4::Scaling(2, 2, 2) * m_mat * Mat4::Translation(0, 10, 0), { 0, 1, 1, 1 });*/

		ImGui::Begin("FPPCamera");
		ImGui::Text("Press ESC to use camera");
		ImGui::Text("Num draw calls: %d", GetObject()->GetComponentRaw<Camera>()->NumRenderObjects());
		ImGui::DragFloat("Rotation sensity", &m_rotationSensi, 0.01f, 0, FLT_MAX);
		ImGui::DragFloat("Moving speed", &m_speed, 0.01f, 0, FLT_MAX);
		ImGui::End();
	}

};

NAMESPACE_END