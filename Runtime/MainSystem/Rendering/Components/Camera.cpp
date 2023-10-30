#include "Camera.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "MainSystem/Rendering/RenderingSystem.h"
#include "MainSystem/Rendering/CAMERA_PRIORITY.h"

#include "../RenderingPipeline/BasicRenderingPipeline.h"

NAMESPACE_BEGIN

Camera::Camera() : BaseCamera(RENDER_TYPE::RENDER_TYPE_CAMERA)
{
	// create render target for this camera
	GRAPHICS_RENDER_TARGET_DESC desc = {};
	desc.format = GRAPHICS_DATA_FORMAT::FORMAT_R8G8B8A8_UNORM;
	desc.width = -1;
	desc.height = -1;
	desc.mipLevels = 1;
	Graphics::Get()->CreateRenderTargets(1, &desc, &m_renderTarget);

	// create rendering pipeline
	m_pipeline = new BasicRenderingPipeline();
}

//Camera::~Camera()
//{
//	CleanUp();
//}

void Camera::Serialize(Serializer* serializer)
{
}

void Camera::Deserialize(Serializer* serializer)
{
}

void Camera::CleanUp()
{
	delete m_pipeline;
	m_pipeline = nullptr;
}

Handle<ClassMetadata> Camera::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void Camera::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

void Camera::OnComponentAdded()
{
	auto rdrSys = GetGameObject()->GetScene()->GetRenderingSystem();
	rdrSys->AddCamera(this, CAMERA_PRIORITY_LOWEST);
	rdrSys->DisplayCamera(this, rdrSys->GetDefaultViewport());

	/*static int count = 0;

	if (count++)
	{
		auto vp = rdrSys->GetDefaultViewport();
		vp.size /= 2.0f;
		rdrSys->DisplayCamera(this, vp);
	}
	else
	{
		auto vp = rdrSys->GetDefaultViewport();
		vp.size /= 2.0f;
		vp.topLeft += vp.size;
		rdrSys->DisplayCamera(this, vp);
	}*/
}

void Camera::OnComponentRemoved()
{
	auto rdrSys = GetGameObject()->GetScene()->GetRenderingSystem();
	rdrSys->RemoveCamera(this);
}

AABox Camera::GetGlobalAABB()
{
	return AABox();
}

NAMESPACE_END