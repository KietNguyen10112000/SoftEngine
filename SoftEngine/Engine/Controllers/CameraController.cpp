#include "CameraController.h"

#include "Engine/Engine.h"

//#include "Engine/Global.h"

#include "Engine/Scene/Scene.h"

CameraController::CameraController(const Vec3& position, const Vec3& focusPos, 
	float fov, float dNear, float dFar, float aspectRatio, float speed, float mouseSensi)
{
	m_proj.SetPerspectiveFovLH(fov, dNear, dFar, aspectRatio);

	m_transform.SetLookAtLH(position, focusPos, { 0,1,0 });

	//m_mvp = m_transform * m_proj;

	m_position = position;

	m_speed = speed;
	m_rotationSensi = mouseSensi;

	Vec3 direction = (focusPos - position).Normalize();
	m_rotateX = asin(direction.y);
	m_rotateY = atan2(direction.x, direction.z);
}

CameraController::~CameraController()
{
}

bool CameraController::UpdateControl(Engine* engine)
{
	static bool lockEngineCamera = true;
	/*if (engine->Input()->GetPressKey(ESC))
	{
		lockEngineCamera = !lockEngineCamera;
		engine->Input()->SetLockMouse(!lockEngineCamera, 500, 200);
		engine->Input()->SetHideCursor(!lockEngineCamera);
	}*/

	if (engine->Input()->GetKeyPressed(ESC))
	{
		if (lockEngineCamera)
		{
			engine->Input()->SetLockMouse(true, 500, 200);
			engine->Input()->SetHideCursor(true);
			lockEngineCamera = false;
		}
		else
		{
			engine->Input()->SetLockMouse(false, 500, 200);
			engine->Input()->SetHideCursor(false);
			lockEngineCamera = true;
		}
	}
	
	/*if (engine->Input()->GetMouseUp(MOUSE_BUTTON::RIGHT))
	{
		if (!lockEngineCamera)
		{
			engine->Input()->SetLockMouse(false, 500, 200);
			engine->Input()->SetHideCursor(false);
			lockEngineCamera = true;
		}
	}*/

	if (lockEngineCamera) return false;

	Vec3 motion = {};

	if (engine->Input()->GetKey('W'))
	{
		motion += m_transform.GetForwardDir().Normalize();
		//m_position = m_position + m_transform.GetForwardDir().Normalize() * m_speed * engine->FDeltaTime();
	}
	if (engine->Input()->GetKey('S'))
	{
		motion -= m_transform.GetForwardDir().Normalize();
		//m_position = m_position - m_transform.GetForwardDir().Normalize() * m_speed * engine->FDeltaTime();
	}
	if (engine->Input()->GetKey('A'))
	{
		motion -= m_transform.GetRightwardDir().Normalize();
		//m_position = m_position - m_transform.GetRightwardDir().Normalize() * m_speed * engine->FDeltaTime();
	}
	if (engine->Input()->GetKey('D'))
	{
		motion += m_transform.GetRightwardDir().Normalize();
		//m_position = m_position + m_transform.GetRightwardDir().Normalize() * m_speed * engine->FDeltaTime();
	}

	m_position += motion.Normalize() * m_speed * engine->FDeltaTime();

	if (engine->Input()->IsMouseMove())
	{
		int* delta = engine->Input()->GetDeltaMousePos();

		//std::cout << delta[0] << ", " << delta[1] << "\n";

		m_rotateY += delta[0] * engine->FDeltaTime() * m_rotationSensi;
		m_rotateX -= delta[1] * engine->FDeltaTime() * m_rotationSensi;

		m_rotateX = std::max(std::min(m_rotateX, PI / 2.0f), -PI / 2.0f);
	}

	return true;
}

void CameraController::Update(Engine* engine)
{
	if (!UpdateControl(engine)) 
	{
		m_change = 0;
		return;
	}

	m_change = 1;

	m_transform.SetIdentity();
	m_transform *= GetRotationYMatrix(m_rotateY);

	auto rwd = m_transform.GetRightwardDir().Normalize();

	m_transform *= GetRotationMatrix(rwd, -m_rotateX);

	if (m_rotateZ != 0)
	{
		auto fwd = m_transform.GetForwardDir().Normalize();
		m_transform *= GetRotationMatrix(fwd, m_rotateZ);
	}

	m_transform *= GetTranslationMatrix(m_position);
}

void CameraController::WriteTo(SceneObject* object)
{
	if (!m_change) return;

	object->Transform() = m_transform;
	object->ExternalData().As<Mat4x4>() = m_proj;

	object->DataChanged() = true;
	object->ExternalDataChanged() = true;
	m_change = false;
}

void CameraController::WriteToShared(SceneSharedObject* object)
{
	if (!m_change) return;

	object->Transform() = m_transform;
	object->ExternalData().As<Mat4x4>() = m_proj;

	object->DataChanged() = true;
	object->ExternalDataChanged() = true;
	m_change = false;
}