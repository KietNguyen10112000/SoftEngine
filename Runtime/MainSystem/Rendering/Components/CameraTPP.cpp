#include "CameraTPP.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "MainSystem/Rendering/RenderingSystem.h"
#include "MainSystem/Rendering/CAMERA_PRIORITY.h"

#include "../RenderingPipeline/BasicRenderingPipeline.h"

NAMESPACE_BEGIN

CameraTPP::CameraTPP()
{
	m_view.SetLookAtLH({ 5,5,5 }, Vec3::ZERO, Vec3::UP);
}

void CameraTPP::OnCameraRenderBegin()
{
	m_globalTransform = Mat4::Translation(-m_target->ReadGlobalTransformMat().Position()) * m_view;
	m_globalTransform.Inverse();
}

//Mat4 CameraTPP::GetView()
//{
//	return Mat4::Translation(-m_target->ReadGlobalTransformMat().Position()) * m_view;
//
//	/*auto pos = m_target->ReadGlobalTransformMat().Position();
//
//	Mat4 res = m_view.SetLookAtLH(pos + Vec3{ 5,5,5 }, pos, Vec3::UP);
//
//	return res;*/
//}

void CameraTPP::SetTarget(const Handle<GameObject>& object)
{
	m_target = object;
}

void CameraTPP::SetViewPoint(const Vec3& point, const Vec3& up)
{
	struct Param
	{
		CameraTPP* camera;
		Mat4 view;
	};

	auto view = Mat4().SetLookAtLH(point, Vec3::ZERO, up);

	if (!GetGameObject() || !GetGameObject()->IsInAnyScene())
	{
		m_view = view;
		return;
	}

	auto system = GetGameObject()->GetScene()->GetRenderingSystem();
	auto taskRunner = system->AsyncTaskRunnerMT();

	auto task = taskRunner->CreateTask(
		[](RenderingSystem* system, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_REF_2(Param, p, camera, view);
			camera->m_view = view;
		}
	);

	auto param = taskRunner->CreateParam<Param>(&task);
	param->camera = this;
	param->view = view;

	taskRunner->RunAsync(&task);
}

NAMESPACE_END