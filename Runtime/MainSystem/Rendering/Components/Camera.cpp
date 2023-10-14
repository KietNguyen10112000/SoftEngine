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

void Camera::Serialize(ByteStream& stream)
{
}

void Camera::Deserialize(ByteStreamRead& stream)
{
}

void Camera::CleanUp()
{
}

Handle<ClassMetadata> Camera::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void Camera::OnPropertyChanged(const UnknownAddress& var)
{
}

void Camera::OnComponentAdded()
{
	auto rdrSys = GameObject()->Scene()->GetRenderingSystem();
	rdrSys->AddCamera(this, CAMERA_PRIORITY_LOWEST);
	rdrSys->DisplayCamera(this, rdrSys->GetDefaultViewport());
}

void Camera::OnComponentRemoved()
{
	auto rdrSys = GameObject()->Scene()->GetRenderingSystem();
	rdrSys->RemoveCamera(this);
}

AABox Camera::GetGlobalAABB()
{
	return AABox();
}

NAMESPACE_END