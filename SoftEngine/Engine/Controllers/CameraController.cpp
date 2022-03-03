#include "CameraController.h"

#include "Engine/Engine.h"

//#include "Engine/Global.h"

#include "Engine/Scene/Scene.h"

CameraController::CameraController(const Vec3& position, const Vec3& focusPos, 
	float fov, float dNear, float dFar, float aspectRatio, float speed)
{
	m_proj.SetPerspectiveFovLH(fov, dNear, dFar, aspectRatio);

	m_transform.SetLookAtLH(position, focusPos, { 0,1,0 });

	//m_mvp = m_transform * m_proj;

	m_position = position;

	m_speed = speed;

	Vec3 direction = (focusPos - position).Normalize();
	m_rotateX = asin(direction.y);
	m_rotateY = atan2(direction.x, direction.z);
}

CameraController::~CameraController()
{
}

bool CameraController::Update(SceneQueryContext* context, 
	SceneQueriedNode* node, const Mat4x4& globalTransform, Engine* engine)
{
	if (!UpdateControl(engine)) return false;

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

	//m_mvp = GetInverse(m_transform) * m_proj;

	node->Blob().AsCamera().proj = m_proj;
	node->Transform() = m_transform;

	context->FlushBackData(node);

	return true;
}

void CameraController::CallMethod(SceneQueryContext* context, SceneQueriedNode* node, const Mat4x4& globalTransform,
	int methodId, void* args, int argc, Engine* engine)
{
}

bool CameraController::UpdateControl(Engine* engine)
{
	static bool lockEngineCamera = true;
	if (engine->Input()->GetPressKey(TAB))
	{
		lockEngineCamera = !lockEngineCamera;
		engine->Input()->SetLockMouse(!lockEngineCamera, 500, 200);
		engine->Input()->SetHideCursor(!lockEngineCamera);
	}

	if (lockEngineCamera) return false;

	if (engine->Input()->GetKey('W'))
	{
		m_position = m_position + m_transform.GetForwardDir().Normalize() * m_speed * engine->FDeltaTime();
	}
	if (engine->Input()->GetKey('S'))
	{
		m_position = m_position - m_transform.GetForwardDir().Normalize() * m_speed * engine->FDeltaTime();
	}
	if (engine->Input()->GetKey('A'))
	{
		m_position = m_position - m_transform.GetRightwardDir().Normalize() * m_speed * engine->FDeltaTime();
	}
	if (engine->Input()->GetKey('D'))
	{
		m_position = m_position + m_transform.GetRightwardDir().Normalize() * m_speed * engine->FDeltaTime();
	}

	if (engine->Input()->IsMouseMove())
	{
		int* delta = engine->Input()->GetDeltaMousePos();

		//std::cout << delta[0] << ", " << delta[1] << "\n";

		m_rotateY += delta[0] * engine->FDeltaTime() * m_rotationSensi;
		m_rotateX -= delta[1] * engine->FDeltaTime() * m_rotationSensi;

		m_rotateX = max(min(m_rotateX, PI / 2.0f), -PI / 2.0f);
	}

	return true;
}