#include "FPPCameraScript.h"

#include "Input/Input.h"

#include "Runtime/Runtime.h"

#include "Core/Random/Random.h"
#include "MainSystem/Rendering/Components/Camera.h"
#include "MainSystem/Rendering/Components/Model3DBasicRenderer.h"

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

	//static size_t count = 0;
	//if ((count++) % (60 * 5) == 0)
	if (Input()->IsKeyUp('K'))
	{
		auto runtime = Runtime::Get();
		auto scene = runtime->CreateScene();

		Transform transform = {};

		auto cameraObj = mheap::New<GameObject>();
		cameraObj->NewComponent<FPPCameraScript>();
		auto camera = cameraObj->NewComponent<Camera>();
		camera->Projection().SetPerspectiveFovLH(
			PI / 3.0f,
			Input()->GetClientWidth() / (float)Input()->GetClientHeight(),
			0.5f,
			1000.0f
		);
		scene->AddObject(cameraObj);

		Vec3 axises[] = {Vec3::X_AXIS, Vec3::Y_AXIS, Vec3::Z_AXIS};
		auto num = Random::RangeInt64(10, 20);
		for (size_t i = 0; i < num; i++)
		{
			transform = {};
			transform.Position() = {
				Random::RangeFloat(-20, 20),
				Random::RangeFloat(-20, 20),
				Random::RangeFloat(-20, 20) 
			};

			transform.Scale() = {
				Random::RangeFloat(0.5, 5),
				Random::RangeFloat(0.5, 5),
				Random::RangeFloat(0.5, 5)
			};

			Mat4 mat =
				Mat4::Rotation(axises[Random::RangeInt32(0, 2)], Random::RangeFloat(0, 2 * PI))
				* Mat4::Rotation(axises[Random::RangeInt32(0, 2)], Random::RangeFloat(0, 2 * PI))
				* Mat4::Rotation(axises[Random::RangeInt32(0, 2)], Random::RangeFloat(0, 2 * PI));

			transform.Rotation() = { mat };

			auto object = mheap::New<GameObject>();
			object->SetLocalTransform(transform);
			object->NewComponent<Model3DBasicRenderer>("cube.obj", "2.png");
			scene->AddObject(object);
		}

		runtime->DestroyScene(runtime->GetCurrentRunningScene().Get());
		runtime->SetRunningScene(scene);
	}

	auto transform = Transform();
	transform.Position() = m_position;
	transform.Rotation().SetFromMat4(trans);
	SetLocalTransform(transform);
}

NAMESPACE_END